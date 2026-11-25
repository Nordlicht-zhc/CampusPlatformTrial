#ifndef SCORE_H
#define SCORE_H

#include <string>
#include <iostream>
using namespace std;

// 成绩类，记录某学生在某门课程、某个学期取得的成绩
class Score {
private:
	string studentId; // 学号
	string courseNo;  // 课程编号
	string term;      // 学期，如 2026-1
	double score;     // 百分制成绩

public:
	Score();
	Score(const string& studentId, const string& courseNo,
		  const string& term, double score);

	string getStudentId() const;
	string getCourseNo() const;
	string getTerm() const;
	double getScore() const;
	void setStudentId(const string& studentId);
	void setCourseNo(const string& courseNo);
	void setTerm(const string& term);
	void setScore(double score);

	void showInfo() const;
	string serialize() const;
	static Score parse(const string& line);
};

#endif
