#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <input/inputdata.h>

enum class SqlType
{
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    UNKNOWN
};

class SqlParseResult
{
protected:
    std::string database;
    std::string table;
    std::vector<std::string> columns;
    std::string type;
public:
    SqlParseResult() = default;

    // Setter和Getter（方便赋值和测试）
    void setDatabase(const std::string& db) { database = db; }
    void setTable(const std::string& t) { table = t; }
    void setColumns(const std::vector<std::string>& cols) { columns = cols; }
    void setType(const std::string& t) { type = t; }

    std::string getDatabase() const { return database; }
    std::string getTable() const { return table; }
    std::vector<std::string> getColumns() const { return columns; }
    std::string getType() const { return type; }
};

class SqlParseResultQuery : public SqlParseResult
{
private:
    std::string whereClause;
    std::string orderByClause;
    std::string limitClause;
    std::vector<std::string> groupByColumns;
    std::string havingClause;
    std::string rawQuery;
    std::string operationType; // SELECT, INSERT, UPDATE, DELETE
public:
    SqlParseResultQuery() = default;

    // Setter和Getter
    void setWhereClause(const std::string& where) { whereClause = where; }
    void setOrderByClause(const std::string& order) { orderByClause = order; }
    void setLimitClause(const std::string& limit) { limitClause = limit; }
    void setGroupByColumns(const std::vector<std::string>& group) { groupByColumns = group; }
    void setHavingClause(const std::string& having) { havingClause = having; }
    void setRawQuery(const std::string& raw) { rawQuery = raw; }
    void setOperationType(const std::string& op) { operationType = op; }

    std::string getWhereClause() const { return whereClause; }
    std::string getOrderByClause() const { return orderByClause; }
    std::string getLimitClause() const { return limitClause; }
    std::vector<std::string> getGroupByColumns() const { return groupByColumns; }
    std::string getHavingClause() const { return havingClause; }
    std::string getRawQuery() const { return rawQuery; }
    std::string getOperationType() const { return operationType; }
};

class SqlParseResultImport : public SqlParseResult
{
private:
    std::unordered_map<std::string, std::vector<std::string>> data;
public:
    SqlParseResultImport() = default;
};

class ISqlParser
{
public:
    virtual SqlParseResult parse(InputData& input) = 0;
};

class QuerySqlParser : public ISqlParser
{
private:
    std::string preprocessSql(const std::string& sql);
    std::string extractSqlType(const std::string& processedSql);
    std::vector<std::string> extractColumns(const std::string& processedSql);
    void extractTableAndDatabase(const std::string& processedSql, std::string& table, std::string& database);
    std::string extractWhereClause(const std::string& processedSql);
    std::vector<std::string> extractGroupByColumns(const std::string& processedSql);
    std::string extractHavingClause(const std::string& processedSql);
    std::string extractOrderByClause(const std::string& processedSql);
    std::string extractLimitClause(const std::string& processedSql);
public:
    SqlParseResult parse(InputData& input) override;
};
