#include "AccountManager.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <iostream>
using namespace std;

static const char* account_file = "data/platform_accounts_plain.txt";
static const char* account_file_header = "CAMPUS_ACCOUNTS_V2";

// 校验账号文件中的格式、权限、重复学号及启用的账号管理员。
bool AccountManager::load() {
	errno = 0;
	ifstream in(account_file);
	if (!in)
		return errno == ENOENT;
	string line;
	if (!getline(in, line) || line != account_file_header)
		return false;

	bool has_admin = false;
	while (getline(in, line)) {
		istringstream row(line);
		Account account{};
		int active;
		string excess;
		if (!(row >> account.username >> account.student_id >> account.permissions >> active) ||
			row >> excess ||
			!validUsername(account.username) || find(account.username) != nullptr ||
			account.permissions < 0 || account.permissions > AllPermissions ||
			(active != 0 && active != 1))
			return false;

		// 密码单独占一行，保留其中的空格。
		if (!getline(in, account.password) || account.password.size() < 8 ||
			account.password.size() > 128)
			return false;

		if (account.student_id == "-")
			account.student_id.clear();
		if (account.student_id.empty() && account.permissions == 0)
			return false;
		if (studentIdInUse(account.student_id))
			return false;

		account.active = active == 1;
		has_admin = has_admin || (account.active && account.has(ManageAccounts));
		accounts.push_back(account);
	}
	return in.eof() && has_admin;
}

// 先将完整账号表写到同目录临时文件，关闭成功后替换旧文件。
bool AccountManager::save()const {
	const string temp_path = string(account_file) + ".tmp";
	ofstream out(temp_path);
	out << account_file_header << '\n';
	for (const Account& account : accounts) {
		out << account.username << ' ' << (account.student_id.empty() ? "-" : account.student_id)
			<< ' ' << account.permissions << ' ' << account.active << '\n'
			<< account.password << '\n';
	}
	out.close();
	if (out && MoveFileExA(temp_path.c_str(), account_file,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		return true;

	std::remove(temp_path.c_str());
	cerr << "平台账号保存失败。\n";
	return false;
}
