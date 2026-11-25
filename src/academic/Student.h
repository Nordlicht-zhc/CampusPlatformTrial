#ifndef STUDENT_H
#define STUDENT_H

#include "Person.h"
#include <string>
using namespace std;

// 学生类，由 Person 公有派生，在通用人员信息之外扩展学籍相关属性
class Student : public Person {
private:
	string major;     // 所学专业
	string className; // 所在班级
	int grade;        // 入学年级
	bool inSchool;    // 学籍状态，true 表示在校，false 表示已退学注销

public:
	Student();
	Student(const string& id, const string& name, const string& gender,
			int age, const string& phone, const string& major,
			const string& className, int grade, bool inSchool = true);

	string getMajor() const;
	string getClassName() const;
	int getGrade() const;
	bool isInSchool() const;
	void setMajor(const string& major);
	void setClassName(const string& className);
	void setGrade(int grade);
	void setInSchool(bool inSchool);

	// 重写基类纯虚函数，完成学生特有的展示与序列化逻辑
	void showInfo() const override;
	string role() const override;
	string serialize() const override;

	// 从文件中的一行文本还原出学生对象，属于对象的反序列化
	static Student parse(const string& line);
};

#endif
