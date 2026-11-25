#ifndef COURSE_H
#define COURSE_H

#include <string>
#include <iostream>
using namespace std;

// 课程类，描述一门课程的基本信息，是成绩记录所关联的对象
class Course {
private:
	string courseNo;   // 课程编号
	string courseName; // 课程名称
	double credit;     // 学分
	int hours;         // 学时

public:
	Course();
	Course(const string& courseNo, const string& courseName,
		   double credit, int hours);

	string getCourseNo() const;
	string getCourseName() const;
	double getCredit() const;
	int getHours() const;
	void setCourseNo(const string& courseNo);
	void setCourseName(const string& courseName);
	void setCredit(double credit);
	void setHours(int hours);

	void showInfo() const;
	string serialize() const;
	static Course parse(const string& line);
};

#endif
