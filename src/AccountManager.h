#ifndef PLATFORM_ACCOUNT_MANAGER_H
#define PLATFORM_ACCOUNT_MANAGER_H

#include <string>
#include <vector>
#include "consumption/LoginGuard.h"

// 会话保存已认证的登录名、业务学号和管理员身份。
struct Session {
	std::string username, student_id;
	bool is_admin;
};

// 账号额外保存密码和启用状态。
struct Account : public Session {
	std::string password;
	bool active;
};

enum class StudentAccountStatus { Failed, Active, Disabled };

// 统一负责账号创建、读写、认证和密码变更。
class AccountManager {
private:
	std::vector<Account> accounts;
	LoginGuard login_guard;

	Account* find(const std::string& username);
	const Account* findByStudentId(const std::string& student_id)const;
	static bool validUsername(const std::string& username);
	bool save()const;
	bool saveNewAccount(Account& account);
	bool readUnusedUsername(std::string& username);
	void updatePassword(Account& account);
	static bool readNewPassword(Account& account);

public:
	bool load();
	bool initialize();
	bool login(Session& session);
	void createAdminAccount(const Session& session);
	StudentAccountStatus ensureStudentAccount(const std::string& student_id);
	void changePassword(const Session& session);
};

#endif
