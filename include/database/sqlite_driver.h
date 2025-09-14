#pragma once
//模仿mysql的接口，实现sqlite的接口
#include <exception>
#include <string>



namespace sql
{
    namespace sqlite
    {
        //Forward Declaration
        class Driver;
        class Connection;
        class Statement;
        class PreparedStatement;
        class ResultSet;
        class ResultSetMetadata;

        enum class DataType
        {
            INT,
            FLOAT,
            VARCHAR,
            BOOLEAN,
            TEXT,
            DATETIME,
            UNKOWN
        };

        //exception class
        class SQLiteException : public std::exception
        {
        private:
            std::string errMsg_;   // 完整错误信息（包含上下文）
            int errorCode_;        // SQLite错误码（如SQLITE_ERROR、SQLITE_BUSY等）
            std::string fileName_; // 发生错误的源文件（可选，调试用）
            int lineNumber_;       // 发生错误的行号（可选，调试用）

        public:
            // 1. 基础构造：仅错误消息（适用于自定义错误）
            explicit SQLiteException(const std::string& errMsg)
                : errMsg_(errMsg), errorCode_(-1), fileName_(""), lineNumber_(-1)
            {
            }

            // 2. SQLite原生错误构造：包含错误码和SQLite返回的消息
            SQLiteException(int errcode, const std::string& errMsg)
                : errMsg_(errMsg), errorCode_(errcode), fileName_(""), lineNumber_(-1)
            {
            }

            // 3. 带位置信息的构造：包含文件名和行号（调试时定位错误位置）
            SQLiteException(int errcode, const std::string& errMsg, const std::string& fileName, int lineNumber)
                : errMsg_(errMsg), errorCode_(errcode), fileName_(fileName), lineNumber_(lineNumber)
            {
            }

            // 4. 从SQLite数据库句柄构造（最常用，自动提取错误码和消息）
            explicit SQLiteException(sqlite3* db, const std::string& fileName = "", int lineNumber = -1)
                : errorCode_(sqlite3_errcode(db)),
                errMsg_(sqlite3_errmsg(db)),
                fileName_(fileName),
                lineNumber_(lineNumber)
            {
            }

            // 重写what()方法：返回完整错误信息（C风格字符串）
            const char* what() const noexcept override
            {
                return errMsg_.c_str();
            }

            // 获取SQLite错误码（便于根据错误类型处理）
            int getErrorCode() const noexcept
            {
                return errorCode_;
            }

            // 获取错误发生的文件名（调试用）
            const std::string& getFileName() const noexcept
            {
                return fileName_;
            }

            // 获取错误发生的行号（调试用）
            int getLineNumber() const noexcept
            {
                return lineNumber_;
            }

            // 获取包含位置信息的详细错误描述（测试和日志用）
            std::string getDetailedMessage() const
            {
                std::string detail = "SQLite Error [" + std::to_string(errorCode_) + "]: " + errMsg_;
                if (!fileName_.empty() && lineNumber_ > 0)
                {
                    detail += " (at " + fileName_ + ":" + std::to_string(lineNumber_) + ")";
                }
                return detail;
            }
        };

        // Driver class (singleton)
        class Driver
        {
        private:
            // 单例：私有构造函数 + 静态实例
            Driver() = default;
            Driver(const Driver&) = delete;               // 禁止拷贝构造
            Driver& operator=(const Driver&) = delete;    // 禁止赋值运算

            static std::unique_ptr<Driver> instance_;     // 单例实例（智能指针管理）

        public:
            // 获取单例实例（线程安全简化版）
            static Driver& getInstance()
            {
                if (!instance_)
                {
                    instance_ = std::unique_ptr<Driver>(new Driver());
                }
                return *instance_;
            }

            // 核心方法：创建数据库连接（返回智能指针，自动管理生命周期）
            // 参数：数据库URL（如"sqlite://test.db"）、用户名、密码（SQLite可能不需要后两者）
            std::unique_ptr<Connection> connect(
                const std::string& url,
                const std::string& user = "",
                const std::string& password = ""
            );
        };
    }
}