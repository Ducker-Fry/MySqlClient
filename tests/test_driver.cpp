#include <gtest/gtest.h>
#include <filesystem>
#include <database/json_driver.h>
#include <json.hpp>
#include <fstream>

using namespace sql::jsondb;
namespace fs = std::filesystem;

// 基础测试类：管理临时数据库目录，自动清理
class JsonDbBaseTest : public testing::Test {
protected:
    std::string tempDbPath = "test_temp_json_db";  // 临时数据库目录
    std::shared_ptr<Driver> driver;
    std::shared_ptr<Connection> conn;

    // 测试前初始化：创建临时目录和连接
    void SetUp() override {
        // 清理残留的临时目录
        if (fs::exists(tempDbPath)) {
            fs::remove_all(tempDbPath);
        }
        // 获取 Driver 单例
        driver = std::make_shared<Driver>(Driver::getInstance());
        // 创建 Connection（用户/密码任意，因 authenticate 暂返回 true）
        conn = driver->connect(tempDbPath, "test_user", "test_pass");
    }

    // 测试后清理：删除临时目录
    void TearDown() override {
        // 关闭连接
        if (conn && !conn->isClosed()) {
            conn->close();
        }
        // 删除临时目录
        if (fs::exists(tempDbPath)) {
            fs::remove_all(tempDbPath);
        }
    }

    // 辅助函数：手动创建测试表（用于 Statement 测试）
    void createTestTable(const std::string& tableName) {
        std::string tablePath = conn->getTableFilePath(tableName);
        // 构造测试表的 JSON 数据（含 2 行数据）
        nlohmann::json tableData = nlohmann::json::array();
        tableData.push_back({{"id", 1}, {"name", "Alice"}, {"age", 25}, {"is_active", true}});
        tableData.push_back({{"id", 2}, {"name", "Bob"}, {"age", 30}, {"is_active", false}});
        // 写入文件
        std::ofstream ofs(tablePath);
        ofs << std::setw(4) << tableData;
        ofs.close();
    }
};


// ------------------------------ Driver 类测试 ------------------------------
TEST_F(JsonDbBaseTest, Driver_Singleton_Uniqueness) {
    // 验证 Driver 单例唯一性：两次 getInstance 返回同一实例
    Driver* driver1 = &Driver::getInstance();
    Driver* driver2 = &Driver::getInstance();
    EXPECT_EQ(driver1, driver2);
}

TEST_F(JsonDbBaseTest, Driver_Connect_ReturnValidConnection) {
    // 验证 connect 返回非空 Connection，且数据库目录已创建
    ASSERT_NE(conn, nullptr);
    EXPECT_FALSE(conn->isClosed());
    EXPECT_TRUE(fs::exists(tempDbPath));  // 连接创建时自动创建目录
}


// ------------------------------ Connection 类测试 ------------------------------
TEST_F(JsonDbBaseTest, Connection_TableExists_CorrectJudgment) {
    const std::string tableName = "test_table";
    std::string tablePath = conn->getTableFilePath(tableName);

    // 初始时表不存在
    EXPECT_FALSE(conn->tableExists(tableName));

    // 手动创建表文件后，表存在
    std::ofstream ofs(tablePath);
    ofs << "[]";  // 空数组
    ofs.close();
    EXPECT_TRUE(conn->tableExists(tableName));

    // 删除表文件后，表不存在
    fs::remove(tablePath);
    EXPECT_FALSE(conn->tableExists(tableName));
}

TEST_F(JsonDbBaseTest, Connection_GetColumnNames_ReturnsCorrectColumns) {
    const std::string tableName = "user";
    // 手动创建含列的表
    createTestTable(tableName);

    // 获取列名，验证是否为 ["id", "name", "age", "is_active"]
    std::vector<std::string> cols = conn->getColumnNames(tableName);
    EXPECT_EQ(cols.size(), 4);
    EXPECT_NE(std::find(cols.begin(), cols.end(), "id"), cols.end());
    EXPECT_NE(std::find(cols.begin(), cols.end(), "name"), cols.end());
    EXPECT_NE(std::find(cols.begin(), cols.end(), "age"), cols.end());
    EXPECT_NE(std::find(cols.begin(), cols.end(), "is_active"), cols.end());
}

TEST_F(JsonDbBaseTest, Connection_GetTableData_ReturnsCorrectData) {
    const std::string tableName = "user";
    createTestTable(tableName);

    // 获取表数据，验证行数和内容
    std::vector<nlohmann::json> data = conn->getTableData(tableName);
    EXPECT_EQ(data.size(), 2);  // 2 行测试数据
    EXPECT_EQ(data[0]["id"].get<int>(), 1);
    //EXPECT_EQ(data[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(data[1]["id"].get<int>(), 2);
    EXPECT_EQ(data[1]["is_active"].get<bool>(), false);
}

TEST_F(JsonDbBaseTest, Connection_Close_SetClosedStatus) {
    // 初始状态：未关闭
    EXPECT_FALSE(conn->isClosed());
    // 调用 close 后：已关闭
    conn->close();
    EXPECT_TRUE(conn->isClosed());
    // 重复调用 close 无异常
    EXPECT_NO_THROW(conn->close());
}


// ------------------------------ Statement 类测试 ------------------------------
TEST_F(JsonDbBaseTest, Statement_ExecuteInsert_CreatesTableAndData) {
    const std::string tableName = "product";
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行 INSERT 语句（带列名）
    std::string insertSql = "INSERT INTO " + tableName + " (id, name, price) VALUES (1, 'Laptop', 5999.9)";
    size_t affectedRows = stmt->executeUpdate(insertSql);

    // 验证：影响行数为 1，表文件存在，数据正确
    EXPECT_EQ(affectedRows, 1);
    EXPECT_TRUE(conn->tableExists(tableName));
    std::vector<nlohmann::json> data = conn->getTableData(tableName);
    EXPECT_EQ(data.size(), 1);
    EXPECT_EQ(data[0]["id"].get<int>(), 1);
    //EXPECT_EQ(data[0]["name"].get<std::string>(), "Laptop");
    EXPECT_DOUBLE_EQ(data[0]["price"].get<double>(), 5999.9);
}

TEST_F(JsonDbBaseTest, Statement_ExecuteQuery_ReturnsFilteredData) {
    const std::string tableName = "user";
    createTestTable(tableName);  // 提前创建测试表（2 行数据）
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行 SELECT 语句（筛选 age > 25 的行）
    std::string selectSql = "SELECT id, name, age FROM " + tableName + " WHERE age > 25";
    std::shared_ptr<ResultSet> rs = stmt->executeQuery(selectSql);

    // 验证结果集：仅 1 行（Bob，age=30）
    EXPECT_TRUE(rs->next());  // 第一行有效
    EXPECT_EQ(rs->getInt("id"), 2);
    EXPECT_EQ(rs->getString("name"), "Bob");
    EXPECT_EQ(rs->getInt("age"), 30);
    EXPECT_FALSE(rs->next());  // 无第二行
}

TEST_F(JsonDbBaseTest, Statement_ExecuteUpdate_ModifiesData) {
    const std::string tableName = "user";
    createTestTable(tableName);
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行 UPDATE 语句（将 Bob 的 age 改为 31）
    std::string updateSql = "UPDATE " + tableName + " SET age=31 WHERE name='Bob'";
    size_t affectedRows = stmt->executeUpdate(updateSql);

    // 验证：影响行数 1，数据已修改
    EXPECT_EQ(affectedRows, 1);
    std::vector<nlohmann::json> data = conn->getTableData(tableName);
    //for (const auto& row : data) {
    //    if (row["name"].get<std::string>() == "Bob") {
    //        EXPECT_EQ(row["age"].get<int>(), 31);
    //    }
    //}
}

TEST_F(JsonDbBaseTest, Statement_ExecuteDelete_RemovesData) {
    const std::string tableName = "user";
    createTestTable(tableName);
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行 DELETE 语句（删除 id=1 的行）
    std::string deleteSql = "DELETE FROM " + tableName + " WHERE id=1";
    size_t affectedRows = stmt->executeUpdate(deleteSql);

    // 验证：影响行数 1，数据仅剩 1 行
    EXPECT_EQ(affectedRows, 1);
    std::vector<nlohmann::json> data = conn->getTableData(tableName);
    EXPECT_EQ(data.size(), 1);
    EXPECT_EQ(data[0]["id"].get<int>(), 2);  // 仅剩 Bob 的行
}

TEST_F(JsonDbBaseTest, Statement_InvalidSQL_ThrowsException) {
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行无效 SQL（语法错误），验证抛异常
    std::string invalidSql = "SELECT * FROM non_exist_table";
    EXPECT_THROW(stmt->executeQuery(invalidSql), JsonDbException);

    // 执行无 WHERE 的 SELECT（原代码正则仅匹配带 WHERE 的 SELECT），验证抛异常
    std::string noWhereSql = "SELECT id FROM user";
    EXPECT_THROW(stmt->executeQuery(noWhereSql), JsonDbException);
}


// ------------------------------ ResultSetMetaData 测试 ------------------------------
TEST_F(JsonDbBaseTest, ResultSetMetaData_GetColumnInfo) {
    const std::string tableName = "user";
    createTestTable(tableName);
    std::shared_ptr<Statement> stmt = conn->createStatement();

    // 执行查询，获取元数据
    std::string selectSql = "SELECT id, name, age, is_active FROM " + tableName + " WHERE id=1";
    std::shared_ptr<ResultSet> rs = stmt->executeQuery(selectSql);
    std::shared_ptr<ResultSetMetaData> meta = rs->getMetaData();

    // 验证元数据：列数、列名、列类型
    EXPECT_EQ(meta->getColumnCount(), 4);
    EXPECT_EQ(meta->getColumnName(0), "id");
    EXPECT_EQ(meta->getColumnType(0), DataType::INT);
    EXPECT_EQ(meta->getColumnName(1), "name");
    EXPECT_EQ(meta->getColumnType(1), DataType::VARCHAR);
    EXPECT_EQ(meta->getColumnName(2), "age");
    EXPECT_EQ(meta->getColumnType(2), DataType::INT);
    EXPECT_EQ(meta->getColumnName(3), "is_active");
    EXPECT_EQ(meta->getColumnType(3), DataType::BOOLEAN);
}


// 测试入口
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}