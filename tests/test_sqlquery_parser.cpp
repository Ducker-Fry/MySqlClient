#include <gtest/gtest.h>

#include <core/sql_parser.h>
#include <input/inputdata.h>

#include <memory>

class SqlParserTest : public ::testing::Test
{
protected:
    QuerySqlParser parser;

    std::shared_ptr<SqlParseResultQuery> Parse(const std::string& sql)
    {
        InputData input;
        input.setRawData(sql);
        return std::dynamic_pointer_cast<SqlParseResultQuery>(parser.parse(input));
    }
};

TEST_F(SqlParserTest, ParsesSelectStatement)
{
    auto result = Parse("SELECT id, name FROM app.user WHERE status = 'active';");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getOperationType(), SqlType::SELECT);
    EXPECT_EQ(result->getDatabase(), "APP");
    EXPECT_EQ(result->getTable(), "USER");
    EXPECT_EQ(result->getColumns(), std::vector<std::string>({"ID", "NAME"}));
    EXPECT_EQ(result->getWhereClause(), "STATUS = 'ACTIVE'");
}

TEST_F(SqlParserTest, ParsesInsertColumnsCorrectly)
{
    auto result = Parse("INSERT INTO employee (id, name, salary) VALUES (1, 'Alice', 5000);");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getOperationType(), SqlType::INSERT);
    EXPECT_EQ(result->getTable(), "EMPLOYEE");
    EXPECT_EQ(result->getColumns(), std::vector<std::string>({"ID", "NAME", "SALARY"}));
}

TEST_F(SqlParserTest, ParsesUpdateAndDeleteStatements)
{
    auto updateResult = Parse("UPDATE product SET price = 99.9, stock = 100 WHERE id = 5;");
    auto deleteResult = Parse("DELETE FROM archive.logs WHERE created_at < '2023-01-01';");

    ASSERT_NE(updateResult, nullptr);
    ASSERT_NE(deleteResult, nullptr);
    EXPECT_EQ(updateResult->getOperationType(), SqlType::UPDATE);
    EXPECT_EQ(updateResult->getTable(), "PRODUCT");
    EXPECT_EQ(updateResult->getColumns(), std::vector<std::string>({"PRICE", "STOCK"}));
    EXPECT_EQ(updateResult->getWhereClause(), "ID = 5");

    EXPECT_EQ(deleteResult->getOperationType(), SqlType::DELETE);
    EXPECT_EQ(deleteResult->getDatabase(), "ARCHIVE");
    EXPECT_EQ(deleteResult->getTable(), "LOGS");
    EXPECT_EQ(deleteResult->getWhereClause(), "CREATED_AT < '2023-01-01'");
}

TEST_F(SqlParserTest, ParsesCreateTableStatement)
{
    auto result = Parse("CREATE TABLE users (id INT, name TEXT, created_at DATETIME);");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getOperationType(), SqlType::CREATE);
    EXPECT_EQ(result->getTable(), "USERS");
    EXPECT_TRUE(result->getColumns().empty());
}

TEST_F(SqlParserTest, HandlesCommentsWhitespaceAndUnknownSql)
{
    auto result = Parse(R"(
        -- comment
        select /* inline */ age
        from users
        limit 10;
    )");
    auto invalid = Parse("MERGE INTO users USING tmp ON users.id = tmp.id;");

    ASSERT_NE(result, nullptr);
    ASSERT_NE(invalid, nullptr);
    EXPECT_EQ(result->getOperationType(), SqlType::SELECT);
    EXPECT_EQ(result->getColumns(), std::vector<std::string>({"AGE"}));
    EXPECT_EQ(result->getLimitClause(), "10");
    EXPECT_EQ(invalid->getOperationType(), SqlType::UNKNOWN);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
