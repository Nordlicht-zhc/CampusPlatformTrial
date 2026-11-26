#include "Student.h"
#include "Utils.h"
#include <array>
#include <windows.h>

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

// 中文和全角字符通常占两列，不能直接用 UTF-8 字节数对齐。
static size_t displayWidth(const string& text) {
	if (text.empty())
		return 0;
	const int bytes = static_cast<int>(text.size());
	const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), bytes, nullptr, 0);
	if (count == 0)
		return text.size();
	wstring wide(count, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), bytes, &wide[0], count);

	vector<WORD> types(count);
	if (!GetStringTypeW(CT_CTYPE3, wide.data(), count, types.data()))
		return wide.size();
	size_t width = 0;
	for (WORD type : types) {
		if (type & C3_NONSPACING)
			continue;
		width += (type & (C3_FULLWIDTH | C3_IDEOGRAPH)) ? 2 : 1;
	}
	return width;
}

// 先按标题和当前结果确定每列宽度，再用相同的列宽输出整张表。
void Student::showTable(const vector<const Student*>& students) {
	using Row = array<string, 9>;
	const Row headings = {{"学号", "姓名", "性别", "年龄", "电话", "专业", "班级", "年级", "状态"}};
	vector<Row> rows;
	rows.push_back(headings);
	for (const Student* student : students) {
		rows.push_back({{student->id, student->name, student->gender, to_string(student->age),
			student->phone, student->major, student->className, to_string(student->grade),
			student->inSchool ? "在校" : "已退学"}});
	}

	array<size_t, 9> widths{};
	for (const Row& row : rows) {
		for (size_t column = 0; column < row.size(); ++column) {
			const size_t width = displayWidth(row[column]);
			if (width > widths[column])
				widths[column] = width;
		}
	}

	const size_t spacing = 2;
	size_t tableWidth = spacing * (widths.size() - 1);
	for (size_t width : widths)
		tableWidth += width;
	CONSOLE_SCREEN_BUFFER_INFO console;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console) &&
			tableWidth > static_cast<size_t>(console.srWindow.Right - console.srWindow.Left + 1)) {
		// 窗口放不下整行时按字段显示，避免数据换行后失去对应标题。
		for (size_t row = 1; row < rows.size(); ++row) {
			if (row > 1) cout << '\n';
			for (size_t column = 0; column < headings.size(); ++column)
				cout << headings[column] << "：" << rows[row][column] << '\n';
		}
		return;
	}

	for (const Row& row : rows) {
		for (size_t column = 0; column < row.size(); ++column) {
			cout << row[column];
			if (column + 1 < row.size())
				cout << string(widths[column] - displayWidth(row[column]) + spacing, ' ');
		}
		cout << '\n';
	}
}

void Student::showInfo() const {
	showTable({this});
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
	vector<string> f = parseFields(line, "S", 10);
	if (f[9] != "0" && f[9] != "1")
		throw invalid_argument("invalid student status");
	Student s;
	s.setId(f[1]);
	s.setName(f[2]);
	s.setGender(f[3]);
	s.setAge(parseInt(f[4]));
	s.setPhone(f[5]);
	s.setMajor(f[6]);
	s.setClassName(f[7]);
	s.setGrade(parseInt(f[8]));
	s.setInSchool(f[9] == "1");
	return s;
}
