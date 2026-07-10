#include <gtest/gtest.h>

#include <database/json_driver.h>
#include <json.hpp>

#include <filesystem>
#include <fstream>

using namespace sql::jsondb;
namespace fs = std::filesystem;

class JsonDbBaseTest : public testing::Test
{
protected:
    std::string tempDbPath = "test_temp_json_db";
    std::shared_ptr<Driver> driver;
    std::shared_ptr<Connection> conn;

    void SetUp() override
    {
        if (fs::exists(tempDbPath))
        {
            fs::remove_all(tempDbPath);
        }

        driver = std::make_shared<Driver>(Driver::getInstance());
        conn = driver->connect(tempDbPath, "test_user", "test_pass");
    }

    void TearDown() override
    {
        if (conn && !conn->isClosed())
        {
            conn->close();
        }

        if (fs::exists(tempDbPath))
        {
            fs::remove_all(tempDbPath);
        }
    }

    void CreateSeedTable(const std::string& tableName)
    {
        const std::string tablePath = conn->getTableFilePath(tableName);
        nlohmann::json tableData = nlohmann::json::array();
        tableData.push_back({{"id", 1}, {"name", "Alice"}, {"age", 25}, {"is_active", true}});
        tableData.push_back({{"id", 2}, {"name", "Bob"}, {"age", 30}, {"is_active", false}});

        std::ofstream tableFile(tablePath);
        tableFile << std::setw(2) << tableData;
        tableFile.close();

        std::ofstream schemaFile(fs::path(tempDbPath) / (tableName + ".schema.json"));
        schemaFile << std::setw(2) << nlohmann::json::array({"id", "name", "age", "is_active"});
    }
};

TEST_F(JsonDbBaseTest, DriverSingletonAndConnectionWork)
{
    Driver* driver1 = &Driver::getInstance();
    Driver* driver2 = &Driver::getInstance();

    EXPECT_EQ(driver1, driver2);
    ASSERT_NE(conn, nullptr);
    EXPECT_FALSE(conn->isClosed());
    EXPECT_TRUE(fs::exists(tempDbPath));
}

TEST_F(JsonDbBaseTest, CreateTableCreatesDataAndSchemaFiles)
{
    auto stmt = conn->createStatement();

    ASSERT_TRUE(stmt->executeCreate("CREATE TABLE product (id INT, name TEXT, price FLOAT);"));
    EXPECT_TRUE(conn->tableExists("product"));
    EXPECT_TRUE(fs::exists(fs::path(tempDbPath) / "product.schema.json"));
    EXPECT_EQ(conn->getColumnNames("product"), std::vector<std::string>({"id", "name", "price"}));
}

TEST_F(JsonDbBaseTest, InsertAndSelectSupportProjectionAndWhere)
{
    CreateSeedTable("user");
    auto stmt = conn->createStatement();

    auto allRows = stmt->executeQuery("SELECT id, name FROM user;");
    ASSERT_TRUE(allRows->next());
    EXPECT_EQ(allRows->getInt("id"), 1);
    EXPECT_EQ(allRows->getString("name"), "Alice");
    ASSERT_TRUE(allRows->next());
    EXPECT_EQ(allRows->getInt("id"), 2);
    EXPECT_EQ(allRows->getString("name"), "Bob");
    EXPECT_FALSE(allRows->next());

    auto filteredRows = stmt->executeQuery("SELECT id, age FROM user WHERE age > 25;");
    ASSERT_TRUE(filteredRows->next());
    EXPECT_EQ(filteredRows->getInt("id"), 2);
    EXPECT_EQ(filteredRows->getInt("age"), 30);
    EXPECT_FALSE(filteredRows->next());
}

TEST_F(JsonDbBaseTest, UpdateDeleteAndMultiInsertPersistCorrectly)
{
    CreateSeedTable("user");
    auto stmt = conn->createStatement();

    EXPECT_EQ(
        stmt->executeUpdate(
            "INSERT INTO user (id, name, age, is_active) VALUES "
            "(3, 'Charlie', 32, true), (4, 'Dana', 28, false);"),
        2U);
    EXPECT_EQ(stmt->executeUpdate("UPDATE user SET age = 31 WHERE name = 'Bob';"), 1U);
    EXPECT_EQ(stmt->executeUpdate("DELETE FROM user WHERE id = 1;"), 1U);

    const std::vector<nlohmann::json> data = conn->getTableData("user");
    ASSERT_EQ(data.size(), 3U);
    EXPECT_EQ(data[0]["id"].get<int>(), 2);
    EXPECT_EQ(data[0]["age"].get<int>(), 31);
    EXPECT_EQ(data[1]["name"].get<std::string>(), "Charlie");
    EXPECT_EQ(data[2]["name"].get<std::string>(), "Dana");
}

TEST_F(JsonDbBaseTest, ResultSetMetadataMatchesSelectedColumns)
{
    CreateSeedTable("user");
    auto stmt = conn->createStatement();

    auto result = stmt->executeQuery("SELECT id, is_active FROM user WHERE id = 1;");
    auto metaData = result->getMetaData();

    ASSERT_EQ(metaData->getColumnCount(), 2U);
    EXPECT_EQ(metaData->getColumnName(0), "id");
    EXPECT_EQ(metaData->getColumnType(0), DataType::INT);
    EXPECT_EQ(metaData->getColumnName(1), "is_active");
    EXPECT_EQ(metaData->getColumnType(1), DataType::BOOLEAN);
}

TEST_F(JsonDbBaseTest, InvalidSqlAndMissingColumnsThrow)
{
    CreateSeedTable("user");
    auto stmt = conn->createStatement();

    EXPECT_THROW(stmt->executeQuery("SELECT missing FROM user;"), JsonDbException);
    EXPECT_THROW(stmt->executeUpdate("UPSERT INTO user VALUES (1);"), JsonDbException);
    EXPECT_THROW(stmt->executeQuery("SELECT * FROM user WHERE missing = 1;"), JsonDbException);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
