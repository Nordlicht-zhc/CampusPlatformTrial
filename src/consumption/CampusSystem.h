// 系统管理类把账户、流水、登录状态和菜单操作放在一起协调。
#ifndef CAMPUS_SYSTEM_H
#define CAMPUS_SYSTEM_H
#include "User.h"
#include "Record.h"
#include "SecurityLog.h"
#include <vector>
#include <string>

class AcademicManager;

// 系统持有校园卡、流水和账本日志。平台会话提供当前学生和管理权限。
class CampusSystem {
private:
	// 当前账户与历史流水分开保存，注销账户时不删除已有流水。
	std::vector<Consumer> users;
	std::vector<Record> records;
	long nextRecordId;
	SecurityLog securityLog;

	bool load();
	bool save();
	bool checkLedger(bool checkAll) const;
	void securityMenu();
	int findUser(const std::string& student_id) const;
	bool idUsed(const std::string& studentId) const;
	int selectUser() const;
	void adminMenu(const AcademicManager& academic);
	void userMenu(int index);
	void addUser(const AcademicManager& academic);
	void listUsers();
	void queryUser();
	void recharge();
	void changeLost(int index, bool admin);
	void consume(int index);
	void trade(int index, int type, int place, long amount);
	void showRecords(const std::string& student_id);
	void queryRecords();
	void statistics();
public:
	// run是对外入口，其余业务作为内部成员，避免绕过菜单随意调用。
	CampusSystem();
	void run(const AcademicManager& academic, const std::string& student_id, bool admin);
};

#endif
