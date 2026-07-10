#include <core/sql_parser.h>

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace
{
std::string Trim(const std::string& value)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos)
    {
        return "";
    }

    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::vector<std::string> SplitCommaSeparated(const std::string& input)
{
    std::vector<std::string> parts;
    std::stringstream stream(input);
    std::string item;
    while (std::getline(stream, item, ','))
    {
        parts.push_back(Trim(item));
    }
    return parts;
}
}

std::string QuerySqlParser::preprocessSql(const std::string& sql)
{
    std::string processed = sql;
    processed = std::regex_replace(processed, std::regex(R"(/\*[\s\S]*?\*/)"), " ");
    processed = std::regex_replace(processed, std::regex(R"(--[^\r\n]*)"), " ");
    processed = std::regex_replace(processed, std::regex(R"(\s+)"), " ");
    processed = Trim(processed);
    processed = std::regex_replace(processed, std::regex(R"(;\s*$)"), "");
    std::transform(
        processed.begin(),
        processed.end(),
        processed.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return processed;
}

std::string QuerySqlParser::extractSqlType(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(^(SELECT|INSERT|UPDATE|DELETE|CREATE|DROP)\b)")))
    {
        return match[1].str();
    }
    return "UNKNOWN";
}

std::vector<std::string> QuerySqlParser::extractColumns(const std::string& processedSql)
{
    std::smatch match;

    if (std::regex_search(processedSql, match, std::regex(R"(^SELECT\s+(.*?)\s+FROM\b)")))
    {
        return SplitCommaSeparated(match[1].str());
    }

    if (std::regex_search(processedSql, match, std::regex(R"(^INSERT\s+INTO\s+\w+(?:\.\w+)?\s*\((.*?)\)\s+VALUES\b)")))
    {
        return SplitCommaSeparated(match[1].str());
    }

    if (std::regex_search(processedSql, match, std::regex(R"(^UPDATE\s+\w+(?:\.\w+)?\s+SET\s+(.*?)(?:\s+WHERE\b|$))")))
    {
        std::vector<std::string> columns;
        for (const auto& assignment : SplitCommaSeparated(match[1].str()))
        {
            const std::size_t equalsPos = assignment.find('=');
            if (equalsPos != std::string::npos)
            {
                columns.push_back(Trim(assignment.substr(0, equalsPos)));
            }
        }
        return columns;
    }

    return {};
}

void QuerySqlParser::extractTableAndDatabase(
    const std::string& processedSql,
    std::string& table,
    std::string& database)
{
    std::smatch match;
    if (std::regex_search(
            processedSql,
            match,
            std::regex(R"((?:FROM|UPDATE|INTO|TABLE)\s+([A-Z0-9_]+(?:\.[A-Z0-9_]+)?))")))
    {
        const std::string tableSpec = match[1].str();
        const std::size_t dotPos = tableSpec.find('.');
        if (dotPos == std::string::npos)
        {
            table = tableSpec;
            database.clear();
        }
        else
        {
            database = tableSpec.substr(0, dotPos);
            table = tableSpec.substr(dotPos + 1);
        }
    }
}

std::string QuerySqlParser::extractWhereClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(WHERE\s+(.*?)(GROUP BY|HAVING|ORDER BY|LIMIT|$))")))
    {
        return match[1].str();
    }
    return "";
}

std::vector<std::string> QuerySqlParser::extractGroupByColumns(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(GROUP BY\s+(.*?)(HAVING|ORDER BY|LIMIT|$))")))
    {
        return SplitCommaSeparated(match[1].str());
    }
    return {};
}

std::string QuerySqlParser::extractHavingClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(HAVING\s+(.*?)(ORDER BY|LIMIT|$))")))
    {
        return match[1].str();
    }
    return "";
}

std::string QuerySqlParser::extractOrderByClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(ORDER BY\s+(.*?)(LIMIT|$))")))
    {
        return match[1].str();
    }
    return "";
}

std::string QuerySqlParser::extractLimitClause(const std::string& processedSql)
{
    std::smatch match;
    if (std::regex_search(processedSql, match, std::regex(R"(LIMIT\s+(.*?)$)")))
    {
        return match[1].str();
    }
    return "";
}

std::shared_ptr<SqlParseResult> QuerySqlParser::parse(InputData& input)
{
    const std::string processedSql = preprocessSql(input.getRawData());

    auto result = std::make_shared<SqlParseResultQuery>();
    result->setRawQuery(processedSql);

    const std::string sqlType = extractSqlType(processedSql);
    result->setType(sqlType);
    result->setColumns(extractColumns(processedSql));

    std::string database;
    std::string table;
    extractTableAndDatabase(processedSql, table, database);
    result->setDatabase(database);
    result->setTable(table);
    result->setWhereClause(extractWhereClause(processedSql));
    result->setGroupByColumns(extractGroupByColumns(processedSql));
    result->setHavingClause(extractHavingClause(processedSql));
    result->setOrderByClause(extractOrderByClause(processedSql));
    result->setLimitClause(extractLimitClause(processedSql));

    if (sqlType == "SELECT")
    {
        result->setOperationType(SqlType::SELECT);
    }
    else if (sqlType == "INSERT")
    {
        result->setOperationType(SqlType::INSERT);
    }
    else if (sqlType == "UPDATE")
    {
        result->setOperationType(SqlType::UPDATE);
    }
    else if (sqlType == "DELETE")
    {
        result->setOperationType(SqlType::DELETE);
    }
    else if (sqlType == "CREATE")
    {
        result->setOperationType(SqlType::CREATE);
    }
    else if (sqlType == "DROP")
    {
        result->setOperationType(SqlType::DROP);
    }
    else
    {
        result->setOperationType(SqlType::UNKNOWN);
    }

    return result;
}
