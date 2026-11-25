// 流水的显示和文件输出。屏幕显示正常数据，写文件时才做字符替换。
#include "Record.h"
#include "Input.h"
#include <iostream>
using namespace std;

// 保存交易快照。id是流水号，type为1充值或2消费，amount和balanceAfter都按分记录。
// place表示消费场景，time表示交易时间；学号和姓名保留交易当时的值。
Record::Record(long id, const string& username, const string& studentId,
			   const string& name, int type, int place, long amount,
			   long balanceAfter, const string& time)
	: id(id), username(username), studentId(studentId), name(name), time(time),
	  type(type), place(place), amount(amount), balanceAfter(balanceAfter) {}

// 只提供流水字段的读取接口，日常业务不修改已生成的历史流水。
string Record::getUsername() const { return username; }
string Record::getStudentId() const { return studentId; }
string Record::getTime() const { return time; }
int Record::getType() const { return type; }
int Record::getPlace() const { return place; }
long Record::getAmount() const { return amount; }
long Record::getBalanceAfter() const { return balanceAfter; }

// 把场景编号转换成文字。0用于充值，1到4依次为食堂、超市、浴室和其他。
string placeName(int place) {
	const char* names[] = {"充值", "食堂", "超市", "浴室", "其他"};
	return place >= 0 && place <= 4 ? names[place] : "未知";
}

// 以便于阅读的形式显示流水，金额在这里从分转换成元。
void Record::show() const {
	cout << id << " | " << time << " | " << username << " | " << studentId
		 << " | " << name << " | " << (type == 1 ? "充值" : "消费")
		 << " | " << placeName(place) << " | " << moneyText(amount)
		 << " | " << moneyText(balanceAfter) << endl;
}

// 按固定字段顺序写入out。只替换用户名、交易金额和交易后余额，便于原样读回。
void Record::write(ostream& out) const {
	out << id << '\t' << encryptText(username) << '\t' << studentId << '\t' << name << '\t'
		<< type << '\t' << place << '\t' << encryptText(to_string(amount)) << '\t' << encryptText(to_string(balanceAfter))
		<< '\t' << time << '\n';
}
