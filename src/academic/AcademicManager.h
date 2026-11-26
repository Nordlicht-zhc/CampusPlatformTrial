#ifndef ACADEMIC_MANAGER_H
#define ACADEMIC_MANAGER_H

#include "Person.h"
#include "Student.h"
#include "Teacher.h"
#include "Course.h"
#include "Score.h"
#include <vector>
#include <string>
using namespace std;

// 学业管理系统核心类，统一持有人员、课程、成绩三类业务数据
// 负责业务处理和文件的加载与保存，是子系统的调度中心
class AcademicManager {
private:
	vector<Person*> people;   // 人员多态容器，实际存放 Student 或 Teacher 对象
	vector<Course> courses;   // 课程容器
	vector<Score> scores;     // 成绩容器
	string dataDir;           // 数据文件目录
	bool dataReady;           // 三份学业文件都成功读取后才开放学业模块

	// 内部辅助函数，对外隐藏查找细节
	int findPersonIndex(const string& id) const;
	int findCourseIndex(const string& courseNo) const;
	double studentAverage(const string& id) const;       // 计算某学生的平均分
	string courseNameOf(const string& courseNo) const;   // 按课程号查课程名

	bool loadAll();
	void saveAll();
	void adminMenu();
	void studentMenu(const string& student_id);

	// 学生信息管理
	void addStudent();
	void queryStudent();
	void modifyStudent();
	void removeStudent();
	void listStudents();
	void sortStudents();

	// 教师信息管理
	void addTeacher();
	void listTeachers();

	// 课程管理
	void addCourse();
	void listCourses();

	// 成绩管理与统计
	void addScore();
	void queryScoreByStudent(const string& id);
	void queryScoreByCourse();
	void courseStatistics();
	void showRanking();

public:
	explicit AcademicManager(const string& dataDir);
	~AcademicManager();
	const Student* getStudent(const string& id) const;
	void run(const string& student_id, bool admin);
};

#endif
