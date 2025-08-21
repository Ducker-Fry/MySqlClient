#pragma once
#include<model/DataTable.h>
#include <string>
#include <chrono>
#include <optional>
#include<file_sqlquery/QueryType.h>

// 查询结果状态
enum class ResultStatus
{
    SUCCESS,       // 成功
    ERROR,         // 错误
    TIMEOUT,       // 超时
    CONNECTION_LOST // 连接丢失
};

class QueryResult
{
private:
    QueryType queryType;                // 查询类型
    ResultStatus status;                // 执行状态
    std::string sql;                    // 原始SQL语句
    std::string errorMessage;           // 错误信息（状态为ERROR时有效）
    DataTable data;                     // 查询结果数据（SELECT时有效）
    int affectedRows;                   // 影响的行数（INSERT/UPDATE/DELETE时有效）
    std::chrono::milliseconds duration; // 执行耗时
    std::string timestamp;              // 执行时间戳（格式：YYYY-MM-DD HH:MM:SS）

public:
    // 构造函数
    QueryResult();
    explicit QueryResult(const std::string& sql);

    // 设置查询类型
    void setQueryType(QueryType type);
    QueryType getQueryType() const;

    // 设置/获取执行状态
    void setStatus(ResultStatus stat, const std::string& errMsg = "");
    ResultStatus getStatus() const;
    bool isSuccess() const;

    // 原始SQL相关
    void setSql(const std::string& sql);
    const std::string& getSql() const;

    // 错误信息（仅状态为ERROR时有效）
    const std::string& getErrorMessage() const;
    void setError(const std::string& errmsg);

    // 结果数据（仅SELECT查询有效）
    DataTable& getData();
    const DataTable& getData() const;
    void setData(const DataTable& table);

    // 影响行数（仅INSERT/UPDATE/DELETE有效）
    void setAffectedRows(int rows);
    int getAffectedRows() const;

    // 执行耗时
    void setDuration(std::chrono::milliseconds dur);
    std::chrono::milliseconds getDuration() const;

    // 时间戳
    void setTimestamp(const std::string& ts);
    const std::string& getTimestamp() const;

    // 重置结果（复用对象时使用）
    void reset();

    //设置结果表格
    void setResultTable(const DataTable& table);
};

