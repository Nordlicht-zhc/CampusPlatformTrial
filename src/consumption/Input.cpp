// 集中处理输入和格式转换，菜单、账户与流水模块直接调用这些函数。
#include "Input.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
using namespace std;

// 上下两行同一位置的字符互相对应。大写、小写、数字分别循环后移3位，不能改成重复字符。
static const string plainChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const string secretChars = "DEFGHIJKLMNOPQRSTUVWXYZABCdefghijklmnopqrstuvwxyzabc3456789012";

// text是需要保存的字符串。逐个查密码本，找得到就替换，其他字符保持原样。
string encryptText(string text) {
	for (size_t i = 0; i < text.size(); ++i) {
		size_t pos = plainChars.find(text[i]);
		if (pos != string::npos) text[i] = secretChars[pos];
	}
	return text;
}

// text是文件中读到的字符串。反查同一份密码本，返回替换前的内容。
string decryptText(string text) {
	for (size_t i = 0; i < text.size(); ++i) {
		size_t pos = secretChars.find(text[i]);
		if (pos != string::npos) text[i] = plainChars[pos];
	}
	return text;
}

// 去掉输入两端的空白，不改动中间的字符。主要给金额输入使用。
static string trim(const string& text) {
	const string spaces = " \t\r\n";
	size_t first = text.find_first_not_of(spaces);
	if (first == string::npos) return "";
	return text.substr(first, text.find_last_not_of(spaces) - first + 1);
}

// prompt 为输入提示。反复检查选项是否在指定范围，输入结束时返回 -1。
int readChoice(const string& prompt, int minimum, int maximum) {
	while (true) {
		cout << prompt;
		string line, extra;
		if (!getline(cin, line))
			return -1;
		istringstream input(line);
		int choice;
		if ((input >> choice) && !(input >> extra) && choice >= minimum && choice <= maximum)
			return choice;
		cout << "菜单选项无效。\n";
	}
}

// 把元转换成整数分放进cents，返回是否成功。逐位处理数字，不使用浮点数计算余额。
static bool parseMoney(const string& text, long& cents) {
	long yuan = 0;
	size_t i = 0;
	// 先读整数元，过程中检查上限，过大的输入不用继续计算。
	while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
		yuan = yuan * 10 + text[i] - '0';
		if (yuan > MAX_MONEY / 100) return false;
		++i;
	}
	if (i == 0) return false;
	long fraction = 0;
	if (i < text.size()) {
		if (text[i++] != '.') return false;
		size_t digits = text.size() - i;
		if (digits < 1 || digits > 2) return false;
		for (size_t j = 0; j < digits; ++j) {
			if (text[i + j] < '0' || text[i + j] > '9') return false;
			fraction = fraction * 10 + text[i + j] - '0';
		}
		if (digits == 1) fraction *= 10;
	}
	// 小数只有一位时已经补成角，两位时按分处理。
	cents = yuan * 100 + fraction;
	return cents > 0 && cents <= MAX_MONEY;
}

// 返回以分为单位的金额。输入无效或结束时返回 -1。
long readMoney(const string& prompt) {
	cout << prompt;
	string text;
	if (!getline(cin, text)) return -1;
	long cents;
	if (parseMoney(trim(text), cents)) return cents;
	cout << "金额无效，本次操作取消。\n";
	return -1;
}

// 把以分为单位的cents格式化成两位小数，供屏幕显示和日志使用。
string moneyText(long long cents) {
	ostringstream out;
	out << cents / 100 << '.' << setw(2) << setfill('0') << cents % 100;
	return out.str();
}

// 取得本机时间，组成不含空格的时间串，方便文件按字段读取。
string currentTime() {
	time_t now = time(NULL);
	tm* local = localtime(&now);
	char buffer[32];
	if (local == NULL) return "1970-01-01_00:00:00";
	strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", local);
	return buffer;
}
