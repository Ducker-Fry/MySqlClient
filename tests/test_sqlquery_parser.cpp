#include <gtest/gtest.h>
#include <core/sql_parser.h>
#include <input/inputdata.h>
#include <memory>

// 测试 fixture，提供通用解析器和输入创建方法
class SqlParserDmlTest : public ::testing::Test {
protected:
    QuerySqlParser parser;

    // 创建InputData对象（假设InputData有setRawData方法）
    InputData createInput(const std::string& sql) {
        InputData input;
        input.setRawData(sql); // 假设InputData支持设置原始SQL字符串
        return input;
    }

    // 通用解析方法，返回查询结果指针
    std::shared_ptr<SqlParseResultQuery> parseSql(const std::string& sql) {
        auto input = createInput(sql);
        auto result = parser.parse(input);
        return std::dynamic_pointer_cast<SqlParseResultQuery>(result);
    }
};

// 测试SELECT语句解析
TEST_F(SqlParserDmlTest, ParseSelectStatements) {
    // 基本SELECT测试
    {
        int stop = 1;
        auto result = parseSql("SELECT id, name FROM user;");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::SELECT);
        EXPECT_EQ(result->getTable(), "USER");
        EXPECT_EQ(result->getDatabase(), "");
        EXPECT_EQ(result->getColumns(), std::vector<std::string>({"ID", "NAME"}));
        EXPECT_EQ(result->getWhereClause(), "");
    }

    // 带完整子句的SELECT测试
    {
        auto result = parseSql(R"(
            SELECT age, COUNT(*) AS cnt 
            FROM mydb.student 
            WHERE age > 18 
            GROUP BY age 
            HAVING cnt > 5 
            ORDER BY age DESC 
            LIMIT 10;
        )");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::SELECT);
        EXPECT_EQ(result->getDatabase(), "MYDB");
        EXPECT_EQ(result->getTable(), "STUDENT");
        EXPECT_EQ(result->getColumns(), std::vector<std::string>({"AGE", "COUNT(*) AS CNT"}));
        EXPECT_EQ(result->getWhereClause(), "AGE > 18 ");
        EXPECT_EQ(result->getGroupByColumns(), std::vector<std::string>({"AGE"}));
        EXPECT_EQ(result->getHavingClause(), "CNT > 5 ");
        EXPECT_EQ(result->getOrderByClause(), "AGE DESC ");
        EXPECT_EQ(result->getLimitClause(), "10");
    }

    // 带注释和大小写混合的SELECT
    {
        auto result = parseSql(R"(
            sElEcT /* 这是块注释 */ id, email 
            FROM /* 数据库名 */ app.user -- 这是行注释
            WHERE status = 'active'
        )");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::SELECT);
        EXPECT_EQ(result->getDatabase(), "APP");
        EXPECT_EQ(result->getTable(), "USER");
        EXPECT_EQ(result->getColumns(), std::vector<std::string>({"ID", "EMAIL"}));
        EXPECT_EQ(result->getWhereClause(), "STATUS = 'ACTIVE' ");
    }
}

// 测试INSERT语句解析
TEST_F(SqlParserDmlTest, ParseInsertStatements) {
    // 基本INSERT测试
    {
        auto result = parseSql("INSERT INTO employee (id, name, salary) VALUES (1, 'Alice', 5000);");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::INSERT);
        EXPECT_EQ(result->getTable(), "EMPLOYEE");
        EXPECT_EQ(result->getColumns(), std::vector<std::string>({"EMPLOYEE (ID", "NAME", "SALARY)"}));
    }

    // 带数据库名的INSERT
    {
        auto result = parseSql("INSERT INTO companydb.department (dept_id, dept_name) VALUES (101, 'HR');");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::INSERT);
        EXPECT_EQ(result->getDatabase(), "COMPANYDB");
        EXPECT_EQ(result->getTable(), "DEPARTMENT");
        EXPECT_EQ(result->getColumns(), std::vector<std::string>({"COMPANYDB.DEPARTMENT (DEPT_ID", "DEPT_NAME)"}));
    }
}

// 测试UPDATE语句解析
TEST_F(SqlParserDmlTest, ParseUpdateStatements) {
    // 基本UPDATE测试
    {
        auto result = parseSql("UPDATE product SET price = 99.9, stock = 100 WHERE id = 5;");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::UPDATE);
        EXPECT_EQ(result->getTable(), "PRODUCT");
        EXPECT_EQ(result->getWhereClause(), "ID = 5");
    }

    // 带数据库和复杂条件的UPDATE
    {
        auto result = parseSql(R"(
            UPDATE shopdb.order
            SET status = 'shipped', ship_time = NOW()
            WHERE order_date < '2024-01-01' AND total > 1000
        )");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::UPDATE);
        EXPECT_EQ(result->getDatabase(), "SHOPDB");
        EXPECT_EQ(result->getTable(), "ORDER");
        EXPECT_EQ(result->getWhereClause(), "ORDER_DATE < '2024-01-01' AND TOTAL > 1000 ");
    }
}

// 测试DELETE语句解析
TEST_F(SqlParserDmlTest, ParseDeleteStatements) {
    // 基本DELETE测试
    {
        auto result = parseSql("DELETE FROM log WHERE create_time < '2023-01-01';");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::DELETE);
        EXPECT_EQ(result->getTable(), "LOG");
        EXPECT_EQ(result->getWhereClause(), "CREATE_TIME < '2023-01-01'");
    }

    // 带数据库和注释的DELETE
    {
        auto result = parseSql(R"(
            DELETE FROM /* 清理过期数据 */ archived.user
            WHERE last_login < '2022-01-01' -- 超过2年未登录
        )");
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->getOperationType(), SqlType::DELETE);
        EXPECT_EQ(result->getDatabase(), "ARCHIVED");
        EXPECT_EQ(result->getTable(), "USER");
        EXPECT_EQ(result->getWhereClause(), "LAST_LOGIN < '2022-01-01' ");
    }
}
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}