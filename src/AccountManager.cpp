#include "AccountManager.h"
#include "academic/AcademicManager.h"
#include "consumption/Input.h"
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

// 权限值由各权限相加。0 表示只使用学号关联的学生服务。
static int readPermissions() {
	cout << "管理权限可相加（1 账号、2 学业、4 图书、8 消费，0 学生）。\n";
	return readChoice("权限值：", 0, AllPermissions);
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
	while (!readNewPassword(admin)) {
		if (!cin)
			return false;
	}
	accounts.push_back(admin);
	if (save())
		return true;
	accounts.pop_back();
	return false;
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

// 返回平台菜单时，从账号表重新读取当前账号的状态和权限。
bool AccountManager::refresh(Session& session) {
	Account* account = find(session.username);
	if (account == nullptr || !account->active)
		return false;
	session = *account;
	return true;
}

// session 是已登录身份。只有账号管理员能访问创建、编辑和列表功能。
void AccountManager::run(const AcademicManager& academic, const Session& session) {
	if (!session.has(ManageAccounts))
		return;
	while (true) {
		cout << "\n========== 平台账号管理 ==========\n";
		int choice = readChoice("1 创建账号  2 编辑账号  3 账号列表  0 返回：", 0, 3);
		switch (choice) {
		case 1:
			addAccount(academic);
			break;
		case 2:
			editAccount(session.username);
			break;
		case 3:
			for (const Account& account : accounts) {
				cout << account.username << " | "
					<< (account.student_id.empty() ? "-" : account.student_id)
					<< " | 权限 " << account.permissions << " | "
					<< (account.active ? "启用" : "停用") << '\n';
			}
			break;
		default:
			return;
		}
	}
}

// 先校验用户名和学号，再分配权限、设置密码并保存。
void AccountManager::addAccount(const AcademicManager& academic) {
	Account account{};
	if (!readField("用户名：", account.username, 32))
		return;
	if (!validUsername(account.username)) {
		cout << "用户名无效。\n";
		return;
	}
	if (find(account.username) != nullptr) {
		cout << "用户名已使用。\n";
		return;
	}
	if (!readField("学号（管理账号可留空）：", account.student_id, 64))
		return;
	if (account.student_id == "-" || account.student_id.find_first_of(" \t\r\n\v\f") != string::npos) {
		cout << "学号格式无效，请输入不含空白字符的有效学号。\n";
		return;
	}

	if (!account.student_id.empty()) {
		const Student* student = academic.getStudent(account.student_id);
		if (student == nullptr || !student->isInSchool()) {
			cout << "未找到可绑定的在校学生。\n";
			return;
		}
		if (studentIdInUse(account.student_id)) {
			cout << "该学号已绑定平台账号。\n";
			return;
		}
	}

	account.permissions = readPermissions();
	if (account.permissions < 0)
		return;
	if (account.student_id.empty() && account.permissions == 0) {
		cout << "请选择管理权限或绑定学号。\n";
		return;
	}
	if (!readNewPassword(account))
		return;

	account.active = true;
	accounts.push_back(account);
	if (save())
		cout << "账号已创建。\n";
	else
		accounts.pop_back();
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

bool AccountManager::studentIdInUse(const string& student_id)const {
	if (student_id.empty())
		return false;
	for (const Account& account : accounts) {
		if (account.student_id == student_id)
			return true;
	}
	return false;
}
