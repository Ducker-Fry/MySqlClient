#include <include/database/sqlite_driver.h>

// 初始化静态成员
std::unique_ptr<Driver> Driver::instance_ = nullptr;