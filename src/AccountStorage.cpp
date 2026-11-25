#include "AccountManager.h"
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <iostream>
using namespace std;

// 账号文件读写和密码验证。
static const char* account_file = "data/platform_accounts.txt";
static const size_t salt_size = 16;
static const size_t hash_size = 32;

// data 指向待转换的字节。每个字节写成两位十六进制，便于逐行保存。
static string toHex(const unsigned char* data, size_t size) {
	const char* digits = "0123456789abcdef";
	string result;
	for (size_t i = 0; i < size; i++) {
		result += digits[data[i] >> 4];
		result += digits[data[i] & 15];
	}
	return result;
}

// text 应有 size 个字节对应的十六进制字符。成功时把解码结果写到 data。
static bool fromHex(const string& text, unsigned char* data, size_t size) {
	const string digits = "0123456789abcdef";
	if (text.size() != size * 2)
		return false;

	for (size_t i = 0; i < size; i++) {
		size_t high = digits.find(text[i * 2]);
		size_t low = digits.find(text[i * 2 + 1]);
		if (high == string::npos || low == string::npos)
			return false;
		data[i] = static_cast<unsigned char>((high << 4) | low);
	}
	return true;
}

// 校验账号文件中的格式、权限、重复学号及启用的账号管理员。
bool AccountManager::load() {
	errno = 0;
	ifstream in(account_file);
	if (!in)
		return errno == ENOENT;
	string line;
	if (!getline(in, line) || line != "CAMPUS_ACCOUNTS_V1")
		return false;

	bool has_admin = false;
	while (getline(in, line)) {
		istringstream row(line);
		Account account{};
		int active;
		string excess;
		unsigned char salt[salt_size], hash[hash_size];
		if (!(row >> account.username >> account.student_id >> account.permissions >>
				active >> account.salt >> account.password_hash) || row >> excess ||
			!validUsername(account.username) || find(account.username) != nullptr ||
			account.permissions < 0 || account.permissions > AllPermissions ||
			(active != 0 && active != 1) || !fromHex(account.salt, salt, salt_size) ||
			!fromHex(account.password_hash, hash, hash_size))
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
	out << "CAMPUS_ACCOUNTS_V1\n";
	for (const Account& account : accounts) {
		out << account.username << ' ' << (account.student_id.empty() ? "-" : account.student_id)
			<< ' ' << account.permissions << ' ' << account.active << ' '
			<< account.salt << ' ' << account.password_hash << '\n';
	}
	out.close();
	if (out && MoveFileExA(temp_path.c_str(), account_file,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		return true;

	std::remove(temp_path.c_str());
	cerr << "平台账号保存失败。\n";
	return false;
}

// 根据明文和独立随机盐导出验证值。PBKDF2 反复计算 HMAC-SHA256，结果仅用于验密。
static bool hashPassword(const string& password, unsigned char* salt, string& result) {
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
			BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
		return false;

	unsigned char digest[hash_size];
	const NTSTATUS status = BCryptDeriveKeyPBKDF2(algorithm,
		reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
		static_cast<ULONG>(password.size()), salt, static_cast<ULONG>(salt_size),
		600000, digest, static_cast<ULONG>(hash_size), 0);
	BCryptCloseAlgorithmProvider(algorithm, 0);
	if (status != 0)
		return false;
	result = toHex(digest, hash_size);
	return true;
}

// 用该账号的盐重新计算验证值。逐字节汇总差异，检查整个验证值是否相同。
bool AccountManager::checkPassword(const Account& account, const string& password) {
	unsigned char salt[salt_size];
	string hash;
	if (!fromHex(account.salt, salt, salt_size) || !hashPassword(password, salt, hash))
		return false;

	unsigned int difference = 0;
	for (size_t i = 0; i < hash.size(); i++)
		difference |= hash[i] ^ account.password_hash[i];
	return difference == 0;
}

bool AccountManager::storePassword(Account& account, const string& password) {
	unsigned char salt[salt_size];
	string hash;
	if (BCryptGenRandom(nullptr, salt, static_cast<ULONG>(salt_size),
			BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0 || !hashPassword(password, salt, hash)) {
		cerr << "生成密码验证值失败。\n";
		return false;
	}

	account.salt = toHex(salt, salt_size);
	account.password_hash = hash;
	return true;
}
