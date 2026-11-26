#include "AccountManager.h"
#include <iostream>
using namespace std;

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

// 首次启动时建立 admin，并由操作者设置它的密码。
bool AccountManager::initialize() {
	if (!accounts.empty())
		return true;
	cout << "首次运行，请为平台管理员 admin 设置密码。\n";
	Account admin{};
	admin.username = "admin";
	admin.is_admin = true;
	while (!readNewPassword(admin)) {
		if (!cin)
			return false;
	}
	return saveNewAccount(admin);
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

	if (account == nullptr || !account->active || account->password != password) {
		cout << "用户名或密码错误，或账号已停用。\n";
		if (account != nullptr && account->active && login_guard.recordFailure(username))
			cout << "连续失败 5 次，账号锁定 60 秒。\n";
		return false;
	}

	login_guard.clear(username);
	session = *account;
	return true;
}

bool AccountManager::readUnusedUsername(string& username) {
	if (!readField("用户名（留空返回）：", username, 32) || username.empty())
		return false;
	if (!validUsername(username)) {
		cout << "用户名无效。\n";
		return false;
	}
	if (find(username) != nullptr) {
		cout << "用户名已使用。\n";
		return false;
	}
	return true;
}

void AccountManager::createAdminAccount(const Session& session) {
	if (!session.is_admin)
		return;
	cout << "\n========== 创建管理员账号 ==========\n";
	Account account{};
	account.is_admin = true;
	if (!readUnusedUsername(account.username))
		return;
	if (!readNewPassword(account))
		return;
	if (saveNewAccount(account))
		cout << "管理员账号已创建。\n";
}

// 学业录入和开卡前由各子系统确认学生身份，同一学号只建立一个账号。
StudentAccountStatus AccountManager::ensureStudentAccount(const string& student_id) {
	if (student_id.empty() || student_id == "-" || student_id.size() > 64 ||
			student_id.find_first_of("| \t\r\n\v\f") != string::npos) {
		cout << "学号格式无效，请核对学籍资料。\n";
		return StudentAccountStatus::Failed;
	}

	const Account* existing = findByStudentId(student_id);
	if (existing != nullptr) {
		cout << "已关联学生账号 " << existing->username
			 << (existing->active ? "。\n" : "（已停用）。\n");
		return existing->active ? StudentAccountStatus::Active : StudentAccountStatus::Disabled;
	}

	Account account{};
	account.student_id = student_id;
	account.username = student_id;
	if (!validUsername(account.username) || find(account.username) != nullptr) {
		cout << "该学号无法直接用作用户名，请为学生填写另一个用户名。\n";
		if (!readUnusedUsername(account.username))
			return StudentAccountStatus::Failed;
	}
	cout << "请为学生账号 " << account.username << " 设置初始密码。\n";
	if (!readNewPassword(account) || !saveNewAccount(account))
		return StudentAccountStatus::Failed;
	cout << "学生账号已创建，用户名为 " << account.username << "。\n";
	return StudentAccountStatus::Active;
}

bool AccountManager::saveNewAccount(Account& account) {
	account.active = true;
	accounts.push_back(account);
	if (save())
		return true;
	accounts.pop_back();
	return false;
}

// 修改本人密码前先核验原密码。session 只提供当前登录名，密码值读取账号表。
void AccountManager::changePassword(const Session& session) {
	Account* account = find(session.username);
	string password;
	if (account == nullptr || !readField("当前密码：", password, 128))
		return;
	if (account->password != password) {
		cout << "当前密码错误。\n";
		return;
	}
	updatePassword(*account);
}

// 保存新密码后清除锁定计数。写入失败时恢复原密码。
void AccountManager::updatePassword(Account& account) {
	const string old_password = account.password;
	if (!readNewPassword(account))
		return;
	if (!save()) {
		account.password = old_password;
		return;
	}
	login_guard.clear(account.username);
	cout << "密码已更新。\n";
}

// 登录名允许字母、数字、下划线、短横线和点。0 保留给登录菜单的返回操作。
bool AccountManager::validUsername(const string& username) {
	return !username.empty() && username.size() <= 32 && username != "0" &&
		username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.") == string::npos;
}

// 新密码输入两遍，确认长度和一致性后赋给账号。
bool AccountManager::readNewPassword(Account& account) {
	string password, repeat;
	if (!readField("新密码（至少 8 位）：", password, 128) ||
		!readField("再次输入新密码：", repeat, 128))
		return false;
	if (password.size() < 8 || password != repeat) {
		cout << "密码长度不足或两次输入不一致。\n";
		return false;
	}

	account.password = password;
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

const Account* AccountManager::findByStudentId(const string& student_id)const {
	if (student_id.empty())
		return nullptr;
	for (const Account& account : accounts) {
		if (account.student_id == student_id)
			return &account;
	}
	return nullptr;
}
