#include "AccountManager.h"
#include "academic/AcademicManager.h"
#include "consumption/CampusSystem.h"
#include "consumption/Input.h"
#include "library/CampusLibrary.h"
#include <windows.h>
#include <iostream>
using namespace std;

// 将数据路径固定在 EXE 旁边。使用宽字符接口处理安装路径中的中文。
static bool prepareData() {
	wchar_t buffer[32768];
	DWORD length = GetModuleFileNameW(nullptr, buffer, sizeof(buffer) / sizeof(buffer[0]));
	if (length == 0 || length >= sizeof(buffer) / sizeof(buffer[0]))
		return false;
	wstring folder(buffer, length);
	size_t slash = folder.find_last_of(L"\\/");
	if (slash == wstring::npos || !SetCurrentDirectoryW(folder.substr(0, slash + 1).c_str()))
		return false;

	const wchar_t* dirs[] = { L"data", L"data/academic", L"data/library", L"data/consumption" };
	for (const wchar_t* dir : dirs) {
		if (!CreateDirectoryW(dir, nullptr)) {
			DWORD attributes = GetFileAttributesW(dir);
			if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				return false;
		}
	}
	return true;
}

// session 提供账号权限及学号。每次显示菜单前重新核对在校状态。
static void runPlatform(AcademicManager& academic, AccountManager& accounts, Session session) {
	while (accounts.refresh(session)) {
		const Student* student = academic.getStudent(session.student_id);
		const bool is_student = student != nullptr && student->isInSchool();
		cout << "\n========== 校园综合服务平台 ==========\n"
			<< "当前账号：" << session.username << '\n';
		if (is_student)
			cout << "学号：" << student->getId() << '\n';
		if (is_student || session.has(ManageAcademic))
			cout << "1. 学生学业管理\n";
		if (is_student || session.has(ManageLibrary))
			cout << "2. 图书管理\n";
		if (is_student || session.has(ManageConsumption))
			cout << "3. 校园消费\n";
		if (session.has(ManageAccounts))
			cout << "4. 平台账号管理\n";
		cout << "5. 修改本人密码\n0. 注销\n";

		int choice = readChoice("请选择：", 0, 5);
		if (choice <= 0)
			return;

		if (choice == 1 && (is_student || session.has(ManageAcademic))) {
			academic.run(session.student_id, session.has(ManageAcademic));
		} else if (choice == 2 && (is_student || session.has(ManageLibrary))) {
			runCampusLibrary(session.student_id, is_student ? student->getName() : "",
				session.has(ManageLibrary));
		} else if (choice == 3 && (is_student || session.has(ManageConsumption))) {
			CampusSystem consumption;
			consumption.run(academic, session.student_id, session.has(ManageConsumption));
		} else if (choice == 4 && session.has(ManageAccounts)) {
			accounts.run(academic, session);
		} else if (choice == 5) {
			accounts.changePassword(session);
		} else {
			cout << "当前账号没有该功能的权限。\n";
		}
	}
}

// 加载统一账号和学业资料，维持登录、注销和退出循环。
static int runLogin() {
	AccountManager accounts;
	if (!accounts.load()) {
		cerr << "无法加载平台账号文件。\n";
		return 1;
	}
	if (!accounts.initialize())
		return 1;

	AcademicManager academic("data/academic");
	while (true) {
		cout << "\n========== 校园综合服务平台 ==========\n";
		int choice = readChoice("1 登录  0 退出：", 0, 1);
		if (choice <= 0)
			break;
		Session session{};
		if (accounts.login(session))
			runPlatform(academic, accounts, session);
	}
	return 0;
}

// 初始化 UTF-8 控制台和数据目录。独占文件句柄防止两个平台进程同时写同一份存档。
int main() {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	if (!prepareData()) {
		cerr << "无法准备平台数据目录。\n";
		return 1;
	}

	HANDLE lock = CreateFileW(L"data/platform.lock", GENERIC_WRITE | DELETE, 0, nullptr,
		OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (lock == INVALID_HANDLE_VALUE) {
		cerr << "无法独占数据目录，请检查是否已有平台窗口正在运行。\n";
		return 1;
	}
	int result = runLogin();
	CloseHandle(lock);
	return result;
}
