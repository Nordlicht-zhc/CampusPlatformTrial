// 安全日志只负责追加事件和显示内容。
#ifndef SECURITY_LOG_H
#define SECURITY_LOG_H
#include <string>

// 账本安全日志负责追加核对事件和查看历史，日志文件独立于余额和流水。
class SecurityLog {
public:
	void write(const std::string& username, const std::string& event) const;
	void show() const;
};
#endif
