// 账户类的定义：通用用户、学生、消费账户逐层增加资料和操作。
#ifndef CONSUMPTION_CARD_H
#define CONSUMPTION_CARD_H

#include <string>

// 基类保存用户名和姓名。showInfo为虚函数，派生类可以扩展显示内容。
// 校园卡持有人保存业务账户标识和姓名。CardStudent 继承并增加学号与班级，Consumer 再增加余额和挂失状态。
class CardHolder {
protected:
	std::string username;
	std::string name;
public:
	CardHolder(const std::string& username,
		 const std::string& name);
	// 通过基类指针销毁派生对象时，仍按正确顺序释放。
	virtual ~CardHolder();
	std::string getUsername() const;
	std::string getName() const;
	void setName(const std::string& value);
	virtual void showInfo() const;
};

// Student公有继承User，增加学号、班级，不重复保存父类已有资料。
class CardStudent : public CardHolder {
protected:
	std::string studentId;
	std::string className;
public:
	CardStudent(const std::string& username,
			const std::string& name, const std::string& studentId,
			const std::string& className);
	std::string getStudentId() const;
	std::string getClassName() const;
	void setClassName(const std::string& value);
	virtual void showInfo() const;
};

// Consumer公有继承Student，增加私有余额和挂失状态，形成三层类体系。
class Consumer : public CardStudent {
private:
	// balance按分保存；只允许通过充值和消费函数修改，防止类外随意赋值。
	long balance;
	bool lost;
public:
	Consumer(const std::string& username,
			 const std::string& name, const std::string& studentId,
			 const std::string& className, long balance = 0, bool lost = false);
	long getBalance() const;
	bool isLost() const;
	void setLost(bool value);
	bool recharge(long amount);
	bool consume(long amount);
	virtual void showInfo() const;
};

#endif
