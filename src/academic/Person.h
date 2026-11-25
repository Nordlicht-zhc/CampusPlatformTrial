#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <iostream>
using namespace std;

// 人员抽象基类，统一描述学生和教师的公共信息
// Student、Teacher 均由该类派生，借助纯虚函数实现运行时多态
class Person {
protected:
	string id;       // 学号或工号，作为人员的唯一标识
	string name;     // 姓名
	string gender;   // 性别
	int age;         // 年龄
	string phone;    // 联系电话

public:
	Person();
	Person(const string& id, const string& name, const string& gender,
		   int age, const string& phone);
	virtual ~Person();  // 虚析构函数，保证通过基类指针释放派生类对象时正确析构

	// 数据成员的访问接口，体现封装性，外部不直接操作内部数据
	string getId() const;
	string getName() const;
	string getGender() const;
	int getAge() const;
	string getPhone() const;
	void setId(const string& id);
	void setName(const string& name);
	void setGender(const string& gender);
	void setAge(int age);
	void setPhone(const string& phone);

	virtual void showInfo() const = 0;    // 纯虚函数：按角色打印完整信息
	virtual string role() const = 0;      // 纯虚函数：返回角色名称
	virtual string serialize() const = 0; // 纯虚函数：把对象序列化为一行文本，便于写文件
};

#endif
