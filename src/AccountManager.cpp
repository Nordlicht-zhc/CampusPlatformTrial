#include "AccountManager.h"
#include "academic/AcademicManager.h"
#include "consumption/Input.h"
#include <windows.h>
#include <bcrypt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdio>
using namespace std;

static const char* account_file = "data/platform_accounts.txt";
static const size_t salt_size = 16;
static const size_t hash_size = 32;

// data 指向待转换的字节。每个字节写成两位十六进制，便于逐行保存。
static string toHex(const unsigned char* data, size_t size) {
	const char* digits = "0123456789abcdef";
	string result;
	for (size_t i = 0; i < size; i++) {
		result += digits[data[i] >> 4];
		result += digits[data[i] & 15];
	}
	return result;
}

// text 应有 size 个字节对应的十六进制字符。成功时把解码结果写到 data。
static bool fromHex(const string& text, unsigned char* data, size_t size) {
	const string digits = "0123456789abcdef";
	if (text.size() != size * 2)
		return false;

	for (size_t i = 0; i < size; i++) {
		size_t high = digits.find(text[i * 2]);
		size_t low = digits.find(text[i * 2 + 1]);
		if (high == string::npos || low == string::npos)
			return false;
		data[i] = static_cast<unsigned char>((high << 4) | low);
	}
	return true;
}

// 根据明文和独立随机盐导出验证值。PBKDF2 反复计算 HMAC-SHA256，结果仅用于验密。
static bool hashPassword(const string& password, unsigned char* salt, string& result) {
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
			BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
		return false;

	unsigned char digest[hash_size];
	const NTSTATUS status = BCryptDeriveKeyPBKDF2(algorithm,
		reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
		static_cast<ULONG>(password.size()), salt, static_cast<ULONG>(salt_size),
		600000, digest, static_cast<ULONG>(hash_size), 0);
	BCryptCloseAlgorithmProvider(algorithm, 0);
	if (status != 0)
		return false;
	result = toHex(digest, hash_size);
	return true;
}

// 用该账号的盐重新计算验证值。逐字节汇总差异，检查整个验证值是否相同。
static bool checkPassword(const Account& account, const string& password) {
	unsigned char salt[salt_size];
	string hash;
	if (!fromHex(account.salt, salt, salt_size) || !hashPassword(password, salt, hash))
		return false;

	unsigned int difference = 0;
	for (size_t i = 0; i < hash.size(); i++)
		difference |= hash[i] ^ account.password_hash[i];
	return difference == 0;
}

// 平台登录名允许字母、数字、下划线、短横线和点，限制为 1 到 32 个字符。
static bool validUsername(const string& username) {
	return !username.empty() && username.size() <= 32 &&
		username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.") == string::npos;
}

// 从控制台读取一个字段。输入结束或超出字段长度时取消本次操作。
static bool readField(const string& prompt, string& field, size_t max_size) {
	cout << prompt;
	if (!getline(cin, field))
		return false;
	if (field.size() <= max_size)
		return true;
	cout << "输入过长。\n";
	return false;
}

// 输入两遍新密码，然后更新 account 的随机盐和验证值。调用者负责保存。
static bool setPassword(Account& account) {
	string password, repeat;
	if (!readField("新密码（至少 8 位）：", password, 128) ||
		!readField("再次输入新密码：", repeat, 128))
		return false;
	if (password.size() < 8 || password != repeat) {
		cout << "密码长度不足或两次输入不一致。\n";
		return false;
	}

	unsigned char salt[salt_size];
	string hash;
	if (BCryptGenRandom(nullptr, salt, static_cast<ULONG>(salt_size),
			BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0 || !hashPassword(password, salt, hash)) {
		cerr << "生成密码验证值失败。\n";
		return false;
	}

	account.salt = toHex(salt, salt_size);
	account.password_hash = hash;
	return true;
}

// username 为登录名。返回账号地址，找不到时返回空指针。
Account* AccountManager::find(const string& username) {
	for (Account& account : accounts) {
		if (account.username == username)
			return &account;
	}
	return nullptr;
}

// 校验账号文件中的格式、权限、重复学号及启用的账号管理员。
bool AccountManager::load() {
	errno = 0;
	ifstream in(account_file);
	if (!in)
		return errno == ENOENT;
	string line;
	if (!getline(in, line) || line != "CAMPUS_ACCOUNTS_V1")
		return false;

	bool has_admin = false;
	while (getline(in, line)) {
		istringstream row(line);
		Account account{};
		int active;
		string excess;
		unsigned char salt[salt_size], hash[hash_size];
		if (!(row >> account.username >> account.student_id >> account.permissions >>
				active >> account.salt >> account.password_hash) || row >> excess ||
			!validUsername(account.username) || find(account.username) != nullptr ||
			account.permissions < 0 || account.permissions > AllPermissions ||
			(active != 0 && active != 1) || !fromHex(account.salt, salt, salt_size) ||
			!fromHex(account.password_hash, hash, hash_size))
			return false;

		if (account.student_id == "-") {
			account.student_id.clear();
			if (account.permissions == 0)
				return false;
		} else {
			for (const Account& existing : accounts) {
				if (existing.student_id == account.student_id)
					return false;
			}
		}

		account.active = active == 1;
		has_admin = has_admin || (account.active && account.has(ManageAccounts));
		accounts.push_back(account);
	}
	return in.eof() && has_admin;
}

// 先将完整账号表写到同目录临时文件，关闭成功后替换旧文件。
bool AccountManager::save()const {
	const string temp_path = string(account_file) + ".tmp";
	ofstream out(temp_path);
	out << "CAMPUS_ACCOUNTS_V1\n";
	for (const Account& account : accounts) {
		out << account.username << ' ' << (account.student_id.empty() ? "-" : account.student_id)
			<< ' ' << account.permissions << ' ' << account.active << ' '
			<< account.salt << ' ' << account.password_hash << '\n';
	}
	out.close();
	if (out && MoveFileExA(temp_path.c_str(), account_file,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		return true;

	std::remove(temp_path.c_str());
	cerr << "平台账号保存失败。\n";
	return false;
}

// 首次启动时建立 admin，并由操作者设置它的密码。
bool AccountManager::initialize() {
	if (!accounts.empty())
		return true;
	cout << "首次运行，请为平台管理员 admin 设置密码。\n";
	Account admin{};
	admin.username = "admin";
	admin.permissions = AllPermissions;
	admin.active = true;
	if (!setPassword(admin))
		return false;
	accounts.push_back(admin);
	return save();
}

// 读取账号和密码，核对启用状态、锁定状态和密码；成功后把身份复制到 session。
bool AccountManager::login(Session& session) {
	string username, password;
	if (!readField("用户名（0 返回）：", username, 32) || username == "0")
		return false;

	Account* account = find(username);
	if (login_guard.remaining(username) > 0) {
		cout << "账号已临时锁定，请稍后重试。\n";
		return false;
	}
	if (!readField("密码：", password, 128))
		return false;

	if (account == nullptr || !account->active || !checkPassword(*account, password)) {
		cout << "用户名或密码错误，或账号已停用。\n";
		if (account != nullptr && account->active && login_guard.recordFailure(username))
			cout << "连续失败 5 次，账号锁定 60 秒。\n";
		return false;
	}

	login_guard.clear(username);
	session = *account;
	return true;
}

// 管理员修改自身权限后，下一次平台菜单按账号表重新计算可用入口。
bool AccountManager::refresh(Session& session) {
	Account* account = find(session.username);
	if (account == nullptr || !account->active)
		return false;
	session = *account;
	return true;
}

// 权限值由各权限相加。0 表示只使用学号关联的学生服务。
static int readPermissions() {
	cout << "管理权限可相加（1 账号、2 学业、4 图书、8 消费，0 学生）。\n";
	return readChoice("权限值：", 0, AllPermissions);
}

// 可用 academic 校验学号归属。每名学生只能绑定一个平台账号。
void AccountManager::addAccount(const AcademicManager& academic) {
	Account account{};
	if (!readField("用户名：", account.username, 32))
		return;
	if (!validUsername(account.username) || account.username == "0" || find(account.username) != nullptr) {
		cout << "用户名无效或已使用。\n";
		return;
	}
	if (!readField("学号（管理账号可留空）：", account.student_id, 64))
		return;

	if (!account.student_id.empty()) {
		const Student* student = academic.getStudent(account.student_id);
		if (student == nullptr || !student->isInSchool() || account.student_id == "-") {
			cout << "未找到可绑定的在校学生。\n";
			return;
		}
		for (const Account& existing : accounts) {
			if (existing.student_id == account.student_id) {
				cout << "该学号已绑定平台账号。\n";
				return;
			}
		}
	}

	account.permissions = readPermissions();
	if (account.permissions < 0)
		return;
	if (account.student_id.empty() && account.permissions == 0) {
		cout << "请选择管理权限或绑定学号。\n";
		return;
	}
	if (!setPassword(account))
		return;

	account.active = true;
	accounts.push_back(account);
	if (save())
		cout << "账号已创建。\n";
	else
		accounts.pop_back();
}

// 保存新密码后清除锁定计数。写入失败时恢复原账号内容。
bool AccountManager::updatePassword(Account& account) {
	const Account old = account;
	if (!setPassword(account))
		return false;
	if (!save()) {
		account = old;
		return false;
	}
	login_guard.clear(account.username);
	cout << "密码已更新。\n";
	return true;
}

// 编辑指定账号。保留当前操作者的账号管理权限，确保始终可管理平台。
void AccountManager::editAccount(const string& current_username) {
	string username;
	if (!readField("要编辑的用户名：", username, 32))
		return;
	Account* account = find(username);
	if (account == nullptr) {
		cout << "账号不存在。\n";
		return;
	}
	int choice = readChoice("1 重置密码  2 分配权限  3 启用或停用  0 返回：", 0, 3);
	if (choice <= 0)
		return;
	if (choice == 1) {
		updatePassword(*account);
		return;
	}

	const Account old = *account;
	if (choice == 2) {
		int permissions = readPermissions();
		if (permissions < 0)
			return;
		if ((account->student_id.empty() && permissions == 0) ||
			(username == current_username && (permissions & ManageAccounts) == 0)) {
			cout << "该账号需要保留相应的管理权限。\n";
			return;
		}
		account->permissions = permissions;
	} else {
		if (username == current_username) {
			cout << "请由另一位账号管理员处理当前账号。\n";
			return;
		}
		account->active = !account->active;
	}

	if (save())
		cout << "账号已更新。\n";
	else
		*account = old;
}

// session 是已登录身份。只有账号管理员能访问创建、编辑和列表功能。
void AccountManager::run(const AcademicManager& academic, const Session& session) {
	if (!session.has(ManageAccounts))
		return;
	while (true) {
		cout << "\n========== 平台账号管理 ==========\n";
		int choice = readChoice("1 创建账号  2 编辑账号  3 账号列表  0 返回：", 0, 3);
		if (choice <= 0)
			return;
		if (choice == 1)
			addAccount(academic);
		else if (choice == 2)
			editAccount(session.username);
		else {
			for (const Account& account : accounts) {
				cout << account.username << " | "
					<< (account.student_id.empty() ? "-" : account.student_id)
					<< " | 权限 " << account.permissions << " | "
					<< (account.active ? "启用" : "停用") << '\n';
			}
		}
	}
}

// 修改本人密码前先核验原密码。session 只提供当前登录名，密码值读取账号表。
void AccountManager::changePassword(const Session& session) {
	Account* account = find(session.username);
	string password;
	if (account == nullptr || !readField("当前密码：", password, 128))
		return;
	if (!checkPassword(*account, password)) {
		cout << "当前密码错误。\n";
		return;
	}
	updatePassword(*account);
}
