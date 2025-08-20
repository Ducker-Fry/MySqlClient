#include<model/QueryResult.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

QueryResult::QueryResult()
    : queryType(QueryType::UNKNOWN),
    status(ResultStatus::SUCCESS),
    affectedRows(0),
    duration(std::chrono::milliseconds(0))
{
    // 生成默认时间戳（当前时间）
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&nowTime);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    timestamp = ss.str();
}

QueryResult::QueryResult(const std::string& sql) : QueryResult()
{
    this->sql = sql;
}

void QueryResult::setQueryType(QueryType type)
{
    queryType = type;
}

QueryType QueryResult::getQueryType() const
{
    return queryType;
}

void QueryResult::setStatus(ResultStatus stat, const std::string& errMsg)
{
    status = stat;
    if (stat == ResultStatus::ERROR)
    {
        errorMessage = errMsg;
    }
    else
    {
        errorMessage.clear();
    }
}

ResultStatus QueryResult::getStatus() const
{
    return status;
}

bool QueryResult::isSuccess() const
{
    return status == ResultStatus::SUCCESS;
}

void QueryResult::setSql(const std::string& sql)
{
    this->sql = sql;
}

const std::string& QueryResult::getSql() const
{
    return sql;
}

const std::string& QueryResult::getErrorMessage() const
{
    return errorMessage;
}

DataTable& QueryResult::getData()
{
    return data;
}

const DataTable& QueryResult::getData() const
{
    return data;
}

void QueryResult::setData(const DataTable& table)
{
    data.copyFrom(table);
}

void QueryResult::setAffectedRows(int rows)
{
    affectedRows = rows;
}

int QueryResult::getAffectedRows() const
{
    return affectedRows;
}

void QueryResult::setDuration(std::chrono::milliseconds dur)
{
    duration = dur;
}

std::chrono::milliseconds QueryResult::getDuration() const
{
    return duration;
}

void QueryResult::setTimestamp(const std::string& ts)
{
    timestamp = ts;
}

const std::string& QueryResult::getTimestamp() const
{
    return timestamp;
}

void QueryResult::reset()
{
    queryType = QueryType::UNKNOWN;
    status = ResultStatus::SUCCESS;
    sql.clear();
    errorMessage.clear();
    data.clear();
    affectedRows = 0;
    duration = std::chrono::milliseconds(0);

    // 更新时间戳为当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&nowTime);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    timestamp = ss.str();
}
