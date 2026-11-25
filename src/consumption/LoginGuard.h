// 只在本次运行中记录每个账号的失败次数和解锁时间。
#ifndef LOGIN_GUARD_H
#define LOGIN_GUARD_H
#include <ctime>
#include <map>
#include <string>

// 按平台登录名保存失败次数和锁定截止时间，由 AccountManager 调用。
class LoginGuard {
private:
	struct LoginState {
		int failures = 0;
		std::time_t lockedUntil = 0;
	};
	std::map<std::string, LoginState> states;

public:
	long remaining(const std::string& username) const;
	bool recordFailure(const std::string& username);
	void clear(const std::string& username);
};
#endif
