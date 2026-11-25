// 一条流水保存一次交易发生时的信息，和当前账户资料分开管理。
#ifndef RECORD_H
#define RECORD_H
#include <string>
#include <ostream>

// 与Consumer没有继承关系。一条Record只描述一次已经完成的交易。
class Record {
private:
	long id;
	std::string username, studentId, name, time;
	// type：1充值、2消费；place：0充值、1食堂、2超市、3浴室、4其他。
	int type, place;
	// amount是本次交易金额，balanceAfter是本次交易完成后的余额，单位都是分。
	long amount, balanceAfter;
public:
	Record(long id, const std::string& username, const std::string& studentId,
		   const std::string& name, int type, int place, long amount,
		   long balanceAfter, const std::string& time);
	std::string getUsername() const;
	std::string getStudentId() const;
	std::string getTime() const;
	int getType() const;
	int getPlace() const;
	long getAmount() const;
	long getBalanceAfter() const;
	void show() const;
	void write(std::ostream& out) const;
};
std::string placeName(int place);
#endif
