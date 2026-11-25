#include "Teacher.h"
#include "Utils.h"

Teacher::Teacher() {}

// 用 id、name、gender、age 和 phone 初始化 Person，title 和 college 为教师专属字段。
Teacher::Teacher(const string& id, const string& name, const string& gender,
				 int age, const string& phone, const string& title,
				 const string& college)
	: Person(id, name, gender, age, phone), title(title), college(college) {}

// 读取或设置教师的职称和学院。基础人员资料由 Person 管理。
string Teacher::getTitle() const { return title; }
string Teacher::getCollege() const { return college; }
void Teacher::setTitle(const string& title) { this->title = title; }
void Teacher::setCollege(const string& college) { this->college = college; }

// 按人员编号和教学资料的顺序显示完整教师记录。
void Teacher::showInfo() const {
	cout << id << "\t" << name << "\t" << gender << "\t" << age << "\t"
		 << phone << "\t" << title << "\t" << college << endl;
}

// 实现 Person 的角色接口，返回教师角色。
string Teacher::role() const { return "教师"; }

// 首字段 T 标识这是一条教师记录
// 在基础信息后写入职称与学院，保存一行教师记录。
string Teacher::serialize() const {
	return string("T|") + id + "|" + name + "|" + gender + "|" +
		   to_string(age) + "|" + phone + "|" + title + "|" + college;
}

// 将 line 按竖线拆分并检查字段数量，再还原教师对象。
Teacher Teacher::parse(const string& line) {
	vector<string> f = split(line, '|');
	Teacher t;
	t.setId(f[1]);
	t.setName(f[2]);
	t.setGender(f[3]);
	t.setAge(stoi(f[4]));
	t.setPhone(f[5]);
	t.setTitle(f[6]);
	t.setCollege(f[7]);
	return t;
}
