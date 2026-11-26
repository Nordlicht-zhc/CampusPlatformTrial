// 处理账户、交易、菜单和账本。
#include "CampusSystem.h"
#include "../academic/AcademicManager.h"
#include <windows.h>
#include <cerrno>
#include <cstdio>
#include "Input.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
using namespace std;

static const char* DATA_FILE = "data/consumption/campus_data.txt";
// 将流水编号从 1 开始，旧存档加载时读取实际的下一编号。
CampusSystem::CampusSystem():nextRecordId(1) {}

// student_id 为稳定的学号。返回该学号对应的卡片下标，找不到时返回 -1。
int CampusSystem::findUser(const string& student_id) const {
	for (size_t i = 0; i < users.size(); i++) {
		if (users[i].getStudentId() == student_id)
			return static_cast<int>(i);
	}
	return -1;
}

// 已经销户但仍有历史流水的学号继续保留。
bool CampusSystem::idUsed(const string& student_id) const {
	if (findUser(student_id) != -1)
		return true;
	for (const Record& record : records) {
		if (record.getStudentId() == student_id)
			return true;
	}
	return false;
}

// 从集成版 V2 或原版 V1 读取余额和流水。登录信息仅由平台账号文件提供。
bool CampusSystem::load() {
	errno = 0;
	ifstream in(DATA_FILE);
	if (!in) return errno == ENOENT;
	string magic;
	if (!(in >> magic)) return in.eof();
	const bool legacy = magic == "CAMPUS_PAY_SUB_V1";
	if (!legacy && magic != "CAMPUS_PAY_SUB_V2")
		return false;

	if (legacy) {
		string old_admin, old_password;
		if (!(in >> old_admin >> old_password))
			return false;
	}
	if (!(in >> nextRecordId) || nextRecordId < 1)
		return false;

	int count;
	if (!(in >> count) || count < 0) return false;
	for (int i = 0; i < count; ++i) {
		string username, name, studentId, className, balanceText;
		int lost;
		if (legacy) {
			string old_password;
			if (!(in >> username >> old_password >> name >> studentId >> className >> balanceText >> lost))
				return false;
		} else if (!(in >> username >> studentId) || in.get() != '\t' ||
			!getline(in, name, '\t') || !getline(in, className, '\t') ||
			!(in >> balanceText >> lost)) {
			return false;
		}
		username = decryptText(username);
		balanceText = decryptText(balanceText);
		long balance;
		istringstream value(balanceText);
		if (!(value >> balance) || balance < 0 || balance > MAX_MONEY || findUser(studentId) != -1)
			return false;
		users.push_back(Consumer(username, name, studentId, className, balance, lost == 1));
	}

	if (!(in >> count) || count < 0) return false;
	for (int i = 0; i < count; ++i) {
		long id, amount, balance;
		int type, place;
		string username, studentId, name, time, amountText, balanceText;
		if (!(in >> id >> username >> studentId))
			return false;
		if (legacy) {
			if (!(in >> name))
				return false;
		} else if (in.get() != '\t' || !getline(in, name, '\t')) {
			return false;
		}
		if (!(in >> type >> place >> amountText >> balanceText >> time))
			return false;
		username = decryptText(username);
		amountText = decryptText(amountText);
		balanceText = decryptText(balanceText);
		istringstream values(amountText + " " + balanceText);
		if (!(values >> amount >> balance)) return false;
		records.push_back(Record(id, username, studentId, name, type, place, amount, balance, time));
	}

	string end;
	return (in >> end) && end == "END";
}

// 账本里的用户名和金额用字符替换格式保存。
bool CampusSystem::save() {
	const string temp_path = string(DATA_FILE) + ".tmp";
	ofstream out(temp_path);
	out << "CAMPUS_PAY_SUB_V2\n" << nextRecordId << '\n' << users.size() << '\n';
	for (const Consumer& card : users) {
		out << encryptText(card.getUsername()) << '\t' << card.getStudentId() << '\t'
			<< card.getName() << '\t' << card.getClassName() << '\t'
			<< encryptText(to_string(card.getBalance())) << '\t' << card.isLost() << '\n';
	}
	out << records.size() << '\n';
	for (size_t i = 0; i < records.size(); ++i) records[i].write(out);
	out << "END\n";
	out.close();
	if (out && MoveFileExA(temp_path.c_str(), DATA_FILE,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		return true;
	std::remove(temp_path.c_str());
	cerr << "校园卡账本保存失败。\n";
	return false;
}
// 使用平台会话确定操作身份。管理员可按学号开户，学生只查看和使用本人的卡。
void CampusSystem::run(const AcademicManager& academic, const string& student_id, bool admin) {
	if (!load()) {
		cerr << "校园卡账本读取失败。\n";
		return;
	}
	if (!checkLedger(false))
		securityLog.write("system", "账目异常");

	if (admin) {
		adminMenu(academic);
		return;
	}
	int index = findUser(student_id);
	if (index == -1)
		cout << "尚未开通校园卡，请联系消费管理员按学号开卡。\n";
	else
		userMenu(index);
}

// academic 提供学籍资料。管理端按开卡、单卡业务和全量账务组织入口。
void CampusSystem::adminMenu(const AcademicManager& academic) {
	while (true) {
		cout << "\n---------- 消费管理 ----------\n"
			 << "1. 按学号开卡\n2. 按学号管理校园卡\n3. 账务管理\n0. 返回平台\n";
		int choice = readChoice("请选择：", 0, 3);
		if (choice <= 0) return;
		switch (choice) {
			case 1: addUser(academic); break;
			case 2: manageCard(); break;
			case 3: accountingMenu(); break;
		}
	}
}

// index是本次登录账户的下标。普通用户始终围绕这个账户操作，不能另选别人。
void CampusSystem::userMenu(int index) {
	while (true) {
		cout << "\n---------- 普通用户菜单（" << users[index].getName() << "）----------\n"
			 << "1. 账户信息与余额\n2. 消费\n3. 我的流水\n4. 账户挂失\n"
			 << "0. 返回平台\n";
		int choice = readChoice("请选择：", 0, 4);
		if (choice <= 0) return;
		switch (choice) {
			case 1: users[index].showInfo(); break;
			case 2: consume(index); break;
			case 3: showRecords(users[index].getStudentId()); break;
			case 4: changeLost(index, false); break;
		}
	}
}

// academic 提供在校学生资料。开卡只建立余额为零的业务记录，保存失败时撤销。
void CampusSystem::addUser(const AcademicManager& academic) {
	string student_id;
	cout << "学号：";
	if (!getline(cin, student_id))
		return;

	const Student* student = academic.getStudent(student_id);
	if (student == nullptr || !student->isInSchool()) {
		cout << "未找到在校学生。\n";
		return;
	}
	if (idUsed(student_id)) {
		cout << "该学号已有校园卡或历史流水。\n";
		return;
	}

	users.push_back(Consumer(student_id, student->getName(), student_id, student->getClassName()));
	if (save())
		cout << "开卡成功，初始余额为 0.00 元。\n";
	else
		users.pop_back();
}

// 学号确定本次操作的校园卡。先展示资料，再允许充值、处理挂失和查询流水。
// 旧账本中已注销的卡仍可按原学号查看历史流水。
void CampusSystem::manageCard() {
	string student_id;
	cout << "学号（留空返回）：";
	if (!getline(cin, student_id) || student_id.empty())
		return;

	int index = findUser(student_id);
	if (index == -1) {
		cout << "校园卡不存在。\n";
		if (idUsed(student_id))
			showRecords(student_id);
		return;
	}

	while (true) {
		cout << "\n---------- 校园卡详情 ----------\n";
		const CardHolder& card = users[index];
		card.showInfo();
		cout << "1. 为该卡充值\n2. 挂失 / 解挂\n3. 查看该卡流水\n0. 返回\n";
		int choice = readChoice("请选择：", 0, 3);
		if (choice <= 0) return;
		switch (choice) {
			case 1: recharge(index); break;
			case 2: changeLost(index, true); break;
			case 3: showRecords(student_id); break;
		}
	}
}

// index 是管理员已确认的校园卡下标。读取金额后交给 trade 记账。
void CampusSystem::recharge(int index) {
	long amount = readMoney("充值金额（元）：");
	if (amount <= 0) return;
	trade(index, 1, 0, amount);
}

// 普通用户可以挂失本人，管理员可以挂失或解挂选中的账户。
void CampusSystem::changeLost(int index, bool admin) {
	int choice = 1;
	if (admin) choice = readChoice("1. 挂失  2. 解挂  0. 取消：", 0, 2);
	else if (readChoice("确认挂失？1 是，0 取消：", 0, 1) != 1) return;
	if (choice <= 0) return;
	bool value = choice == 1;
	bool old = users[index].isLost();
	users[index].setLost(value);
	if (!save()) users[index].setLost(old);
	else {
		cout << (value ? "挂失成功。\n" : "解挂成功。\n");
	}
}

// 只处理本人消费的场景和金额输入，扣款和生成流水统一交给trade。
void CampusSystem::consume(int index) {
	int place = readChoice("消费场景：1 食堂，2 超市，3 浴室，4 其他，0 取消：", 0, 4);
	if (place <= 0) return;
	long amount = readMoney("消费金额（元）：");
	if (amount > 0) trade(index, 2, place, amount);
}

// index为账户下标，type为1充值或2消费，place为场景，amount的单位是分。
// 余额和流水一起保存，写入失败时恢复这次内存改动。
void CampusSystem::trade(int index, int type, int place, long amount) {
	Consumer old = users[index];
	bool ok = type == 1 ? users[index].recharge(amount) : users[index].consume(amount);
	if (!ok) {
		if (type == 1) cout << "充值失败，余额不能超过 1000000 元。\n";
		else cout << "消费失败，余额不足或账户已挂失。\n";
		return;
	}
	const Consumer& u = users[index];
	records.push_back(Record(nextRecordId, u.getUsername(), u.getStudentId(), u.getName(),
							 type, place, amount, u.getBalance(), currentTime()));
	++nextRecordId;
	// 余额、流水和下一编号一起回退，不能只恢复余额。
	if (!save()) {
		users[index] = old;
		records.pop_back();
		--nextRecordId;
	} else {
		if (type == 1) cout << "充值成功";
		else cout << "消费成功";
		cout << "，当前余额：" << moneyText(users[index].getBalance()) << " 元。\n";
	}
}

// 管理员可以查询全部流水，普通用户菜单只会传入本人的学号。
void CampusSystem::showRecords(const string& student_id) {
	cout << "流水号 | 时间 | 账户标识 | 学号 | 姓名 | 类型 | 场景 | 金额(元) | 余额(元)\n";
	int count = 0;
	for (size_t i = 0; i < records.size(); ++i) {
		if (student_id.empty() || records[i].getStudentId() == student_id) {
			records[i].show();
			++count;
		}
	}
	cout << "共 " << count << " 条流水。\n";
}

static bool validMonth(const string& month) {
	if (month.size() != 7 || month[4] != '-')
		return false;
	string year = month.substr(0, 4);
	string number = month.substr(5, 2);
	const string digits = "0123456789";
	return year != "0000" && year.find_first_not_of(digits) == string::npos &&
		number.find_first_not_of(digits) == string::npos && number >= "01" && number <= "12";
}

// 按月份或全部历史统计充值、消费及场景合计，当前总余额另行计算。
void CampusSystem::statistics() {
	string month;
	while (true) {
		cout << "统计月份（YYYY-MM，输入 0 统计全部）：";
		if (!getline(cin, month)) return;
		if (month == "0" || validMonth(month)) break;
		cout << "月份无效，请按 YYYY-MM 格式输入，月份范围为 01 至 12。\n";
	}
	long long rechargeTotal = 0, consumeTotal = 0, places[5] = {0};
	long long balances = 0;
	int rechargeCount = 0, consumeCount = 0;
	for (size_t i = 0; i < records.size(); ++i) {
		const Record& r = records[i];
		if (month != "0" && r.getTime().substr(0, 7) != month) continue;
		if (r.getType() == 1) {
			rechargeTotal += r.getAmount();
			++rechargeCount;
		} else if (r.getType() == 2) {
			consumeTotal += r.getAmount();
			int place = r.getPlace();
			if (place >= 1 && place <= 4) places[place] += r.getAmount();
			++consumeCount;
		}
	}
	for (size_t i = 0; i < users.size(); ++i) balances += users[i].getBalance();
	cout << "统计范围：" << (month == "0" ? "全部历史" : month) << endl;
	cout << "充值：" << rechargeCount << " 笔，合计 " << moneyText(rechargeTotal) << " 元\n";
	cout << "消费：" << consumeCount << " 笔，合计 " << moneyText(consumeTotal) << " 元\n";
	for (int i = 1; i <= 4; ++i)
		cout << placeName(i) << "消费：" << moneyText(places[i]) << " 元\n";
	cout << "当前全部账户余额（不受月份筛选影响）：" << moneyText(balances) << " 元\n";
}

// 启动时只取最后 10 条做对账，管理员可以手动核对全部。
bool CampusSystem::checkLedger(bool checkAll) const {
	size_t start = !checkAll && records.size() > 10 ? records.size() - 10 : 0;
	map<string, long long> balances;

	for (size_t i = start; i < records.size(); ++i) {
		const Record& record = records[i];
		string student_id = record.getStudentId();

		// 最近 10 条里第一次遇到这个账号时，取窗口之前最后一笔的余额作为起点。
		if (balances.count(student_id) == 0) {
			balances[student_id] = 0;
			for (size_t j = start; j > 0; --j) {
				if (records[j - 1].getStudentId() == student_id) {
					balances[student_id] = records[j - 1].getBalanceAfter();
					break;
				}
			}
		}

		if (record.getType() == 1) balances[student_id] += record.getAmount();
		else balances[student_id] -= record.getAmount();

		if (balances[student_id] != record.getBalanceAfter()) {
			cout << "账目异常，流水金额与交易后余额不一致：" << student_id << '\n';
			return false;
		}
	}

	for (const Consumer& user : users) {
		string student_id = user.getStudentId();
		if (!checkAll && balances.count(student_id) == 0) continue;
		if (balances[student_id] != user.getBalance()) {
			cout << "账目异常，账户余额与流水不一致：" << student_id << '\n';
			return false;
		}
	}
	return true;
}

// 集中处理跨校园卡的流水、统计和安全日志。全量核对的结果写入安全日志。
void CampusSystem::accountingMenu() {
	while (true) {
		cout << "\n---------- 账务管理 ----------\n"
			 << "1. 查看全部流水\n2. 消费统计\n3. 查看安全日志\n4. 检查全部流水\n0. 返回\n";
		int choice = readChoice("请选择：", 0, 4);
		if (choice <= 0) return;
		switch (choice) {
			case 1: showRecords(""); break;
			case 2: statistics(); break;
			case 3: securityLog.show(); break;
			case 4: {
				bool ok = checkLedger(true);
				if (ok) cout << "账目检查通过。\n";
				securityLog.write("system", ok ? "全量对账通过" : "账目异常");
				break;
			}
		}
	}
}
