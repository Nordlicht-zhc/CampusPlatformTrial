#include "Student.h"
#include "Utils.h"

Student::Student() : grade(0), inSchool(true) {}

// 带参构造函数，先通过基类构造函数初始化继承来的公共成员，再初始化自身成员
Student::Student(const string& id, const string& name, const string& gender,
				 int age, const string& phone, const string& major,
				 const string& className, int grade, bool inSchool)
	: Person(id, name, gender, age, phone),
	  major(major), className(className), grade(grade), inSchool(inSchool) {}

// 读取学生的专业、班级、年级和在校状态，平台根据状态判断可开放的学生业务。
string Student::getMajor() const { return major; }
string Student::getClassName() const { return className; }
int Student::getGrade() const { return grade; }
bool Student::isInSchool() const { return inSchool; }

// 把传入的新值写入对应学籍字段，不动从 Person 继承的普通资料。
void Student::setMajor(const string& major) { this->major = major; }
void Student::setClassName(const string& className) { this->className = className; }
void Student::setGrade(int grade) { this->grade = grade; }
void Student::setInSchool(bool inSchool) { this->inSchool = inSchool; }

// 以表格字段形式输出学生的完整信息
void Student::showInfo() const {
	cout << id << "\t" << name << "\t" << gender << "\t" << age << "\t"
		 << phone << "\t" << major << "\t" << className << "\t" << grade
		 << "\t" << (inSchool ? "在校" : "已退学") << endl;
}

// 实现基类的角色接口，返回学生角色。
string Student::role() const { return "学生"; }

// 把学生各字段按统一分隔符拼成一行，首字段 S 用于标识对象类型
string Student::serialize() const {
	return string("S|") + id + "|" + name + "|" + gender + "|" +
		   to_string(age) + "|" + phone + "|" + major + "|" + className +
		   "|" + to_string(grade) + "|" + (inSchool ? "1" : "0");
}

// 反序列化：按分隔符切分后逐字段还原，stoi 完成字符串到数值的转换
Student Student::parse(const string& line) {
	vector<string> f = split(line, '|');
	Student s;
	s.setId(f[1]);
	s.setName(f[2]);
	s.setGender(f[3]);
	s.setAge(stoi(f[4]));
	s.setPhone(f[5]);
	s.setMajor(f[6]);
	s.setClassName(f[7]);
	s.setGrade(stoi(f[8]));
	s.setInSchool(f[9] == "1");
	return s;
}
