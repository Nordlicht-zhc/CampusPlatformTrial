#ifndef PLATFORM_ACCOUNT_MANAGER_H
#define PLATFORM_ACCOUNT_MANAGER_H

#include <string>
#include <vector>
#include "consumption/LoginGuard.h"

class AcademicManager;

// 每一位表示一项管理权限。一个账号可同时负责多个模块。
enum Permission {
	ManageAccounts = 1,
	ManageAcademic = 2,
	ManageLibrary = 4,
	ManageConsumption = 8,
	AllPermissions = 15
};

// 会话保存已认证的登录名、业务学号和权限。学号为空表示未绑定学生身份。
struct Session {
	std::string username, student_id;
	int permissions;

	// 用位运算核对 permission 对应的管理权限。
	bool has(Permission permission)const {
		return (permissions & permission) != 0;
	}
};

// 账号在身份资料的基础上记录启用状态和验密所需的盐、哈希值。
struct Account : public Session {
	std::string salt, password_hash;
	bool active;
};

// 统一负责账号读写、认证、权限分配及密码变更。LoginGuard 记录本次运行的失败次数。
class AccountManager {
private:
	std::vector<Account> accounts;
	LoginGuard login_guard;

	Account* find(const std::string& username);
	bool save()const;
	void addAccount(const AcademicManager& academic);
	void editAccount(const std::string& current_username);
	bool updatePassword(Account& account);

public:
	bool load();
	bool initialize();
	bool login(Session& session);
	bool refresh(Session& session);
	void run(const AcademicManager& academic, const Session& session);
	void changePassword(const Session& session);
};

#endif
