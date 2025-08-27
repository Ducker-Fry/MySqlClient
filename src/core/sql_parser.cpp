#include <core/sql_parser.h>
#include <regex>
#include <algorithm>
#include <sstream>

std::string QuerySqlParser::preprocessSql(const std::string& sql)
{
    std::string processed = sql;

    // 去除/* */块注释
    processed = std::regex_replace(processed, std::regex("/\\*.*?\\*/"), "", std::regex_constants::match_any);
    // 去除-- 单行注释
    processed = std::regex_replace(processed, std::regex("--.*$"), "", std::regex_constants::match_any);
    // 去除多余空格和换行
    processed = std::regex_replace(processed, std::regex("\\s+"), " ");
    // 去除语句末尾的分号
    processed = std::regex_replace(processed, std::regex(";\\s*$"), "");
    // 转为大写（便于关键词匹配）
    std::transform(processed.begin(), processed.end(), processed.begin(), ::toupper);

    return processed;
}

std::string QuerySqlParser::extractSqlType(const std::string& processedSql)
{
    std::smatch match;
    if(std::regex_search(processedSql, match, std::regex("(SELECT|INSERT|UPDATE|DELETE)")))
    {
        return match[1].str();
    }
    return "UNKNOWN";
}

std::vector<std::string> QuerySqlParser::extractColumns(const std::string& processedSql)
{
    std::vector<std::string> columns;
    std::smatch match;

    // 匹配SQL语句中的列名
    if (std::regex_search(processedSql, match, std::regex("(SELECT|UPDATE|INSERT INTO)\\s+(.*?)\\s+(FROM|SET|VALUES)")))
    {
        std::string colsStr = match[2].str();
        // 按逗号分割列（处理可能的空格）
        std::stringstream ss(colsStr);
        std::string col;
        while (std::getline(ss, col, ','))
        {
            // 去除列名前后空格
            col.erase(0, col.find_first_not_of(" "));
            col.erase(col.find_last_not_of(" ") + 1);
            columns.push_back(col);
        }
    }
    return columns;
}
void QuerySqlParser::extractTableAndDatabase(const std::string& processedSql, std::string& table, std::string& database)
{
    std::smatch match;
    // 匹配SQL语句中的表名（支持SELECT, UPDATE, INSERT, DELETE语句）
    std::regex tableRegex(R"((FROM|UPDATE|INTO|FROM) (\w+\.?\w*))");
    if (std::regex_search(processedSql, match, tableRegex))
    {
        std::string tableStr = match[2].str();
        size_t dotPos = tableStr.find('.');
        if (dotPos != std::string::npos)
        {
            database = tableStr.substr(0, dotPos);
            table = tableStr.substr(dotPos + 1);
        }
        else
        {
            table = tableStr; // 无数据库名，默认空
        }
    }
}

std::string QuerySqlParser::extractWhereClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex("WHERE (.*?)(GROUP BY|ORDER BY|LIMIT|$)")))
    {
        return match[1].str();
    }
    return "";
}

std::vector<std::string> QuerySqlParser::extractGroupByColumns(const std::string& processedSql)
{
    std::vector<std::string> groupBy;
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex("GROUP BY (.*?)(HAVING|ORDER BY|LIMIT|$)")))
    {
        std::string groupStr = match[1].str();
        std::stringstream ss(groupStr);
        std::string col;
        while (std::getline(ss, col, ','))
        {
            col.erase(0, col.find_first_not_of(" "));
            col.erase(col.find_last_not_of(" ") + 1);
            groupBy.push_back(col);
        }
    }
    return groupBy;
}

std::string QuerySqlParser::extractHavingClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex("HAVING (.*?)(ORDER BY|LIMIT|$)")))
    {
        return match[1].str();
    }
    return "";
}

std::string QuerySqlParser::extractOrderByClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex("ORDER BY (.*?)(LIMIT|$)")))
    {
        return match[1].str();
    }
    return "";
}

std::string QuerySqlParser::extractLimitClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex("LIMIT (.*?)$")))
    {
        return match[1].str();
    }
    return "";
}

std::shared_ptr<SqlParseResult> QuerySqlParser::parse(InputData& input)
{
    std::string rawData = input.getRawData();
    SqlParseResultQuery result;
    std::string processedSql = preprocessSql(rawData);
    std::string sqlType = extractSqlType(processedSql);
    std::string db, table;
    extractTableAndDatabase(processedSql, table, db);
    std::string where = extractWhereClause(processedSql);
    std::string orderBy = extractOrderByClause(processedSql);
    std::vector<std::string> groupBy = extractGroupByColumns(processedSql);
    std::string limit = extractLimitClause(processedSql);
    std::string having = extractHavingClause(processedSql);

    if(sqlType == "SELECT")
        result.setOperationType(SqlType::SELECT);
    else if(sqlType == "INSERT")
        result.setOperationType(SqlType::INSERT);
    else if(sqlType == "UPDATE")
        result.setOperationType(SqlType::UPDATE);
    else if(sqlType == "DELETE")
        result.setOperationType(SqlType::DELETE);
    else
        result.setOperationType(SqlType::UNKNOWN);

    result.setTable(table);
    result.setDatabase(db);
    result.setColumns(extractColumns(processedSql));
    result.setWhereClause(where);
    result.setGroupByColumns(groupBy);
    result.setOrderByClause(orderBy);
    result.setHavingClause(having);
    result.setLimitClause(limit);

    return std::make_shared<SqlParseResultQuery>(result);
}