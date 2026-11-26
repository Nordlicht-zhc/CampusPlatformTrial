#include "Score.h"
#include "Utils.h"

Score::Score() : score(0) {}

// studentId、courseNo 和 term 指定成绩归属，score 保存百分制成绩。
Score::Score(const string& studentId, const string& courseNo,
			 const string& term, double score)
	: studentId(studentId), courseNo(courseNo), term(term), score(score) {}

// 读取学号、课程号、学期和分数，用于查询、统计和排序。
string Score::getStudentId() const { return studentId; }
string Score::getCourseNo() const { return courseNo; }
string Score::getTerm() const { return term; }
double Score::getScore() const { return score; }

// 用参数更新单项成绩数据，重复录入时更新已有记录的学期和分数。
void Score::setStudentId(const string& studentId) { this->studentId = studentId; }
void Score::setCourseNo(const string& courseNo) { this->courseNo = courseNo; }
void Score::setTerm(const string& term) { this->term = term; }
void Score::setScore(double score) { this->score = score; }

// 按学号、课程、学期和分数显示一条成绩。
void Score::showInfo() const {
	cout << studentId << "\t" << courseNo << "\t" << term << "\t" << score << endl;
}

// 首字段 G 标识成绩记录
// 按固定顺序输出成绩字段，使学号和课程编号在重启后仍可用于关联。
string Score::serialize() const {
	return string("G|") + studentId + "|" + courseNo + "|" + term + "|" +
		   to_string(score);
}

// 将 line 拆成四个成绩字段并转换分数，恢复一条存档记录。
Score Score::parse(const string& line) {
	vector<string> f = parseFields(line, "G", 5);
	double value = parseDouble(f[4]);
	if (value < 0 || value > 100)
		throw invalid_argument("invalid score");
	return Score(f[1], f[2], f[3], value);
}
