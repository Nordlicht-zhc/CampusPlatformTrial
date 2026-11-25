#include "Course.h"
#include "Utils.h"

Course::Course() : credit(0), hours(0) {}

// 用 courseNo、courseName、credit 和 hours 初始化课程编号、名称、学分与学时。
Course::Course(const string& courseNo, const string& courseName,
			   double credit, int hours)
	: courseNo(courseNo), courseName(courseName), credit(credit), hours(hours) {}

// 依次提供编号、名称、学分和学时的读取接口，供成绩关联和显示使用。
string Course::getCourseNo() const { return courseNo; }
string Course::getCourseName() const { return courseName; }
double Course::getCredit() const { return credit; }
int Course::getHours() const { return hours; }

// 用参数更新单个课程字段，可在保留其余字段的情况下调整课程资料。
void Course::setCourseNo(const string& courseNo) { this->courseNo = courseNo; }
void Course::setCourseName(const string& courseName) { this->courseName = courseName; }
void Course::setCredit(double credit) { this->credit = credit; }
void Course::setHours(int hours) { this->hours = hours; }

// 把课程各字段按表头顺序显示成一行。
void Course::showInfo() const {
	cout << courseNo << "\t" << courseName << "\t" << credit << "\t" << hours << endl;
}

// 首字段 C 标识课程记录
// 按固定字段顺序序列化课程，保存时用竖线分隔。
string Course::serialize() const {
	return string("C|") + courseNo + "|" + courseName + "|" +
		   to_string(credit) + "|" + to_string(hours);
}

// 将 line 拆成课程字段，把学分和学时转为相应数值后构造课程。
Course Course::parse(const string& line) {
	vector<string> f = split(line, '|');
	return Course(f[1], f[2], stod(f[3]), stoi(f[4]));
}
