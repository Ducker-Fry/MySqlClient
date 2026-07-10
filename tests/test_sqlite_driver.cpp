#include <gtest/gtest.h>

#include <database/sqlite_driver.h>

#include <filesystem>

using namespace sql::sqlite;
namespace fs = std::filesystem;

class SqliteDriverTest : public testing::Test
{
protected:
    fs::path tempDir = "test_temp_sqlite_db";
    fs::path dbPath = tempDir / "app.db";

    void SetUp() override
    {
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }
        fs::create_directories(tempDir);
    }

    void TearDown() override
    {
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }
    }
};

TEST_F(SqliteDriverTest, ConnectCreateAndQueryCrudData)
{
    auto connection = Driver::getInstance().connect(dbPath.string());
    ASSERT_TRUE(connection->isValid());

    auto statement = connection->createStatement();
    EXPECT_TRUE(statement->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);"));
    EXPECT_EQ(statement->executeUpdate("INSERT INTO users (name, age) VALUES ('Alice', 25);"), 1U);
    EXPECT_EQ(statement->executeUpdate("INSERT INTO users (name, age) VALUES ('Bob', 30);"), 1U);
    EXPECT_EQ(statement->executeUpdate("UPDATE users SET age = 31 WHERE name = 'Bob';"), 1U);

    auto result = statement->executeQuery("SELECT id, name, age FROM users WHERE age >= 25 ORDER BY id;");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->getInt("id"), 1);
    EXPECT_EQ(result->getString("name"), "Alice");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->getInt("id"), 2);
    EXPECT_EQ(result->getString("name"), "Bob");
    EXPECT_EQ(result->getInt("age"), 31);
    EXPECT_FALSE(result->next());
}

TEST_F(SqliteDriverTest, PreparedStatementsAndReconnectWork)
{
    {
        auto connection = Driver::getInstance().connect(dbPath.string());
        auto statement = connection->createStatement();
        ASSERT_TRUE(statement->execute("CREATE TABLE items (id INTEGER, title TEXT, price REAL);"));

        auto insert = connection->prepareStatement("INSERT INTO items (id, title, price) VALUES (?, ?, ?);");
        insert->setInt(1, 1);
        insert->setString(2, "Laptop");
        insert->setFloat(3, 5999.9f);
        EXPECT_EQ(insert->executeUpdate(), 1U);
    }

    auto reopened = Driver::getInstance().connect(dbPath.string());
    auto query = reopened->createStatement()->executeQuery("SELECT id, title, price FROM items;");
    ASSERT_TRUE(query->next());
    EXPECT_EQ(query->getInt("id"), 1);
    EXPECT_EQ(query->getString("title"), "Laptop");
}

TEST_F(SqliteDriverTest, InvalidSqlThrows)
{
    auto connection = Driver::getInstance().connect(dbPath.string());
    auto statement = connection->createStatement();

    EXPECT_THROW(statement->executeQuery("SELECT * FROM missing_table;"), SQLiteException);
    EXPECT_THROW(statement->executeUpdate("INSERT BROKEN SQL"), SQLiteException);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
