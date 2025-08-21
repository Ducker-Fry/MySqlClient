#include <gtest/gtest.h>
#include <string>
#include <chrono>
#include <vector>
#include <any>
#include "model/QueryResult.h"
#include "model/DataTable.h"

// 测试枚举类型的有效性（确保与类定义一致）
TEST(QueryResultEnumTest, BasicValidation) {
    EXPECT_EQ(static_cast<int>(QueryType::UNKNOWN), 5);
    EXPECT_EQ(static_cast<int>(QueryType::SELECT), 0);  // 假设定义了SELECT类型
    EXPECT_EQ(static_cast<int>(QueryType::INSERT), 1);  // 假设定义了INSERT类型
    EXPECT_EQ(static_cast<int>(QueryType::UPDATE), 2);  // 假设定义了UPDATE类型
    EXPECT_EQ(static_cast<int>(QueryType::DELETE), 3);  // 假设定义了DELETE类型

    EXPECT_EQ(static_cast<int>(ResultStatus::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(ResultStatus::ERROR), 1);   // 假设定义了ERROR状态
    EXPECT_EQ(static_cast<int>(ResultStatus::TIMEOUT), 2); // 假设定义了TIMEOUT状态
}

// 测试构造函数和默认初始化
TEST(QueryResultTest, ConstructorInitialization) {
    // 测试默认构造函数
    QueryResult result;
    EXPECT_EQ(result.getQueryType(), QueryType::UNKNOWN);
    EXPECT_EQ(result.getStatus(), ResultStatus::SUCCESS);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.getSql().empty());
    EXPECT_TRUE(result.getErrorMessage().empty());
    EXPECT_EQ(result.getAffectedRows(), 0);
    EXPECT_EQ(result.getDuration(), std::chrono::milliseconds(0));
    EXPECT_FALSE(result.getTimestamp().empty());  // 时间戳应自动生成

    // 测试带SQL的构造函数
    std::string testSql = "SELECT * FROM users";
    QueryResult resultWithSql(testSql);
    EXPECT_EQ(resultWithSql.getSql(), testSql);
    EXPECT_EQ(resultWithSql.getQueryType(), QueryType::UNKNOWN);  // 类型默认UNKNOWN
    EXPECT_TRUE(resultWithSql.isSuccess());
}

// 测试查询类型设置与获取
TEST(QueryResultTest, QueryTypeOperations) {
    QueryResult result;
    
    result.setQueryType(QueryType::SELECT);
    EXPECT_EQ(result.getQueryType(), QueryType::SELECT);
    
    result.setQueryType(QueryType::INSERT);
    EXPECT_EQ(result.getQueryType(), QueryType::INSERT);
    
    result.setQueryType(QueryType::DELETE);
    EXPECT_EQ(result.getQueryType(), QueryType::DELETE);
}

// 测试状态设置与错误信息管理
TEST(QueryResultTest, StatusAndErrorHandling) {
    QueryResult result;
    
    // 测试成功状态
    result.setStatus(ResultStatus::SUCCESS, "");
    EXPECT_EQ(result.getStatus(), ResultStatus::SUCCESS);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.getErrorMessage().empty());
    
    // 测试错误状态
    std::string errorMsg = "Table 'users' not found";
    result.setStatus(ResultStatus::ERROR, errorMsg);
    EXPECT_EQ(result.getStatus(), ResultStatus::ERROR);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getErrorMessage(), errorMsg);
    
    // 测试非错误状态下错误信息清空
    result.setStatus(ResultStatus::TIMEOUT, "This message should be ignored");
    EXPECT_EQ(result.getStatus(), ResultStatus::TIMEOUT);
    EXPECT_TRUE(result.getErrorMessage().empty());  // 非ERROR状态不保留错误信息
}

// 测试数据表格关联功能
TEST(QueryResultTest, DataTableOperations) {
    QueryResult result;
    DataTable expectedTable({"id", "name"});
    expectedTable.addRow({1, std::string("Alice")});
    expectedTable.addRow({2, std::string("Bob")});
    
    // 设置数据并验证
    result.setData(expectedTable);
    const DataTable& actualTable = result.getData();
    
    EXPECT_EQ(actualTable.getColumnCount(), expectedTable.getColumnCount());
    EXPECT_EQ(actualTable.getRowCount(), expectedTable.getRowCount());
    EXPECT_EQ(std::any_cast<std::string>(actualTable.getValue(0, "name")), "Alice");
    EXPECT_EQ(std::any_cast<int>(actualTable.getValue(1, "id")), 2);
    
    // 测试数据修改（通过引用修改内部DataTable）
    DataTable& mutableTable = result.getData();
    mutableTable.addRow({3, std::string("Charlie")});
    EXPECT_EQ(result.getData().getRowCount(), 3);  // 内部数据应被修改
}

// 测试影响行数和耗时设置
TEST(QueryResultTest, AffectedRowsAndDuration) {
    QueryResult result;
    
    // 测试影响行数
    result.setAffectedRows(5);
    EXPECT_EQ(result.getAffectedRows(), 5);
    
    result.setAffectedRows(0);
    EXPECT_EQ(result.getAffectedRows(), 0);
    
    // 测试耗时设置
    auto testDuration = std::chrono::milliseconds(123);
    result.setDuration(testDuration);
    EXPECT_EQ(result.getDuration(), testDuration);
    
    result.setDuration(std::chrono::milliseconds(0));
    EXPECT_EQ(result.getDuration(), std::chrono::milliseconds(0));
}

// 测试时间戳设置
TEST(QueryResultTest, TimestampOperations) {
    QueryResult result;
    std::string originalTs = result.getTimestamp();
    EXPECT_FALSE(originalTs.empty());
    
    // 自定义时间戳
    std::string customTs = "2024-01-01 12:34:56";
    result.setTimestamp(customTs);
    EXPECT_EQ(result.getTimestamp(), customTs);
}

// 测试重置功能
TEST(QueryResultTest, ResetFunctionality) {
    QueryResult result("INSERT INTO logs VALUES ('test')");
    result.setQueryType(QueryType::INSERT);
    result.setStatus(ResultStatus::ERROR, "Duplicate entry");
    result.setAffectedRows(1);
    result.setDuration(std::chrono::milliseconds(50));
    result.setTimestamp("2024-01-01 00:00:00");
    
    DataTable table({"id"});
    table.addRow({100});
    result.setData(table);
    
    // 执行重置
    result.reset();
    
    // 验证重置后状态
    EXPECT_EQ(result.getQueryType(), QueryType::UNKNOWN);
    EXPECT_EQ(result.getStatus(), ResultStatus::SUCCESS);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.getSql().empty());
    EXPECT_TRUE(result.getErrorMessage().empty());
    EXPECT_EQ(result.getAffectedRows(), 0);
    EXPECT_EQ(result.getDuration(), std::chrono::milliseconds(0));
    EXPECT_TRUE(result.getData().getRowCount() == 0);  // 数据被清空
    EXPECT_NE(result.getTimestamp(), "2024-01-01 00:00:00");  // 时间戳更新为当前
}
