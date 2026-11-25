// 账户对象的实现。这里负责单个账户的资料显示、充值和扣款。
#include "User.h"
#include "Input.h"
#include <iostream>
using namespace std;

// username、name分别是账户名和姓名，初始化所有账户共有的资料。
CardHolder::CardHolder(const string& username, const string& name)
	: username(username), name(name) {}
// 基类使用虚析构函数，通过基类指针释放派生对象时也能正确析构。
CardHolder::~CardHolder() {}

// 下面这些函数只读取或设置对应资料。
string CardHolder::getUsername() const { return username; }
string CardHolder::getName() const { return name; }
void CardHolder::setName(const string& value) { name = value; }
// 显示所有用户共有的信息，声明成虚函数后，派生类可以补充自己的资料。
void CardHolder::showInfo() const {
	cout << "用户名：" << username << "  姓名：" << name << endl;
}

// 先初始化User部分，再保存studentId学号和className班级。
CardStudent::CardStudent(const string& username, const string& name,
				 const string& studentId, const string& className)
	: CardHolder(username, name), studentId(studentId), className(className) {}
// 学生层增加学号、班级的读取和班级修改接口。
string CardStudent::getStudentId() const { return studentId; }
string CardStudent::getClassName() const { return className; }
void CardStudent::setClassName(const string& value) { className = value; }
// 先显示父类资料，再显示学生特有的学号和班级。
void CardStudent::showInfo() const {
	CardHolder::showInfo();
	cout << "学号：" << studentId << "  班级：" << className << endl;
}

// 在学生资料的基础上保存balance余额和lost挂失状态，余额单位统一为分。
Consumer::Consumer(const string& username, const string& name,
				   const string& studentId, const string& className,
				   long balance, bool lost)
	: CardStudent(username, name, studentId, className),
	  balance(balance), lost(lost) {}
// 账户状态通过这些接口读取或设置，余额不能在类外直接改写。
long Consumer::getBalance() const { return balance; }
bool Consumer::isLost() const { return lost; }
void Consumer::setLost(bool value) { lost = value; }

// amount为充值金额，单位是分。检查正数和余额上限，成功才增加余额并返回true。
bool Consumer::recharge(long amount) {
	if (amount <= 0 || amount > MAX_MONEY - balance) return false;
	balance += amount;
	return true;
}
// amount为消费金额，单位是分。挂失、非正数或余额不足都返回false，不扣款。
bool Consumer::consume(long amount) {
	if (lost || amount <= 0 || amount > balance) return false;
	balance -= amount;
	return true;
}
// 显示完整消费账户。通过User引用调用时，也会执行这个派生类版本。
void Consumer::showInfo() const {
	CardStudent::showInfo();
	cout << "余额：" << moneyText(balance) << " 元  状态："
		 << (lost ? "挂失" : "正常") << endl;
}
