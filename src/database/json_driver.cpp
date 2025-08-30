// json_driver_impl.cpp
#include <database/json_driver.h>
#include <fstream>
#include <filesystem>
#include <regex>

using namespace sql::jsondb;
namespace fs = std::filesystem;

std::shared_ptr<Connection> Driver::connect(const std::string& dbPath, std::string user, std::string passwd)
{
    auto conn = std::make_shared<Connection>(dbPath, user, passwd);
    return conn;
}


std::string sql::jsondb::Connection::getTableFilePath(const std::string& tableName) const
{
    auto path = fs::path(dbPath);
    path /= tableName + ".json";
    return path.string();
}

Connection::Connection(const std::string& dbPath, std::string user, std::string passwd)
{
    this->dbPath = dbPath;
    this->closed = false;
    this->autoCommit = true;
    
    // authenticate user
    if(!authenticate(user, passwd))
        throw JsonDbException("Authentication failed for user: " + user);

    // check if dbPath exists, if not create it
    auto path = fs::path(dbPath);
    if (!fs::exists(path))
    {
        fs::create_directories(path);
    }
    else if (!fs::is_directory(path))
    {
        throw JsonDbException("Database path is not a directory: " + dbPath);
    }
}

sql::jsondb::Connection::~Connection()
{
    try
    {
        if (!closed)
        {
            close();
        }
    }
    catch (const JsonDbException& e)
    {
        // handle exception
        throw e;
    }
}

std::shared_ptr<Statement> sql::jsondb::Connection::createStatement()
{
    return std::make_shared<Statement>(this);
}

std::shared_ptr<PreparedStatement> sql::jsondb::Connection::prepareStatement(const std::string& sql)
{
    return std::make_shared<PreparedStatement>(this, sql);
}

void sql::jsondb::Connection::commit()
{
    // Put commit logic here and handle exceptions
    // I'll complete this later if needed
}

void sql::jsondb::Connection::rollback()
{
    // Put rollback logic here and handle exceptions
    // I'll complete this later
}

bool sql::jsondb::Connection::authenticate(std::string user, std::string passwd)
{
    // Put authentication logic here and handle exceptions
    // I'll complete this later and use common configuration files. the user and passwd are stored in a specific file
    // return true to ensure the connection is established and program runs smoothly
    return true;
}

bool sql::jsondb::Connection::validateConnection() const
{
    // Put validation logic here and handle exceptions
    // I'll realize more complicated logic later
    if(closed) return false;
    return true;
}

bool sql::jsondb::Connection::tableExists(const std::string& tableName) const
{
    auto path = getTableFilePath(tableName);
    return fs::exists(path);
}

std::vector<std::string> sql::jsondb::Connection::getColumnNames(const std::string& tableName) const
{
    // 获取表文件路径
    std::string tablePath = getTableFilePath(tableName);
    
    // 检查文件是否存在
    if (!std::filesystem::exists(tablePath)) {
        throw JsonDbException("Table does not exist: " + tableName);
    }
    
    // 读取 JSON 文件
    std::ifstream file(tablePath);
    if (!file.is_open()) {
        throw JsonDbException("Failed to open table file: " + tablePath);
    }
    
    // 解析 JSON 数据
    nlohmann::json tableData;
    try {
        file >> tableData;
    } catch (const nlohmann::json::parse_error& e) {
        throw JsonDbException("Failed to parse JSON file: " + std::string(e.what()));
    }
    
    // 检查 JSON 是否是数组
    if (!tableData.is_array() || tableData.empty()) {
        throw JsonDbException("Invalid table format: table data should be a non-empty array");
    }
    
    // 获取第一行的所有键作为列名
    std::vector<std::string> columns;
    for (auto it = tableData[0].begin(); it != tableData[0].end(); ++it) {
        columns.push_back(it.key());
    }
    
    return columns;
}

std::shared_ptr<ResultSet> sql::jsondb::Statement::executeQuery(const std::string& sql)
{
    // parse sql statement and use std::regex to match the pattern
    std::regex selectPattern(R"(SELECT\s+(.*?)\s+FROM\s+(.*?)\s+WHERE\s+(.*?)\s+)", std::regex::icase);

    std::smatch matches;
    //handle exception
    if (!std::regex_match(sql, matches, selectPattern))
    {
        throw JsonDbException("Invalid SQL statement: " + sql);
    }

    // execute query and return result set
    // extract columns , table and condition from the matches
    std::string columns = matches[1];
    std::string table = matches[2];
    std::string condition = matches[3];

    //extract columns
    std::vector<std::string> columnNames;
    if (columns == "*")
    {
        columnNames = connection->getColumnNames(table);
    }
    else
    {
        std::regex columnPattern(R"((\w+)(?:\s*,\s*(\w+))*)");
        std::sregex_iterator bgn = std::sregex_iterator(columns.begin(), columns.end(), columnPattern);
        std::regex_iterator end = std::sregex_iterator();
        for (std::regex_iterator it = bgn; it != end; ++it)
        {
            columnNames.push_back((*it)[1].str());
        }
    }

    //extract table
    if (table == "")
    {
        throw JsonDbException("Table name is required in SELECT statement");
    }
    else
    {
        // 优化后正则：允许前后空格，匹配最后一个点后的表名
        std::regex tablePattern(R"(\s*.*\.?(\w+)\s*)");
        std::smatch tableMatch;

        // 用 regex_match 确保全字符串匹配（避免部分匹配问题）
        if (!std::regex_match(table, tableMatch, tablePattern))
        {
            throw JsonDbException("Invalid table name: " + table);
        }
        else
        {
            table = tableMatch[1].str();
        }
    }

}
