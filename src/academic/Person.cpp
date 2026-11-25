#include "Person.h"

// 默认构造函数，给出成员的初始值，避免出现未初始化的脏数据
Person::Person() : age(0) {}

// 带参构造函数，创建对象时一次性完成各属性赋值
Person::Person(const string& id, const string& name, const string& gender,
			   int age, const string& phone)
	: id(id), name(name), gender(gender), age(age), phone(phone) {}

// 虚析构函数，本类无动态资源，声明为虚函数是为了保证派生类析构链完整
Person::~Person() {}

// 读取人员编号、姓名、性别、年龄和电话，供查询和派生类显示使用。
string Person::getId() const { return id; }
string Person::getName() const { return name; }
string Person::getGender() const { return gender; }
int Person::getAge() const { return age; }
string Person::getPhone() const { return phone; }

// 用传入的新值更新一个基础字段，其余人员资料由同一对象继续保存。
void Person::setId(const string& id) { this->id = id; }
void Person::setName(const string& name) { this->name = name; }
void Person::setGender(const string& gender) { this->gender = gender; }
void Person::setAge(int age) { this->age = age; }
void Person::setPhone(const string& phone) { this->phone = phone; }
