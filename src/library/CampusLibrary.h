#ifndef CAMPUS_LIBRARY_H
#define CAMPUS_LIBRARY_H

#include <string>

// 从统一平台进入图书模块。学生使用自己的学号，管理员权限由平台检查。
void runCampusLibrary(const std::string& student_id, const std::string& name, bool admin);

#endif
