#include "AcademicManager.h"
#include "../AccountManager.h"
#include "Utils.h"
#include <fstream>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <windows.h>
using namespace std;

// 确保数据目录存在，Windows 下调用 CreateDirectory，目录已存在时返回失败可忽略
static void ensureDir(const string& dir) {
	CreateDirectoryA(dir.c_str(), NULL);
}

// 逐行读取记录，保留行号以便定位损坏的数据。
template <class ParseLine>
static bool readRecords(const string& path, ParseLine parseLine) {
	errno = 0;
	ifstream in(path);
	if (!in) {
		if (errno == ENOENT)
			return true;
		cerr << "无法打开学业存档 " << path << "。\n";
		return false;
	}
	string line;
	size_t lineNumber = 0;
	while (getline(in, line)) {
		++lineNumber;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) continue;
		try {
			parseLine(line);
		} catch (const logic_error&) {
			cerr << "学业存档 " << path << " 第 " << lineNumber << " 行格式错误。\n";
			return false;
		}
	}
	if (in.bad() || !in.eof()) {
		cerr << "读取学业存档 " << path << " 第 " << lineNumber + 1 << " 行失败。\n";
		return false;
	}
	return true;
}

// 构造函数：记录数据目录并加载历史数据
AcademicManager::AcademicManager(const string& dataDir, AccountManager& accounts)
	: dataDir(dataDir), accounts(accounts) {
	ensureDir(dataDir);
	dataReady = loadAll();
}

// 析构函数：释放多态容器中动态申请的人员对象
AcademicManager::~AcademicManager() {
	for (size_t i = 0; i < people.size(); ++i) delete people[i];
}

// 按编号查找人员下标，找不到返回 -1
int AcademicManager::findPersonIndex(const string& id) const {
	for (size_t i = 0; i < people.size(); ++i)
		if (people[i]->getId() == id) return (int)i;
	return -1;
}

// courseNo 是课程编号，返回匹配课程的下标，未找到时返回 -1。
int AcademicManager::findCourseIndex(const string& courseNo) const {
	for (size_t i = 0; i < courses.size(); ++i)
		if (courses[i].getCourseNo() == courseNo) return (int)i;
	return -1;
}

// 计算某学生所有已记录课程的平均分，无成绩时返回 0
double AcademicManager::studentAverage(const string& id) const {
	double sum = 0;
	int count = 0;
	for (size_t i = 0; i < scores.size(); ++i) {
		if (scores[i].getStudentId() == id) {
			sum += scores[i].getScore();
			++count;
		}
	}
	return count == 0 ? 0 : sum / count;
}

// 通过课程号换取课程名称，找不到时返回原编号
string AcademicManager::courseNameOf(const string& courseNo) const {
	int idx = findCourseIndex(courseNo);
	return idx == -1 ? courseNo : courses[idx].getCourseName();
}

// 加载人员、课程和成绩，按类型标记创建对应子类
bool AcademicManager::loadAll() {
	const bool peopleReady = readRecords(dataDir + "/people.dat", [this](const string& line) {
		if (line[0] == 'S')
			people.push_back(new Student(Student::parse(line)));
		else if (line[0] == 'T')
			people.push_back(new Teacher(Teacher::parse(line)));
		else
			throw invalid_argument("invalid person type");
	});
	if (!peopleReady) {
		for (Person* person : people)
			delete person;
		people.clear();
	}

	const bool coursesReady = readRecords(dataDir + "/courses.dat", [this](const string& line) {
		courses.push_back(Course::parse(line));
	});
	const bool scoresReady = readRecords(dataDir + "/scores.dat", [this](const string& line) {
		scores.push_back(Score::parse(line));
	});
	return peopleReady && coursesReady && scoresReady;
}

bool AcademicManager::writePeople(const string& path) const {
	ofstream out(path);
	for (const Person* person : people)
		out << person->serialize() << '\n';
	out.close();
	return static_cast<bool>(out);
}

// 开立平台账号前，先把学籍写入正式存档。
bool AcademicManager::savePeople() const {
	const string path = dataDir + "/people.dat";
	const string tempPath = path + ".tmp";
	if (writePeople(tempPath) && MoveFileExA(tempPath.c_str(), path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		return true;
	std::remove(tempPath.c_str());
	cerr << "学业存档 " << path << " 保存失败。\n";
	return false;
}

// 三份临时文件都写完后，再依次替换人员、课程和成绩存档。
void AcademicManager::saveAll() {
	const string paths[] = {dataDir + "/people.dat", dataDir + "/courses.dat", dataDir + "/scores.dat"};
	const string tempPaths[] = {paths[0] + ".tmp", paths[1] + ".tmp", paths[2] + ".tmp"};
	const bool peopleWritten = writePeople(tempPaths[0]);
	ofstream courseOut(tempPaths[1]);
	ofstream scoreOut(tempPaths[2]);
	for (const Course& course : courses)
		courseOut << course.serialize() << '\n';
	for (const Score& score : scores)
		scoreOut << score.serialize() << '\n';
	courseOut.close();
	scoreOut.close();

	if (!peopleWritten || !courseOut || !scoreOut) {
		const string& failed = !peopleWritten ? paths[0] : !courseOut ? paths[1] : paths[2];
		cerr << "学业存档 " << failed << " 写入失败。\n";
	} else {
		for (size_t i = 0; i < 3; ++i) {
			if (!MoveFileExA(tempPaths[i].c_str(), paths[i].c_str(),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
				cerr << "学业存档 " << paths[i] << " 替换失败。\n";
				break;
			}
		}
	}
	for (const string& path : tempPaths)
		std::remove(path.c_str());
}

// id 为学号。查到人员后检查其实际类型，学生和教师共用人员表。
const Student* AcademicManager::getStudent(const string& id) const {
	int index = findPersonIndex(id);
	if (index == -1)
		return nullptr;
	return dynamic_cast<const Student*>(people[index]);
}

// 学业管理员进入管理菜单。学生通过本人学号查询，输入结束时结束本次菜单。
void AcademicManager::run(const string& student_id, bool admin) {
	if (!dataReady) {
		cout << "学业存档读取失败，学业功能暂不可用。\n";
		return;
	}
	try {
		if (admin)
			adminMenu();
		else if (getStudent(student_id) != nullptr && getStudent(student_id)->isInSchool())
			studentMenu(student_id);
	} catch (const ios_base::failure&) {
	}
	if (admin)
		saveAll();
}

// 录入学籍后建立学生账号。已有学籍可在同一入口补建账号。
void AcademicManager::addStudent() {
	string id = readLine("学号：");
	if (id.empty() || id == "-" || id.size() > 64 || id.find_first_of("| \t\r\n\v\f") != string::npos) {
		cout << "请输入有效学号。" << endl;
		return;
	}
	int index = findPersonIndex(id);
	if (index != -1) {
		const Student* student = dynamic_cast<const Student*>(people[index]);
		if (student != nullptr && student->isInSchool())
			accounts.ensureStudentAccount(id);
		else
			cout << "该编号已使用，请核对学号。" << endl;
		return;
	}
	string name = readLine("姓名：");
	string gender = readLine("性别：");
	int age = readInt("年龄：");
	string phone = readLine("联系电话：");
	string major = readLine("专业：");
	string className = readLine("班级：");
	int grade = readInt("入学年级：");
	people.push_back(new Student(id, name, gender, age, phone,
								 major, className, grade, true));
	if (!savePeople()) {
		delete people.back();
		people.pop_back();
		return;
	}
	cout << "学生信息录入成功。" << endl;
	if (accounts.ensureStudentAccount(id) == StudentAccountStatus::Failed)
		cout << "学生资料已保存。可再次录入原学号补建账号。" << endl;
}

// 查询学生：支持按学号精确查询或按姓名模糊查询
void AcademicManager::queryStudent() {
	int way = readInt("查询方式：1 按学号  2 按姓名，请选择：");
	if (way == 1) {
		string id = readLine("请输入学号：");
		int idx = findPersonIndex(id);
		if (idx == -1) {
			cout << "未找到该学生。" << endl;
			return;
		}
		Student* s = dynamic_cast<Student*>(people[idx]);
		if (s == NULL) {
			cout << "该编号不是学生。" << endl;
			return;
		}
		s->showInfo();
		queryScoreByStudent(id);
	} else {
		string name = readLine("请输入姓名关键字：");
		vector<const Student*> matches;
		for (const Person* person : people) {
			const Student* student = dynamic_cast<const Student*>(person);
			if (student != nullptr && student->getName().find(name) != string::npos)
				matches.push_back(student);
		}
		if (matches.empty())
			cout << "未找到匹配的学生。" << endl;
		else
			Student::showTable(matches);
	}
}

// 修改学生信息，按字段子菜单定点修改，避免整表重录
void AcademicManager::modifyStudent() {
	string id = readLine("请输入要修改的学生学号：");
	int idx = findPersonIndex(id);
	if (idx == -1) {
		cout << "未找到该学生。" << endl;
		return;
	}
	Student* s = dynamic_cast<Student*>(people[idx]);
	if (s == NULL) {
		cout << "该编号不是学生。" << endl;
		return;
	}
	cout << "当前信息：" << endl;
	s->showInfo();
	int field = readInt("修改项：1 姓名 2 年龄 3 电话 4 专业 5 班级：");
	switch (field) {
		case 1: s->setName(readLine("新姓名：")); break;
		case 2: s->setAge(readInt("新年龄：")); break;
		case 3: s->setPhone(readLine("新电话：")); break;
		case 4: s->setMajor(readLine("新专业：")); break;
		case 5: s->setClassName(readLine("新班级：")); break;
		default: cout << "选项无效。" << endl; return;
	}
	cout << "修改完成。" << endl;
}

// 办理退学时保留学号和业务记录，以便核对已有成绩、借阅和流水。
void AcademicManager::removeStudent() {
	string id = readLine("请输入学生学号：");
	int index = findPersonIndex(id);
	Student* student = index == -1 ? nullptr : dynamic_cast<Student*>(people[index]);
	if (student == nullptr) {
		cout << "未找到该学生。" << endl;
		return;
	}

	if (readLine("确认办理退学请输入 yes：") == "yes") {
		student->setInSchool(false);
		cout << "已办理退学，学籍状态标记为已退学。" << endl;
	}
}

// 显示学生列表，可选择全部或仅在校学生
void AcademicManager::listStudents() {
	int scope = readInt("显示范围：1 仅在校  2 全部(含退学)，请选择：");
	vector<const Student*> list;
	for (const Person* person : people) {
		const Student* student = dynamic_cast<const Student*>(person);
		if (student != nullptr && (scope != 1 || student->isInSchool()))
			list.push_back(student);
	}
	if (list.empty())
		cout << "暂无符合条件的学生记录。" << endl;
	else
		Student::showTable(list);
}

// 排序展示：先收集学生指针，再按所选关键字排序，不改动原始存储顺序
void AcademicManager::sortStudents() {
	vector<const Student*> list;
	for (size_t i = 0; i < people.size(); ++i) {
		Student* s = dynamic_cast<Student*>(people[i]);
		if (s != NULL && s->isInSchool()) list.push_back(s);
	}
	if (list.empty()) {
		cout << "暂无在校学生。" << endl;
		return;
	}
	int key = readInt("排序方式：1 按学号 2 按姓名 3 按平均分，请选择：");
	if (key == 1) {
		sort(list.begin(), list.end(),
			 [](const Student* a, const Student* b) { return a->getId() < b->getId(); });
	} else if (key == 2) {
		sort(list.begin(), list.end(),
			 [](const Student* a, const Student* b) { return a->getName() < b->getName(); });
	} else if (key == 3) {
		sort(list.begin(), list.end(), [this](const Student* a, const Student* b) {
			return studentAverage(a->getId()) > studentAverage(b->getId());
		});
	} else {
		cout << "选项无效。" << endl;
		return;
	}
	Student::showTable(list);
}

// 录入教师
void AcademicManager::addTeacher() {
	string id = readLine("工号：");
	if (findPersonIndex(id) != -1) {
		cout << "该工号已存在。" << endl;
		return;
	}
	string name = readLine("姓名：");
	string gender = readLine("性别：");
	int age = readInt("年龄：");
	string phone = readLine("电话：");
	string title = readLine("职称：");
	string college = readLine("院系：");
	people.push_back(new Teacher(id, name, gender, age, phone, title, college));
	cout << "教师信息录入成功。" << endl;
}

// 显示全部教师
void AcademicManager::listTeachers() {
	cout << "工号\t姓名\t性别\t年龄\t电话\t职称\t院系" << endl;
	bool any = false;
	for (size_t i = 0; i < people.size(); ++i) {
		Teacher* t = dynamic_cast<Teacher*>(people[i]);
		if (t != NULL) {
			t->showInfo();
			any = true;
		}
	}
	if (!any) cout << "暂无教师记录。" << endl;
}

// 录入课程，课程号查重
void AcademicManager::addCourse() {
	string no = readLine("课程编号：");
	if (findCourseIndex(no) != -1) {
		cout << "该课程编号已存在。" << endl;
		return;
	}
	string name = readLine("课程名称：");
	double credit = readDouble("学分：");
	int hours = readInt("学时：");
	courses.push_back(Course(no, name, credit, hours));
	cout << "课程录入成功。" << endl;
}

// 显示全部课程
void AcademicManager::listCourses() {
	cout << "课程编号\t课程名称\t学分\t学时" << endl;
	if (courses.empty())
		cout << "暂无课程记录。" << endl;
	for (size_t i = 0; i < courses.size(); ++i) courses[i].showInfo();
}

// 录入成绩，校验学生、课程是否存在以及成绩范围，重复录入则更新
void AcademicManager::addScore() {
	string sid = readLine("学号：");
	int personIdx = findPersonIndex(sid);
	if (personIdx == -1 || dynamic_cast<Student*>(people[personIdx]) == NULL) {
		cout << "学生不存在。" << endl;
		return;
	}
	string cno = readLine("课程编号：");
	if (findCourseIndex(cno) == -1) {
		cout << "课程不存在。" << endl;
		return;
	}
	double value = readDouble("成绩(0-100)：");
	if (value < 0 || value > 100) {
		cout << "成绩超出合法范围。" << endl;
		return;
	}
	string term = readLine("学期(如 2026-1)：");
	for (size_t i = 0; i < scores.size(); ++i) {
		if (scores[i].getStudentId() == sid && scores[i].getCourseNo() == cno &&
				scores[i].getTerm() == term) {
			scores[i].setScore(value);
			cout << "原成绩已更新。" << endl;
			return;
		}
	}
	scores.push_back(Score(sid, cno, term, value));
	cout << "成绩录入成功。" << endl;
}

// 查询某学生的全部成绩，并汇总平均分
void AcademicManager::queryScoreByStudent(const string& id) {
	cout << "课程编号\t课程名称\t学期\t成绩" << endl;
	int count = 0;
	double sum = 0;
	for (size_t i = 0; i < scores.size(); ++i) {
		if (scores[i].getStudentId() == id) {
			const Score& g = scores[i];
			cout << g.getCourseNo() << "\t" << courseNameOf(g.getCourseNo())
				 << "\t" << g.getTerm() << "\t" << g.getScore() << endl;
			sum += g.getScore();
			++count;
		}
	}
	if (count == 0) {
		cout << "暂无成绩记录。" << endl;
	} else {
		cout << "共 " << count << " 条成绩，平均分：" << sum / count << endl;
	}
}

// 按课程查询所有学生成绩
void AcademicManager::queryScoreByCourse() {
	string cno = readLine("请输入课程编号：");
	if (findCourseIndex(cno) == -1) {
		cout << "课程不存在。" << endl;
		return;
	}
	cout << "学号\t课程名称\t学期\t成绩" << endl;
	bool any = false;
	for (size_t i = 0; i < scores.size(); ++i) {
		if (scores[i].getCourseNo() == cno) {
			const Score& g = scores[i];
			cout << g.getStudentId() << "\t" << courseNameOf(cno) << "\t"
				 << g.getTerm() << "\t" << g.getScore() << endl;
			any = true;
		}
	}
	if (!any) cout << "该课程暂无成绩。" << endl;
}

// 课程成绩统计：人数、平均、最高、最低、及格率
void AcademicManager::courseStatistics() {
	string cno = readLine("请输入课程编号：");
	if (findCourseIndex(cno) == -1) {
		cout << "课程不存在。" << endl;
		return;
	}
	string term = readLine("统计学期（如 2026-1，留空统计全部学期）：");
	int count = 0, pass = 0;
	double sum = 0, maxV = -1, minV = 101;
	for (size_t i = 0; i < scores.size(); ++i) {
		if (scores[i].getCourseNo() == cno && (term.empty() || scores[i].getTerm() == term)) {
			double v = scores[i].getScore();
			sum += v;
			++count;
			if (v >= 60) ++pass;
			if (v > maxV) maxV = v;
			if (v < minV) minV = v;
		}
	}
	if (count == 0) {
		cout << "所选范围内暂无成绩数据。" << endl;
		return;
	}
	cout << "课程：" << courseNameOf(cno) << endl;
	cout << "统计学期：" << (term.empty() ? "全部" : term) << endl;
	cout << "成绩条数：" << count << endl;
	cout << "平均分：" << sum / count << endl;
	cout << "最高分：" << maxV << endl;
	cout << "最低分：" << minV << endl;
	cout << "及格率：" << (double)pass / count * 100 << "%" << endl;
}

// 学生成绩排名：在校学生按平均分从高到低输出名次
void AcademicManager::showRanking() {
	vector<Student*> list;
	for (size_t i = 0; i < people.size(); ++i) {
		Student* s = dynamic_cast<Student*>(people[i]);
		if (s != NULL && s->isInSchool()) list.push_back(s);
	}
	if (list.empty()) {
		cout << "暂无在校学生。" << endl;
		return;
	}
	sort(list.begin(), list.end(), [this](Student* a, Student* b) {
		return studentAverage(a->getId()) > studentAverage(b->getId());
	});
	cout << "名次\t学号\t姓名\t班级\t平均分" << endl;
	for (size_t i = 0; i < list.size(); ++i) {
		cout << (i + 1) << "\t" << list[i]->getId() << "\t"
			 << list[i]->getName() << "\t" << list[i]->getClassName() << "\t"
			 << studentAverage(list[i]->getId()) << endl;
	}
}

// 学生端菜单，按 student_id 查询本人资料和成绩
void AcademicManager::studentMenu(const string& student_id) {
	while (true) {
		cout << endl << "===== 学生学业管理子系统(学生端) =====" << endl;
		cout << "1 查看本人信息  2 查看本人成绩  0 返回平台" << endl;
		int choice = readInt("请选择：");
		if (choice == 0) {
			break;
		}
		switch (choice) {
			case 1: {
				const Student* student = getStudent(student_id);
				if (student != nullptr) {
					student->showInfo();
				} else {
					cout << "未关联有效学生信息。" << endl;
				}
				break;
			}
			case 2: queryScoreByStudent(student_id); break;
			default: cout << "选项无效。" << endl;
		}
	}
}

// 管理员端菜单，平铺全部管理功能
void AcademicManager::adminMenu() {
	while (true) {
		cout << endl << "===== 学生学业管理子系统(管理端) =====" << endl;
		cout << "1 录入学生/建立账号   2 查询学生   3 修改学生   4 办理退学" << endl;
		cout << "5 学生列表   6 排序展示   7 录入教师   8 教师列表" << endl;
		cout << "9 录入课程  10 课程列表  11 录入成绩  12 按课程查成绩" << endl;
		cout << "13 成绩统计 14 成绩排名" << endl;
		cout << "0 保存并返回平台" << endl;
		int choice = readInt("请选择：");
		switch (choice) {
			case 1: addStudent(); break;
			case 2: queryStudent(); break;
			case 3: modifyStudent(); break;
			case 4: removeStudent(); break;
			case 5: listStudents(); break;
			case 6: sortStudents(); break;
			case 7: addTeacher(); break;
			case 8: listTeachers(); break;
			case 9: addCourse(); break;
			case 10: listCourses(); break;
			case 11: addScore(); break;
			case 12: queryScoreByCourse(); break;
			case 13: courseStatistics(); break;
			case 14: showRanking(); break;
			case 0:
				return;
			default: cout << "选项无效，请重新输入。" << endl;
		}
	}
}
