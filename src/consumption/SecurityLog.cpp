#include "SecurityLog.h"
#include "Input.h"
#include <fstream>
#include <iostream>
using namespace std;

static const char* LOG_FILE = "data/consumption/security_log.txt";

// 以追加方式记录当前时间、操作者 username 与事件 event，保留之前的记录。
void SecurityLog::write(const string& username, const string& event) const {
	ofstream out(LOG_FILE, ios::app);
	out << currentTime() << '\t' << username << '\t' << event << '\n';
}

// 依次显示保存的安全事件，尚未产生日志时给出提示。
void SecurityLog::show() const {
	ifstream in(LOG_FILE);
	if (!in) {
		cout << "暂无安全日志。\n";
		return;
	}
	cout << "时间 | 用户名 | 事件\n";
	string line;
	while (getline(in, line)) cout << line << '\n';
}
