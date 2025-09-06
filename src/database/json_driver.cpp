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


std::string Connection::getTableFilePath(const std::string& tableName) const
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

Connection::~Connection()
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

void Connection::close()
{
    if (!closed)
    {
        closed = true;
        // 可以在这里添加任何需要的资源清理代码
    }
}

std::shared_ptr<Statement> Connection::createStatement()
{
    return std::make_shared<Statement>(this);
}

std::shared_ptr<PreparedStatement> Connection::prepareStatement(const std::string& sql)
{
    return std::make_shared<PreparedStatement>(this, sql);
}

void Connection::commit()
{
    // Put commit logic here and handle exceptions
    // I'll complete this later if needed
}

void Connection::rollback()
{
    // Put rollback logic here and handle exceptions
    // I'll complete this later
}

bool Connection::authenticate(std::string user, std::string passwd)
{
    // Put authentication logic here and handle exceptions
    // I'll complete this later and use common configuration files. the user and passwd are stored in a specific file
    // return true to ensure the connection is established and program runs smoothly
    return true;
}

bool Connection::validateConnection() const
{
    // Put validation logic here and handle exceptions
    // I'll realize more complicated logic later
    if(closed) return false;
    return true;
}

bool Connection::tableExists(const std::string& tableName) const
{
    auto path = getTableFilePath(tableName);
    return fs::exists(path);
}

std::vector<std::string> Connection::getColumnNames(const std::string& tableName) const
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

std::shared_ptr<ResultSet> Statement::executeQuery(const std::string& sql)
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


// 主函数：判断SQL语句类型并调用相应的实现函数
size_t Statement::executeUpdate(const std::string& sql)
{
    // parse sql statement and use std::regex to match the pattern
    std::regex updatePattern(R"(UPDATE\s+(.*?)\s+SET\s+(.*?)\s+WHERE\s+(.*?)\s*(?:;)?$)", std::regex::icase);
    std::regex deletePattern(R"(DELETE\s+FROM\s+(.*?)\s+WHERE\s+(.*?)\s*(?:;)?$)", std::regex::icase);
    std::regex insertPattern(R"(INSERT\s+INTO\s+(.*?)\s+\((.*?)\)\s+VALUES\s+\((.*?)\)\s*(?:;)?$)", std::regex::icase);
    std::regex insertNoColumnsPattern(R"(INSERT\s+INTO\s+(.*?)\s+VALUES\s+\((.*?)\)\s*(?:;)?$)", std::regex::icase);

    std::smatch matches;
    size_t affectedRows = 0;

    // 判断SQL语句类型并调用相应的实现函数
    if (std::regex_match(sql, matches, updatePattern))
    {
        std::string table = extractTableName(matches[1]);
        std::string setClause = matches[2];
        std::string whereClause = matches[3];
        affectedRows = executeUpdateImpl(table, setClause, whereClause);
    }
    else if (std::regex_match(sql, matches, deletePattern))
    {
        std::string table = extractTableName(matches[1]);
        std::string whereClause = matches[2];
        affectedRows = executeDeleteImpl(table, whereClause);
    }
    else if (std::regex_match(sql, matches, insertPattern))
    {
        std::string table = extractTableName(matches[1]);
        std::string columns = matches[2];
        std::string values = matches[3];
        affectedRows = executeInsertWithColumns(table, columns, values);
    }
    else if (std::regex_match(sql, matches, insertNoColumnsPattern))
    {
        std::string table = extractTableName(matches[1]);
        std::string values = matches[2];
        affectedRows = executeInsertWithoutColumns(table, values);
    }
    else
    {
        throw JsonDbException("Invalid SQL statement: " + sql);
    }

    return affectedRows;
}

bool sql::jsondb::Statement::execute(const std::string& sql)
{
    std::regex operationPattern(R"(^(?:SELECT|UPDATE|DELETE|INSERT)\s)", std::regex::icase);
    std::smatch matches;
    if (std::regex_search(sql, matches, operationPattern))
    {
        if (::toupper(matches[0].str()[0]) == 'S')
        {
            return executeQuery(sql) != nullptr;
        }
        else
        {
            return executeUpdate(sql) > 0;
        }
    }
    else
    {
        throw JsonDbException("Invalid SQL statement: " + sql);
    }
}

// 从完整表规范中提取表名
std::string Statement::extractTableName(const std::string& tableSpec)
{
    std::regex tablePattern(R"(\s*.*\.?([a-zA-Z0-9_]+)\s*)");
    std::smatch tableMatch;
    if (!std::regex_match(tableSpec, tableMatch, tablePattern))
    {
        throw JsonDbException("Invalid table name: " + tableSpec);
    }
    return tableMatch[1].str();
}

// 解析SET子句
std::map<std::string, std::string> Statement::parseSetClause(const std::string &setClause)
{
    std::map<std::string, std::string> updates;
    size_t start = 0, end = 0;
    
    while ((end = setClause.find(',', start)) != std::string::npos)
    {
        std::string pair = setClause.substr(start, end - start);
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos)
        {
            std::string col = pair.substr(0, eqPos);
            std::string val = pair.substr(eqPos + 1);
            // 去除空格和引号
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t'\""));
            val.erase(val.find_last_not_of(" \t'\"") + 1);
            updates[col] = val;
        }
        start = end + 1;
    }
    
    // 处理最后一对
    std::string lastPair = setClause.substr(start);
    size_t eqPos = lastPair.find('=');
    if (eqPos != std::string::npos)
    {
        std::string col = lastPair.substr(0, eqPos);
        std::string val = lastPair.substr(eqPos + 1);
        col.erase(0, col.find_first_not_of(" \t"));
        col.erase(col.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t'\""));
        val.erase(val.find_last_not_of(" \t'\"") + 1);
        updates[col] = val;
    }
    
    return updates;
}

// 解析逗号分隔的列表（用于列名和值）
std::vector<std::string> Statement::parseList(const std::string& list)
{
    std::vector<std::string> items;
    size_t start = 0, end = 0;
    
    while ((end = list.find(',', start)) != std::string::npos)
    {
        std::string item = list.substr(start, end - start);
        // 去除空格和引号
        item.erase(0, item.find_first_not_of(" \t'\""));
        item.erase(item.find_last_not_of(" \t'\"") + 1);
        items.push_back(item);
        start = end + 1;
    }
    
    // 处理最后一项
    std::string lastItem = list.substr(start);
    lastItem.erase(0, lastItem.find_first_not_of(" \t'\""));
    lastItem.erase(lastItem.find_last_not_of(" \t'\"") + 1);
    items.push_back(lastItem);
    
    return items;
}

// 根据列名和值创建新行数据
nlohmann::json Statement::createRowFromValues(const std::vector<std::string>& colNames, const std::vector<std::string>& colValues)
{
    if (colNames.size() != colValues.size())
    {
        throw JsonDbException("Column count doesn't match value count");
    }
    
    nlohmann::json newRow;
    for (size_t i = 0; i < colNames.size(); i++)
    {
        const std::string& val = colValues[i];
        if (val == "NULL")
        {
            newRow[colNames[i]] = nullptr;
        }
        else if (isdigit(val[0]) || (val[0] == '-' && val.size() > 1 && isdigit(val[1])))
        {
            try
            {
                if (val.find('.') != std::string::npos)
                {
                    newRow[colNames[i]] = std::stod(val);
                }
                else
                {
                    newRow[colNames[i]] = std::stoll(val);
                }
            }
            catch (...) {
                newRow[colNames[i]] = val;
            }
        }
        else if (val == "true" || val == "false")
        {
            newRow[colNames[i]] = (val == "true");
        }
        else
        {
            newRow[colNames[i]] = val;
        }
    }
    
    return newRow;
}

// 读取表数据
nlohmann::json Statement::readTableData(const std::string& tablePath)
{
    std::ifstream file(tablePath);
    if (!file.is_open())
    {
        throw JsonDbException("Failed to open table file: " + tablePath);
    }
    
    nlohmann::json tableData;
    try
    {
        file >> tableData;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        file.close();
        throw JsonDbException("Failed to parse JSON file: " + std::string(e.what()));
    }
    file.close();
    
    if (!tableData.is_array())
    {
        throw JsonDbException("Invalid table format: table data should be an array");
    }
    
    return tableData;
}

// 写回表数据
void Statement::writeTableData(const std::string& tablePath, const nlohmann::json& tableData)
{
    std::ofstream outFile(tablePath);
    if (!outFile.is_open())
    {
        throw JsonDbException("Failed to open table file for writing: " + tablePath);
    }
    outFile << std::setw(4) << tableData << std::endl;
    outFile.close();
}

// 实现UPDATE操作
size_t Statement::executeUpdateImpl(const std::string& table, const std::string& setClause, const std::string& whereClause)
{
    // 检查表是否存在
    if (!connection->tableExists(table))
    {
        throw JsonDbException("Table does not exist: " + table);
    }
    
    // 读取表数据
    std::string tablePath = connection->getTableFilePath(table);
    nlohmann::json tableData = readTableData(tablePath);
    
    // 解析SET子句
    std::map<std::string, std::string> updates = parseSetClause(setClause);
    
    // 更新匹配的行
    size_t affectedRows = 0;
    for (auto& row : tableData)
    {
        if (whereClause.empty() || evaluateCondition(row, whereClause))
        {
            // 应用更新
            for (const auto& [col, val] : updates)
            {
                // 尝试转换值的类型
                if (val == "NULL")
                {
                    row[col] = nullptr;
                }
                else if (isdigit(val[0]) || (val[0] == '-' && val.size() > 1 && isdigit(val[1])))
                {
                    try
                    {
                        if (val.find('.') != std::string::npos)
                        {
                            row[col] = std::stod(val);
                        }
                        else
                        {
                            row[col] = std::stoll(val);
                        }
                    }
                    catch (...) {
                        row[col] = val;
                    }
                }
                else if (val == "true" || val == "false")
                {
                    row[col] = (val == "true");
                }
                else
                {
                    row[col] = val;
                }
            }
            affectedRows++;
        }
    }
    
    // 写回文件
    writeTableData(tablePath, tableData);
    
    return affectedRows;
}

// 实现DELETE操作
size_t Statement::executeDeleteImpl(const std::string& table, const std::string& whereClause)
{
    // 检查表是否存在
    if (!connection->tableExists(table))
    {
        throw JsonDbException("Table does not exist: " + table);
    }
    
    // 读取表数据
    std::string tablePath = connection->getTableFilePath(table);
    nlohmann::json tableData = readTableData(tablePath);
    
    // 过滤保留不匹配WHERE条件的行
    nlohmann::json newTableData = nlohmann::json::array();
    size_t affectedRows = 0;
    
    for (const auto& row : tableData)
    {
        if (!whereClause.empty() && evaluateCondition(row, whereClause))
        {
            affectedRows++;
        }
        else
        {
            newTableData.push_back(row);
        }
    }
    
    // 写回文件
    writeTableData(tablePath, newTableData);
    
    return affectedRows;
}

// 实现带列名的INSERT操作
size_t Statement::executeInsertWithColumns(const std::string& table, const std::string& columns, const std::string& values)
{
    // 解析列名和值
    std::vector<std::string> colNames = parseList(columns);
    std::vector<std::string> colValues = parseList(values);
    
    // 创建新行数据
    nlohmann::json newRow = createRowFromValues(colNames, colValues);
    
    // 读取现有表数据或创建新表
    std::string tablePath = connection->getTableFilePath(table);
    nlohmann::json tableData;
    
    if (connection->tableExists(table))
    {
        tableData = readTableData(tablePath);
    }
    else
    {
        tableData = nlohmann::json::array();
    }
    
    // 添加新行
    tableData.push_back(newRow);
    
    // 写回文件
    writeTableData(tablePath, tableData);
    
    return 1; // INSERT操作通常只影响一行
}

// 实现不带列名的INSERT操作
size_t Statement::executeInsertWithoutColumns(const std::string& table, const std::string& values)
{
    // 检查表是否存在
    if (!connection->tableExists(table))
    {
        throw JsonDbException("Table does not exist for INSERT without column names: " + table);
    }
    
    // 获取表的列名
    std::vector<std::string> columnNames = connection->getColumnNames(table);
    
    // 解析值
    std::vector<std::string> colValues = parseList(values);
    
    // 创建新行数据
    nlohmann::json newRow = createRowFromValues(columnNames, colValues);
    
    // 读取表数据
    std::string tablePath = connection->getTableFilePath(table);
    nlohmann::json tableData = readTableData(tablePath);
    
    // 添加新行
    tableData.push_back(newRow);
    
    // 写回文件
    writeTableData(tablePath, tableData);
    
    return 1; // INSERT操作通常只影响一行
}

// 辅助函数：评估WHERE条件
bool Statement::evaluateCondition(const nlohmann::json& row, const std::string& condition)
{
    try
    {
        // 尝试匹配简单的条件格式：column operator value
        std::regex conditionPattern(R"((\w+)\s*([=!<>]=?)\s*(.+))");
        std::smatch condMatches;
        if (std::regex_match(condition, condMatches, conditionPattern))
        {
            std::string colName = condMatches[1];
            std::string op = condMatches[2];
            std::string value = condMatches[3];

            // 去除值两端的引号
            value.erase(0, value.find_first_not_of(" \t'\""));
            value.erase(value.find_last_not_of(" \t'\"") + 1);

            // 检查列是否存在
            if (row.contains(colName))
            {
                // 根据操作符进行比较
                if (op == "=")
                {
                    return (row[colName].is_number() && value == std::to_string(row[colName].get<double>())) ||
                           (row[colName].is_string() && row[colName] == value);
                }
                else if (op == "!=")
                {
                    return !(row[colName].is_number() && value == std::to_string(row[colName].get<double>())) &&
                           !(row[colName].is_string() && row[colName] == value);
                }
                else if (op == ">" && row[colName].is_number())
                {
                    return row[colName] > std::stod(value);
                }
                else if (op == "<" && row[colName].is_number())
                {
                    return row[colName] < std::stod(value);
                }
                else if (op == ">=" && row[colName].is_number())
                {
                    return row[colName] >= std::stod(value);
                }
                else if (op == "<=" && row[colName].is_number())
                {
                    return row[colName] <= std::stod(value);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // 条件解析错误，返回false
    }
    return false;
}

void sql::jsondb::PreparedStatement::bindParameter(size_t index, const std::string& value)
{
    parameters[index - 1] = value; // subtract 1 because parameters are 1-based index
}

void sql::jsondb::PreparedStatement::setInt(size_t index, int value)
{
    //convert int to string
    parameters[index - 1] = std::to_string(value);
}

void sql::jsondb::PreparedStatement::setFloat(size_t index, float value)
{
    parameters[index - 1] = std::to_string(value);
}

void sql::jsondb::PreparedStatement::setString(size_t index, const std::string& value)
{
    parameters[index - 1] = value;
}

void sql::jsondb::PreparedStatement::setDateTime(size_t index, const std::string& value)
{
    parameters[index - 1] = value;
}

std::shared_ptr<ResultSet> sql::jsondb::PreparedStatement::executeQuery()
{
    // TODO: Implement executeQuery for PreparedStatement
    auto input = this->sql;
    size_t pos = 0;
    int index = 0;
    while ((pos = input.find('?')) != std::string::npos)
    {
        input.replace(pos, 1, parameters[index++]);
    }

    return stmt.executeQuery(input);
}

size_t sql::jsondb::PreparedStatement::executeUpdate()
{
    // TODO: Implement executeUpdate for PreparedStatement
    auto input = this->sql;
    size_t pos = 0;
    int index = 0;
    while ((pos = input.find('?')) != std::string::npos)
    {
        input.replace(pos, 1, parameters[index++]);
    }

    return stmt.executeUpdate(input);
}

bool sql::jsondb::PreparedStatement::execute()
{
    // TODO: Implement execute for PreparedStatement
    auto input = this->sql;
    size_t pos = 0;
    int index = 0;
    while ((pos = input.find('?')) != std::string::npos)
    {
        input.replace(pos, 1, parameters[index++]);
    }

    return stmt.execute(input);
}

bool sql::jsondb::ResultSet::is_date(const std::string& str)
{
    // using regex to match date format, e.g., 2022-01-01
    // not perfect, but should work for most cases
    std::regex datePattern(R"(\d{4}-\d{2}-\d{2})");
    std::regex dateTimePattern(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})");
    std::regex timePattern(R"(\d{2}:\d{2}:\d{2})");
    return std::regex_match(str, datePattern) || std::regex_match(str, dateTimePattern) || std::regex_match(str, timePattern);
}

sql::jsondb::ResultSet::ResultSet(const std::vector<nlohmann::json>& data)
{
    currentIndex = 0;
    rows = data;
    auto& columns = metaData->columns;
    // TODO: Initialize metaData based on the structure of the rows
    auto firstRow = rows.empty() ? nlohmann::json::object() : rows[0];
    for (auto& [key, value] : firstRow.items())
    {
        if (value.is_number_integer())
        {
            columns.push_back({ key, DataType::INT });
        }
        else if (value.is_number_float())
        {
            columns.push_back({ key, DataType::FLOAT });
        }
        else if (value.is_boolean())
        {
            columns.push_back({ key, DataType::BOOLEAN });
        }
        else if (value.is_string())
        {
            columns.push_back({ key, DataType::VARCHAR });
        }
        else if (is_date(value.get<std::string>()))
        {
            columns.push_back({ key, DataType::DATETIME });
        }
        else
        {
            columns.push_back({ key, DataType::UNKOWN });
        }
    }

}

bool ResultSet::next()
{
    if (currentIndex < rows.size())
    {
        currentIndex++;
    }
    else return false;
}

int sql::jsondb::ResultSet::getInt(const std::string& columnLabel)
{
    return rows[currentIndex][columnLabel].get<int>();
}

float sql::jsondb::ResultSet::getFloat(const std::string& columnLabel)
{
    return rows[currentIndex][columnLabel].get<float>();
}

std::string sql::jsondb::ResultSet::getString(const std::string& columnLabel)
{
    return rows[currentIndex][columnLabel].get<std::string>();
}

bool sql::jsondb::ResultSet::getBoolean(const std::string& columnLabel)
{
    return rows[currentIndex][columnLabel].get<bool>();
}

std::string sql::jsondb::ResultSet::getDateTime(const std::string& columnLabel)
{
    return rows[currentIndex][columnLabel].get<std::string>();
}

std::string sql::jsondb::ResultSetMetaData::getColumnName(size_t index) const
{
    return columns[index].first;
}

DataType sql::jsondb::ResultSetMetaData::getColumnType(size_t index) const
{
    return columns[index].second;
}

std::vector<std::string> sql::jsondb::DatabaseMetaData::getTables()
{
    auto dbPath = connection->getDbPath();
    std::vector<std::string> tables;
    for (auto& file : std::filesystem::directory_iterator(dbPath))
    {
        if (file.is_regular_file() && file.path().extension() == ".json")
        {
            tables.push_back(file.path().stem().string());
        }
    }

    return tables;
}
