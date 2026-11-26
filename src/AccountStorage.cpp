#include "AccountManager.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <iostream>
using namespace std;

static const char* account_file = "data/platform_accounts_plain.txt";
static const char* account_file_header = "CAMPUS_ACCOUNTS_V3";

// V2 的非零权限值读作管理员身份。V3 只存 0（学生）和 1（管理员）。
bool AccountManager::load() {
	errno = 0;
	ifstream in(account_file);
	if (!in)
		return errno == ENOENT;
	string line;
	if (!getline(in, line))
		return false;
	const bool legacy = line == "CAMPUS_ACCOUNTS_V2";
	if (!legacy && line != account_file_header)
		return false;
	const int max_role = legacy ? 15 : 1;

	bool has_admin = false;
	while (getline(in, line)) {
		istringstream row(line);
		Account account{};
		int role, active;
		string excess;
		if (!(row >> account.username >> account.student_id >> role >> active) ||
			row >> excess ||
			!validUsername(account.username) || find(account.username) != nullptr ||
			role < 0 || role > max_role ||
			(active != 0 && active != 1))
			return false;

		// 密码单独占一行，保留其中的空格。
		if (!getline(in, account.password) || account.password.size() < 8 ||
			account.password.size() > 128)
			return false;

		if (account.student_id == "-")
			account.student_id.clear();
		account.is_admin = role != 0;
		if (account.student_id.empty() && !account.is_admin)
			return false;
		if (findByStudentId(account.student_id) != nullptr)
			return false;

		account.active = active == 1;
		has_admin = has_admin || (account.active && account.is_admin);
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
			<< ' ' << account.is_admin << ' ' << account.active << '\n'
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
