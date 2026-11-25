#include "LoginGuard.h"
using namespace std;

// username 为登录名。查找该账号的截止时间并计算剩余秒数，未锁定时为零。
long LoginGuard::remaining(const string& username) const {
	auto it = states.find(username);
	if (it == states.end()) return 0;
	time_t now = time(NULL);
	if (it->second.lockedUntil <= now) return 0;
	return static_cast<long>(it->second.lockedUntil - now);
}

// 返回 true 表示这次输错触发了锁定。
bool LoginGuard::recordFailure(const string& username) {
	LoginState& state = states[username];
	if (++state.failures < 5) return false;
	state.failures = 0;
	state.lockedUntil = time(NULL) + 60;
	return true;
}

// 删除 username 对应的失败计数与锁定状态，供登录成功或改密后调用。
void LoginGuard::clear(const string& username) {
	states.erase(username);
}
