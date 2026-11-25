#ifndef TEACHER_H
#define TEACHER_H

#include "Person.h"
#include <string>
using namespace std;

// 教师类，同样由 Person 派生，扩展职称、所属院系等属性
// 与 Student 一起构成 Person 下的第二层类，用于演示同一接口的不同多态实现
class Teacher : public Person {
private:
	string title;    // 职称，如教授、讲师
	string college;  // 所属院系

public:
	Teacher();
	Teacher(const string& id, const string& name, const string& gender,
			int age, const string& phone, const string& title,
			const string& college);

	string getTitle() const;
	string getCollege() const;
	void setTitle(const string& title);
	void setCollege(const string& college);

	void showInfo() const override;
	string role() const override;
	string serialize() const override;

	static Teacher parse(const string& line);
};

#endif
