// 各模块共用的输入检查、金额格式、时间和字符替换函数。
#ifndef INPUT_H
#define INPUT_H
#include <string>

// 余额上限为1000000元，程序内统一以分计算，避免浮点数舍入误差。
const long MAX_MONEY = 100000000L;

// 读取指定范围内的整数菜单选项，输入结束时返回 -1。
int readChoice(const std::string& prompt, int minimum, int maximum);
long readMoney(const std::string& prompt);
std::string moneyText(long long cents);
std::string currentTime();
// 简单一对一替换，仅在文件边界使用；没有额外的数据密码或加密类。
std::string encryptText(std::string text);
std::string decryptText(std::string text);
#endif
