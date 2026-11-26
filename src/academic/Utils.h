#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace std;

// 通用工具函数集合，集中处理文本分隔、输入保护等重复逻辑
// 数据文件统一用竖线 | 分隔字段，避免姓名、班级中出现空格导致解析错位

// 按指定分隔符把一行文本切分为字段数组
inline vector<string> split(const string& line, char sep) {
	vector<string> result;
	string field;
	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == sep) {
			result.push_back(field);
			field.clear();
		} else {
			field.push_back(line[i]);
		}
	}
	result.push_back(field);
	return result;
}

inline vector<string> parseFields(const string& line, const string& type, size_t count) {
	vector<string> fields = split(line, '|');
	if (fields.size() != count || fields[0] != type)
		throw invalid_argument("invalid record");
	return fields;
}

inline int parseInt(const string& text) {
	size_t end = 0;
	int value = stoi(text, &end);
	if (end != text.size())
		throw invalid_argument("invalid integer");
	return value;
}

inline double parseDouble(const string& text) {
	size_t end = 0;
	double value = stod(text, &end);
	if (end != text.size() || !std::isfinite(value))
		throw invalid_argument("invalid number");
	return value;
}

// 读取一整行输入，并去除首尾可能残留的空格
inline string readLine(const string& prompt) {
	cout << prompt;
	string s;
	while (true) {
		if (!getline(cin, s)) throw ios_base::failure("Input closed");
		if (s.find_first_of("|\t") == string::npos) break;
		cout << "请输入不含竖线或制表符的文本：";
	}
	size_t begin = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (begin == string::npos) return "";
	return s.substr(begin, end - begin + 1);
}

// 安全读取一个整数，输入非法时反复提示直到合法
inline int readInt(const string& prompt) {
	while (true) {
		string s = readLine(prompt);
		try {
			return parseInt(s);
		} catch (...) {
		}
		cout << "输入不合法，请重新输入一个整数。" << endl;
	}
}

// 安全读取一个浮点数
inline double readDouble(const string& prompt) {
	while (true) {
		string s = readLine(prompt);
		try {
			return parseDouble(s);
		} catch (...) {
		}
		cout << "输入不合法，请重新输入一个数字。" << endl;
	}
}

#endif
