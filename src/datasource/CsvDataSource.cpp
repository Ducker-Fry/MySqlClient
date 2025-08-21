#include<file_sqlquery/core/CsvDataSource.h>
#include "model/QueryResult.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <stdexcept>

namespace
{
    // 字符串修剪辅助函数
    std::string trim(const std::string& s)
    {
        auto start = s.begin();
        while (start != s.end() && std::isspace(*start))
        {
            start++;
        }
        auto end = s.end();
        do
        {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));
        return std::string(start, end + 1);
    }
}

CsvDataSource::~CsvDataSource()
{
    disconnect();
}

bool CsvDataSource::connect(const std::string& source,
    const std::vector<std::pair<std::string, std::string>>& params)
{
    // 解析连接参数
    for (const auto& param : params)
    {
        if (param.first == "delimiter" && !param.second.empty())
        {
            m_delimiter = param.second[0];
        }
        else if (param.first == "quote" && !param.second.empty())
        {
            m_quote = param.second[0];
        }
        else if (param.first == "header")
        {
            m_hasHeader = (param.second == "true");
        }
    }

    // 尝试打开文件
    m_filePath = source;
    m_fileStream.open(source, std::ios::in);
    if (!m_fileStream.is_open())
    {
        return false;
    }

    m_connected = true;
    return true;
}

void CsvDataSource::disconnect()
{
    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }
    m_connected = false;
    m_filePath.clear();
}

QueryResult CsvDataSource::query(const std::string& query)
{
    QueryResult result;
    if (!isConnected())
    {
        result.setError("Not connected to CSV data source");
        return result;
    }

    // 重置文件流到开始位置
    m_fileStream.clear();
    m_fileStream.seekg(0, std::ios::beg);

    DataTable table;
    table.setTableName(m_filePath.substr(m_filePath.find_last_of("/\\") + 1));

    std::string line;
    std::vector<std::string> columns;
    std::string conditionColumn;
    std::string conditionValue;
    bool hasCondition = false;

    // 解析查询条件
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    if (upperQuery.find("WHERE") != std::string::npos)
    {
        hasCondition = parseQueryCondition(query, conditionColumn, conditionValue);
    }

    // 读取表头
    if (m_hasHeader && std::getline(m_fileStream, line))
    {
        columns = parseCsvLine(line);
        for (const auto& col : columns)
        {
            table.addColumn(trim(col));
        }
    }
    else if (!m_hasHeader)
    {
        // 如果没有表头，使用默认列名
        if (std::getline(m_fileStream, line))
        {
            auto firstRow = parseCsvLine(line);
            for (size_t i = 0; i < firstRow.size(); ++i)
            {
                table.addColumn("col_" + std::to_string(i + 1));
                columns.push_back("col_" + std::to_string(i + 1));
            }
            // 重置流，重新读取第一行
            m_fileStream.clear();
            m_fileStream.seekg(0, std::ios::beg);
        }
    }

    // 查找条件列索引
    size_t conditionIndex = std::string::npos;
    if (hasCondition)
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (trim(columns[i]) == conditionColumn)
            {
                conditionIndex = i;
                break;
            }
        }
        if (conditionIndex == std::string::npos)
        {
            result.setError("Column not found: " + conditionColumn);
            return result;
        }
    }

    // 读取数据行
    while (std::getline(m_fileStream, line))
    {
        if (line.empty()) continue;

        auto fields = parseCsvLine(line);
        DataRow row;

        // 检查是否符合条件
        if (hasCondition)
        {
            if (conditionIndex >= fields.size()) continue;
            if (trim(fields[conditionIndex]) != conditionValue) continue;
        }

        // 添加行数据
        for (const auto& field : fields)
        {
            row.addValue(convertStringToValue(trim(field)));
        }
        table.addRow(row);
    }

    result.setResultTable(table);
    return result;
}

bool CsvDataSource::write(const DataTable& table, const std::string& target)
{
    std::ofstream outFile(target, std::ios::out);
    if (!outFile.is_open())
    {
        return false;
    }

    const auto& columns = table.getColumns();
    const auto& rows = table.getRows();

    // 写入表头
    if (m_hasHeader && !columns.empty())
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (i > 0) outFile << m_delimiter;
            // 包含特殊字符时添加引号
            if (columns[i].find(m_delimiter) != std::string::npos ||
                columns[i].find(m_quote) != std::string::npos)
            {
                outFile << m_quote << columns[i] << m_quote;
            }
            else
            {
                outFile << columns[i];
            }
        }
        outFile << "\n";
    }

    // 写入数据行
    for (const auto& row : rows)
    {
        for (size_t i = 0; i < row.size(); ++i)
        {
            if (i > 0) outFile << m_delimiter;

            std::string valueStr;
            const auto& value = row[i];

            // 转换值为字符串
            if (std::holds_alternative<int>(value))
            {
                valueStr = std::to_string(std::get<int>(value));
            }
            else if (std::holds_alternative<double>(value))
            {
                valueStr = std::to_string(std::get<double>(value));
            }
            else if (std::holds_alternative<bool>(value))
            {
                valueStr = std::get<bool>(value) ? "true" : "false";
            }
            else if (std::holds_alternative<std::string>(value))
            {
                valueStr = std::get<std::string>(value);
            }
            else
            {
                valueStr = "";
            }

            // 包含特殊字符时添加引号
            if (valueStr.find(m_delimiter) != std::string::npos ||
                valueStr.find(m_quote) != std::string::npos ||
                valueStr.find('\n') != std::string::npos)
            {
                outFile << m_quote << valueStr << m_quote;
            }
            else
            {
                outFile << valueStr;
            }
        }
        outFile << "\n";
    }

    return true;
}

bool CsvDataSource::isConnected() const
{
    return m_connected && m_fileStream.is_open();
}

DataSourceType CsvDataSource::getType() const
{
    return DataSourceType::CSV;
}

std::string CsvDataSource::getDescription() const
{
    return "CSV File: " + m_filePath;
}

std::vector<std::string> CsvDataSource::parseCsvLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::string currentField;
    bool inQuotes = false;
    size_t i = 0;
    size_t n = line.size();

    while (i < n)
    {
        if (line[i] == m_quote)
        {
            // 处理引号
            if (i + 1 < n && line[i + 1] == m_quote)
            {
                // 两个引号表示转义
                currentField += m_quote;
                i += 2;
            }
            else
            {
                inQuotes = !inQuotes;
                i++;
            }
        }
        else if (line[i] == m_delimiter && !inQuotes)
        {
            // 分隔符且不在引号中
            fields.push_back(currentField);
            currentField.clear();
            i++;
        }
        else
        {
            currentField += line[i];
            i++;
        }
    }

    // 添加最后一个字段
    fields.push_back(currentField);

    return fields;
}

ValueType CsvDataSource::convertStringToValue(const std::string& str)
{
    if (str.empty())
    {
        return std::nullptr_t{};
    }

    // 尝试转换为整数
    try
    {
        size_t pos;
        int intVal = std::stoi(str, &pos);
        if (pos == str.size())
        {
            return intVal;
        }
    }
    catch (...) {}

    // 尝试转换为浮点数
    try
    {
        size_t pos;
        double doubleVal = std::stod(str, &pos);
        if (pos == str.size())
        {
            return doubleVal;
        }
    }
    catch (...) {}

    // 尝试转换为布尔值
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    if (lowerStr == "true")
    {
        return true;
    }
    else if (lowerStr == "false")
    {
        return false;
    }

    // 默认为字符串
    return str;
}

bool CsvDataSource::parseQueryCondition(const std::string& query, std::string& column, std::string& value)
{
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    size_t wherePos = upperQuery.find("WHERE");
    if (wherePos == std::string::npos)
    {
        return false;
    }

    std::string conditionPart = query.substr(wherePos + 5);
    size_t eqPos = conditionPart.find('=');
    if (eqPos == std::string::npos)
    {
        return false;
    }

    column = trim(conditionPart.substr(0, eqPos));
    value = trim(conditionPart.substr(eqPos + 1));

    // 移除值两边可能的引号
    if (!value.empty() && (value.front() == '"' || value.front() == '\''))
    {
        value = value.substr(1);
    }
    if (!value.empty() && (value.back() == '"' || value.back() == '\''))
    {
        value = value.substr(0, value.size() - 1);
    }

    return !column.empty() && !value.empty();
}