#include "CampusLibrary.h"
#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<cerrno>
#include<cstdio>
#include<ctime>
#include<limits>
#include<unordered_set>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<windows.h>
using namespace std;
// 图书保存书号、书名、作者、出版社、分类以及总册数和可借册数。
// LibSys 持有 Book 集合，借出和归还时通过 Book 修改库存。
class Book {
private:
	string bookName;
	string id;
	string author;
	string publisher;
	string category;
	int total;
	int validNum;
public:
	Book(){
		bookName = "";
		id = "";
		author = "";
		publisher = "";
		category = "";
		total = 0;
		validNum = 0;
	}
	Book(const string&name,const string&id,const string&authorName,const string&publisherName,const string&type,int total):bookName(name),id(id),author(authorName),publisher(publisherName),category(type),total(total),validNum(total){}
	string getName()const {
		return bookName;
	}
	string getId()const {
		return id;
	}
	string getAuthor()const {
		return author;
	}
	string getPublisher()const {
		return publisher;
	}
	string getCategory()const {
		return category;
	}
	int getTotal()const {
		return total;
	}
	int getValid()const {
		return validNum;
	}
	void setName(const string& name) {
		bookName = name;
	}
	void setAuthor(const string& authorName) {
		author = authorName;
	}
	void setPublisher(const string& publisherName) {
		publisher = publisherName;
	}
	void setCategory(const string& type) {
		category = type;
	}
	void setTotal(int totalNum) {
		total = totalNum;
	}
	// 库存有余量时减一册，返回是否成功借出。
	bool borrowBook() {
		if (validNum > 0) {
			validNum--;
			return 1;
		}
		return 0;
	}
	// 归还时增加一册可借库存，达到总册数时返回失败。
	bool increaseValidCount() {
		if (validNum < total) {
			validNum++;
			return 1;
		}
		return 0;
	}
	void displayInfo()const {
		cout << "书名：" << bookName << endl;
		cout << "书号：" << id << endl;
		cout << "作者：" << author << endl;
		cout << "出版社：" << publisher << endl;
		cout << "分类：" << category << endl;
		cout << "总册数：" << total << endl;
		cout << "可借册数：" << validNum << endl;
	}
};
// 读者资料以学号为编号，记录姓名和借阅限额。current 由未归还流水恢复。
class Reader {
private:
	string id, name;
	int current, limit;
public:
	Reader(const string& id, const string& name, int limit = 5):
		id(id), name(name), current(0), limit(limit) {}

	string getId()const { return id; }
	string getName()const { return name; }
	int getMax()const { return limit; }

	// 未达到借阅上限时增加在借册数，并向调用者返回是否成功。
	bool addBorrow() {
		if (current >= limit)
			return false;
		current++;
		return true;
	}

	// 有在借图书时减少一册，避免册数减成负数。
	bool minusBorrow() {
		if (current <= 0)
			return false;
		current--;
		return true;
	}

	void displayInfo()const {
		cout << "读者编号：" << id << "  姓名：" << name
			<< "  在借册数：" << current << '/' << limit << '\n';
	}
};

// date 必须是 YYYY-MM-DD 格式。按月份和闰年规则检查日期范围。
static bool validDate(const string& date) {
	if (date.size() != 10 || date[4] != '-' || date[7] != '-')
		return false;
	for (size_t i = 0; i < date.size(); i++) {
		if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9'))
			return false;
	}
	const int year = stoi(date.substr(0, 4));
	const int month = stoi(date.substr(5, 2));
	const int day = stoi(date.substr(8, 2));
	if (year < 1 || month < 1 || month > 12 || day < 1)
		return false;
	const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
	const int limit = month_days[month - 1] + (month == 2 && leap ? 1 : 0);
	return day <= limit;
}

// 借书和还书使用同一个本地日期来源。
static bool getToday(tm& local, string& date) {
	const time_t now = time(nullptr);
	if (now == static_cast<time_t>(-1))
		return false;
	const tm* localDate = std::localtime(&now);
	if (localDate == nullptr)
		return false;
	local = *localDate;
	char text[11];
	if (strftime(text, sizeof(text), "%Y-%m-%d", &local) == 0)
		return false;
	date = text;
	return true;
}

// 借阅流水关联读者学号和书号，记录借出、到期、归还日期与流水号。
// 流水由 LibSys 持有，退学或注销平台账号后仍用于核查借阅历史。
class BorrowRecord {
private:
	string reader_id;
	string book_id;
	string borrow_date;
	string return_date;
	string due_date;
	string record_id;
public:
	BorrowRecord(const string& record,const string& reader,const string& book,const string& borrow,const string& due):reader_id(reader),book_id(book),borrow_date(borrow),return_date(""),due_date(due),record_id(record){}
	string getBorrowDate()const {
		return borrow_date;
	}
	string getReturnDate()const {
		return return_date;
	}
	string getDueDate()const {
		return due_date;
	}
	//检查归还
	bool isReturn()const {
		return (!return_date.empty());
	}
	//是否逾期
	// 用 due 所给的查询日期判断尚未归还的流水是否超过应还日。
	bool isDue(const string&due)const {
		return !isReturn() && due > due_date;
	}
	//标记归还
	// re 为归还日期，首次归还才写入，防止重复归还。
	bool markReturn(const string& re) {
		if (isReturn()||re.empty()) {
			return 0;
		}
		return_date = re;
		return 1;
	}
	string getReaderID()const {
		return reader_id;
	}
	string getBookID()const {
		return book_id;
	}
	string getRecordID()const {
		return record_id;
	}
	void display()const {
		cout << "借阅流水号：" << record_id << endl;
		cout << "读者编号：" << reader_id << endl;
		cout << "书号：" << book_id << endl;
		cout << "借出日期：" << borrow_date << endl;
		cout << "应还日期：" << due_date << endl;
		cout << "归还日期：" << (!return_date.empty() ? return_date : "未归还") << endl;
		if (isReturn())
			cout << "已归还" << endl;
	}
	// 将本条流水按固定字段顺序写到 out，返回写入状态。
	bool save(ostream& out)const {
		out << record_id << '\n'
			<< reader_id << '\n'
			<< book_id << '\n'
			<< borrow_date << '\n'
			<< due_date << '\n'
			<< return_date << '\n';
		return static_cast<bool> (out);
	}
	// 从 in 读入完整流水，成功后才替换输出参数 record。
	static bool load(istream& in, BorrowRecord& record) {
		string recordID,readerID,bookID;
		string borrowDate,dueDate,returnDate;
		if (!getline(in,recordID)||!getline(in,readerID)||!getline(in,bookID)||!getline(in,borrowDate)||!getline(in,dueDate)||!getline(in,returnDate))
			return 0;
		if (recordID.empty()||readerID.empty()||bookID.empty()||borrowDate.empty()||dueDate.empty())
			return 0;
		if (dueDate<borrowDate||(!returnDate.empty()&&returnDate<borrowDate))
			return 0;
		BorrowRecord tmp(recordID,readerID,bookID,borrowDate,dueDate);
		if(!returnDate.empty()&&!tmp.markReturn(returnDate))
			return 0;
		record=tmp;
		return 1;
	}
};
// 管理图书、读者资料和借阅流水，并统一处理借期、库存和读写存档。
enum class LoadStatus { Loaded, Missing, Error };

class LibSys {
private:
	int next_record=1;
	int loanDays = 14;
	vector<Book> books;
	vector<Reader> readers;
	vector<BorrowRecord> recs;
	// 按书号查找图书，找不到时返回空指针。
	Book* findBook(const string& bookid) {
		for (Book& book : books) {
			if (book.getId() == bookid)
				return &book;
		}
		return nullptr;
	}
	// 按读者学号查找借阅资料，找不到时返回空指针。
	Reader* findReader(const string& readerid) {
		for (Reader& reader : readers) {
			if (reader.getId() == readerid)
				return &reader;
		}
		return nullptr;
	}
	// 按流水号查找借阅记录，找不到时返回空指针。
	BorrowRecord* findRecord(const string& recordid) {
		for (BorrowRecord& record : recs) {
			if (record.getRecordID() == recordid)
				return &record;
		}
		return nullptr;
	}
	// 用自增序号生成 R 开头的流水号，序号耗尽时返回空串。
	string generateRecordID() {
		if (next_record == numeric_limits<int>::max())
			return "";
		return "R" + to_string(next_record++);
	}
	// 取本地日期为借出日，按当前借期计算到期日，成功后写到两个输出参数。
	bool calculateLoanDates(string& borrowDate, string& dueDate) const {
		tm due{};
		string start;
		if (!getToday(due, start))
			return false;

		char end[11];
		due.tm_mday += loanDays;
		due.tm_hour = 12;
		due.tm_min = 0;
		due.tm_sec = 0;
		due.tm_isdst = -1;
		if (mktime(&due) == static_cast<time_t>(-1) ||
			strftime(end, sizeof(end), "%Y-%m-%d", &due) == 0)
			return false;

		borrowDate = start;
		dueDate = end;
		return true;
	}
public:
	LibSys(){}

	enum { MaxLoanDays = 365 };

	int getLoanDays() const {
		return loanDays;
	}

	// text 是借期天数，只有合法范围内的整数才会更新设置。
	bool setLoanDays(const string& text) {
		int days;
		if (!parseNumber(text, days) || days < 1 || days > MaxLoanDays)
			return false;
		loanDays = days;
		return true;
	}
	// 已有读者沿用原借阅限额，新学生按学号建立读者资料。
	void addStudentReader(const string& student_id, const string& name) {
		if (findReader(student_id) == nullptr)
			readers.push_back(Reader(student_id, name));
	}
	void displayReader()const {
		for (auto it = readers.begin(); it < readers.end(); it++) {
			it->displayInfo();
		}
	}

	// readerid 为学号，bookid 为书号。检查库存和借阅上限，更新册数并记录 dueDate。
	bool borrowBook(const string& readerid, const string& bookid, string& dueDate) {
		Reader* reader = findReader(readerid);
		Book* book = findBook(bookid);
		if (reader == nullptr || book == nullptr)
			return 0;
		string borrowDate, calculatedDueDate;
		if (!calculateLoanDates(borrowDate, calculatedDueDate))
			return false;
		string record_id = generateRecordID();
		if (record_id.empty())
			return false;
		BorrowRecord record(record_id, readerid, bookid, borrowDate, calculatedDueDate);
		if (!book->borrowBook())
			return 0;
		if (!reader->addBorrow()) {
			book->increaseValidCount();
			return 0;
		}
		recs.push_back(record);
		dueDate = calculatedDueDate;
		return 1;
	}
	// 验证 reader_id 是流水 record_id 的持有人，再检查还书日期、恢复库存并标记归还。
	bool returnBook(const string& reader_id,
		const string& record_id,
		const string& return_date) {
		BorrowRecord* record = findRecord(record_id);

		if (record == nullptr ||
			record->getReaderID() != reader_id ||
			record->isReturn()) {
			return false;
		}

		if (!validDate(return_date) ||
			return_date < record->getBorrowDate()) {
			return false;
		}

		Reader* reader = findReader(reader_id);
		Book* book = findBook(record->getBookID());

		if (reader == nullptr || book == nullptr) {
			return false;
		}

		if (!reader->minusBorrow()) {
			return false;
		}

		if (!book->increaseValidCount()) {
			reader->addBorrow();
			return false;
		}

		if (!record->markReturn(return_date)) {
			book->borrowBook();
			reader->addBorrow();
			return false;
		}

		return true;
	}
	// 添加 book 前检查书号是否重复，返回是否添加成功。
	bool addBook(const Book& book) {
		if(findBook(book.getId())!=nullptr)
			return 0;
		books.push_back(book);
		return 1;
	}
	// 按 bookID 删除无未归还借阅的图书，保护现有流水与库存关系。
	bool deleteBook(const string& bookID) {
		Book* book = findBook(bookID);
		if (book == nullptr)
			return 0;
		for (const BorrowRecord& record : recs) {
			if (record.getBookID() == bookID && !record.isReturn())
				return 0;
		}
		for (auto it = books.begin(); it < books.end();it++) {
			if (&(*it) == book) {
				books.erase(it);
				return 1;
			}
		}
		return 0;
	}
	// 按 reader_id 显示该读者的全部历史借阅。
	void displayReaderRecords(const string& reader_id)const {
		bool found = 0;
		for (const BorrowRecord& record : recs) {
			if (record.getReaderID() == reader_id) {
				record.display();
				cout << "----------" << endl;
				found = 1;
			}
		}
		if (!found) {
			cout << "暂无相关借阅记录。" << endl;
		}
	}
	// 按 reader_id 显示该读者仍未归还的图书。
	void displayReturnRecord(const string& reader_id)const {
		bool found = 0;
		for (const BorrowRecord& record : recs) {
			if (!record.isReturn() && record.getReaderID() == reader_id) {
				record.display();
				cout << "----------" << endl;
				found = 1;
			}
		}
		if (!found)
			cout << "暂无相关借阅记录。" << endl;
	}
	// 按 curr_date 筛出已超过到期日且仍在借的流水。
	void displayOverdueRecord(const string& curr_date)const {
		bool found = 0;
		for (const BorrowRecord& record : recs) {
			if (record.isDue(curr_date)) {
				record.display();
				cout << "----------" << endl;
				found = 1;
			}
		}
		if (!found)
			cout << "暂无相关借阅记录。" << endl;
	}
private:
	// 逐字符检查并转换非负整数，计算前判断是否溢出。
	static bool parseNumber(const string& text, int& number) {
		if (text.empty())
			return false;

		number = 0;
		for (char ch : text) {
			if (ch < '0' || ch > '9')
				return false;
			const int digit = ch - '0';
			if (number > (numeric_limits<int>::max() - digit) / 10)
				return false;
			number = number * 10 + digit;
		}
		return true;
	}

	// 从 in 读取一整行非负整数，成功时写入 number。
	static bool readNumber(istream& in, int& number) {
		string line;
		return getline(in, line) && parseNumber(line, number);
	}

	// 将流水条数和每条借阅流水依次写到 out。
	bool writeRecords(ostream& out) const {
		out << recs.size() << '\n';
		for (const BorrowRecord& record : recs) {
			if (!record.save(out))
				return false;
		}
		return static_cast<bool>(out);
	}

	// 按记录条数从 in 读取流水，使用集合检查是否有重复流水号。
	static bool readRecords(istream& in, vector<BorrowRecord>& records) {
		int count;
		if (!readNumber(in, count))
			return false;

		unordered_set<string> ids;
		for (int i = 0; i < count; ++i) {
			BorrowRecord record("", "", "", "", "");
			if (!BorrowRecord::load(in, record) ||
				!ids.insert(record.getRecordID()).second)
				return false;
			records.push_back(record);
		}
		return in.peek() == char_traits<char>::eof() && !in.bad();
	}

	// 依次写入借期、馆藏、读者业务资料和借阅流水，V3 存档只包含图书业务字段。
	bool writeData(ostream& out) const {
		out << "CAMPUS_LIBRARY_V3\n" << loanDays << '\n' << books.size() << '\n';
		for (const Book& book : books) {
			out << book.getId() << '\n' << book.getName() << '\n'
				<< book.getAuthor() << '\n' << book.getPublisher() << '\n'
				<< book.getCategory() << '\n' << book.getTotal() << '\n';
		}

		out << readers.size() << '\n';
		for (const Reader& reader : readers) {
			out << reader.getId() << '\n' << reader.getName() << '\n'
				<< reader.getMax() << '\n';
		}

		return writeRecords(out);
	}

	// 检查版本并还原馆藏、读者和流水，旧存档只采用业务资料。
	bool readData(istream& in) {
		string header;
		int count;
		if (!getline(in, header))
			return false;
		const bool legacy = header != "CAMPUS_LIBRARY_V3";
		if (header == "CAMPUS_LIBRARY_V2" || !legacy) {
			string days;
			if (!getline(in, days) || !setLoanDays(days))
				return false;
		} else if (header != "CAMPUS_LIBRARY_V1") {
			return false;
		}
		if (!readNumber(in, count))
			return false;

		for (int i = 0; i < count; ++i) {
			string id, name, author, publisher, category;
			int total;
			if (!getline(in, id) || !getline(in, name) || !getline(in, author) ||
				!getline(in, publisher) || !getline(in, category) ||
				!readNumber(in, total) || id.empty() || name.empty() ||
				!addBook(Book(name, id, author, publisher, category, total)))
				return false;
		}

		if (!readNumber(in, count))
			return false;
		for (int i = 0; i < count; ++i) {
			string id, name, old_password;
			int limit;
			if (!getline(in, id) || !getline(in, name) ||
				(legacy && !getline(in, old_password)) || !readNumber(in, limit) ||
				id.empty() || name.empty() || findReader(id) != nullptr)
				return false;
			readers.push_back(Reader(id, name, limit));
		}

		// V1/V2 另有一段管理员资料。读取旧格式时跳过这些字段。
		if (legacy) {
			if (!readNumber(in, count))
				return false;
			string line;
			for (int i = 0; i < count; i++) {
				if (!getline(in, line) || !getline(in, line) || !getline(in, line))
					return false;
			}
		}

		if (!readRecords(in, recs))
			return false;
		return restoreBorrowingState();
	}

	// 扫描流水恢复可借库存、读者在借数和下一流水号。
	bool restoreBorrowingState() {
		for (const BorrowRecord& record : recs) {
			const string id = record.getRecordID();
			int number;
			if (id.size() < 2 || id.front() != 'R' || id[1] == '0' ||
				!parseNumber(id.substr(1), number) || number == numeric_limits<int>::max())
				return false;
			if (number >= next_record)
				next_record = number + 1;

			if (record.isReturn())
				continue;

			Reader* reader = findReader(record.getReaderID());
			Book* book = findBook(record.getBookID());
			if (reader == nullptr || book == nullptr ||
				!reader->addBorrow() || !book->borrowBook())
				return false;
		}
		return true;
	}

public:
	// 将所有图书数据写入 file_path 同目录的临时文件，再尝试替换正式存档。
	bool saveData(const string& file_path) const {
		const string temp_path = file_path + ".tmp";
		ofstream out(temp_path);
		if (!out)
			return false;

		const bool written = writeData(out);
		out.close();
		if (written && out) {
			if (MoveFileExA(temp_path.c_str(), file_path.c_str(),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				return true;
		}
		std::remove(temp_path.c_str());
		return false;
	}

	// 读取 file_path 到临时对象，解析成功后才交换进当前系统。
	LoadStatus loadData(const string& file_path) {
		errno = 0;
		ifstream in(file_path);
		if (!in)
			return errno == ENOENT ? LoadStatus::Missing : LoadStatus::Error;

		LibSys loaded;
		if (!loaded.readData(in))
			return LoadStatus::Error;

		books.swap(loaded.books);
		readers.swap(loaded.readers);
		recs.swap(loaded.recs);
		next_record = loaded.next_record;
		loanDays = loaded.loanDays;
		return LoadStatus::Loaded;
	}
};
// 新存档使用两本示例书，便于直接运行借还流程。
static void createSampleData(LibSys& library) {
	library.addBook(
		Book("C++ 程序设计", "B001", "张三", "清华大学出版社", "计算机科学", 5)
	);
	library.addBook(
		Book("数据结构", "B002", "李四", "高等教育出版社", "计算机科学", 3)
	);
}

// 学生按当天日期还书，管理端可以补录历史归还日期。
static bool returnFromMenu(LibSys& library, const string& id, bool admin) {
	string recordID, today;

	cout << "借阅流水号：";
	if (!getline(cin, recordID)) return false;

	tm local{};
	if (!getToday(local, today)) {
		cout << "无法读取今天的日期。\n";
		return true;
	}

	string returnDate = today;
	if (admin) {
		cout << "归还日期（YYYY-MM-DD，留空使用今天）：";
		if (!getline(cin, returnDate)) return false;
		if (returnDate.empty())
			returnDate = today;
		if (!validDate(returnDate) || returnDate > today) {
			cout << "请输入有效的归还日期，且日期不能晚于今天。\n";
			return true;
		}
	}

	cout << (library.returnBook(id, recordID, returnDate)
		? "归还成功。\n"
		: "归还失败，请核对流水号、归还状态和日期。\n");
	return true;
}

// 由平台鉴权后调用。管理菜单负责图书增删、读者查询、逾期和借期设置。
static void adminMenu(LibSys& library) {
	string choice;
	while (true) {
		cout << "\n========== 图书管理子系统（管理端） ==========\n";
		cout << "1. 录入图书\n";
		cout << "2. 删除图书\n";
		cout << "3. 查看读者列表\n";
		cout << "4. 查看逾期记录\n";
		cout << "5. 设置借阅期限（" << library.getLoanDays() << " 天）\n";
		cout << "6. 按读者编号办理归还\n";
		cout << "0. 返回平台\n";
		cout << "请选择：";

		if (!getline(cin, choice)) {
			return;
		}

		if (choice == "0") {
			break;
		}

		if (choice == "1") {
			string name, bookID, author, publisher, category;
			string totalText;

			cout << "书名：";
			if (!getline(cin, name)) return;

			cout << "书号：";
			if (!getline(cin, bookID)) return;

			cout << "作者：";
			if (!getline(cin, author)) return;

			cout << "出版社：";
			if (!getline(cin, publisher)) return;

			cout << "分类：";
			if (!getline(cin, category)) return;

			cout << "总册数：";
			if (!getline(cin, totalText)) return;

			//检查数量是否整数型
			int total = 0;
			bool valid = !totalText.empty();

			for (char ch : totalText) {
				if (ch < '0' || ch > '9') {
					valid = false;
					break;
				}
			}

			if (valid) {
				try {
					total = stoi(totalText);
				}
				catch (...) {
					valid = false;
				}
			}

			if (!valid || total <= 0 ||
				name.empty() || bookID.empty()) {
				cout << "请输入书名、书号和大于零的总册数。\n";
				continue;
			}

			Book book(
				name, bookID, author, publisher, category, total
			);

			cout << (library.addBook(book)
				? "图书录入成功。\n"
				: "该书号已存在。\n");
		}
		else if (choice == "2") {
			string bookID;

			cout << "要删除的图书书号：";
			if (!getline(cin, bookID)) return;

			cout << (library.deleteBook(bookID)
				? "图书删除成功。\n"
				: "删除失败，请确认图书存在且没有未归还的借阅。\n");
		}
		else if (choice == "3") {
			library.displayReader();
		}
		else if (choice == "4") {
			string currentDate;

			cout << "查询日期（如 2026-01-31）：";
			if (!getline(cin, currentDate)) return;

			if (validDate(currentDate))
				library.displayOverdueRecord(currentDate);
			else
				cout << "日期无效。\n";
		}
		else if (choice == "5") {
			string days;
			cout << "借阅期限（天，1～" << LibSys::MaxLoanDays << "）：";
			if (!getline(cin, days)) return;

			if (library.setLoanDays(days))
				cout << "借阅期限已设为 " << library.getLoanDays() << " 天。\n";
			else
				cout << "借阅期限无效，请输入 1～"
					<< LibSys::MaxLoanDays << " 之间的整数。\n";
		}
		else if (choice == "6") {
			string reader_id;
			cout << "读者编号：";
			if (!getline(cin, reader_id) || !returnFromMenu(library, reader_id, true))
				return;
		}
		else {
			cout << "选项无效，请重新输入。\n";
		}
	}
}

// id 是平台会话绑定的学号。借阅和归还始终使用此学号。
static void readerMenu(LibSys& library, const string& id) {
	string choice;
	while (true) {
		cout << "\n========== 图书管理子系统（学生端） ==========\n";
		cout << "1. 借书\n";
		cout << "2. 还书\n";
		cout << "3. 查看全部借阅记录\n";
		cout << "4. 查看未归还图书\n";
		cout << "0. 返回平台\n";
		cout << "请选择：";

		if (!getline(cin, choice)) {
			return;
		}

		if (choice == "0") {
			break;
		}

		if (choice == "1") {
			string bookID, dueDate;

			cout << "书号：";
			if (!getline(cin, bookID)) return;

			if (library.borrowBook(id, bookID, dueDate)) {
				cout << "借阅成功，应还日期：" << dueDate << '\n';
			} else {
				cout << "借阅失败，请核对书号、可借库存和个人借阅上限。\n";
			}
		}
		else if (choice == "2") {
			if (!returnFromMenu(library, id, false)) return;
		}
		else if (choice == "3") {
			library.displayReaderRecords(id);
		}
		else if (choice == "4") {
			library.displayReturnRecord(id);
		}
		else {
			cout << "选项无效，请重新输入。\n";
		}
	}
}

// admin 表示图书管理权限，student_id 和 name 来自平台已核验的学生资料。
void runCampusLibrary(const string& student_id, const string& name, bool admin) {
	const string data_file = "data/library/library_data.txt";
	LibSys library;
	const LoadStatus status = library.loadData(data_file);
	if (status == LoadStatus::Error) {
		cerr << "无法读取图书存档（" << data_file << "）。\n";
		return;
	}
	if (status == LoadStatus::Missing)
		createSampleData(library);

	if (admin)
		adminMenu(library);
	else if (!student_id.empty()) {
		library.addStudentReader(student_id, name);
		readerMenu(library, student_id);
	}

	if (!library.saveData(data_file))
		cerr << "无法保存图书存档（" << data_file << "）。\n";
}
