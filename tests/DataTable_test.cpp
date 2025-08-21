#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <any>
#include "model/DataTable.h"

// 测试DataTable的基本功能
TEST(DataTableTest, BasicFunctionality) {
    // 测试构造函数和列管理
    std::vector<std::string> columns = {"id", "name", "age"};
    DataTable table(columns);
    
    EXPECT_EQ(table.getColumnCount(), 3);
    EXPECT_EQ(table.getRowCount(), 0);
    EXPECT_TRUE(table.hasColumn("name"));
    EXPECT_FALSE(table.hasColumn("email"));
    
    // 测试添加行
    std::vector<std::any> row1 = {1, std::string("Alice"), 30};
    std::vector<std::any> row2 = {2, std::string("Bob"), 25};
    
    EXPECT_TRUE(table.addRow(row1));
    EXPECT_TRUE(table.addRow(row2));
    EXPECT_EQ(table.getRowCount(), 2);
    
    // 测试添加不匹配列数的行
    std::vector<std::any> invalidRow = {3, std::string("Charlie")}; // 缺少age列
    EXPECT_FALSE(table.addRow(invalidRow));
    EXPECT_EQ(table.getRowCount(), 2); // 行数不变
}

// 测试数据访问功能
TEST(DataTableTest, DataAccess) {
    std::vector<std::string> columns = {"id", "name", "salary"};
    DataTable table(columns);
    
    // 添加测试数据
    table.addRow({1, std::string("Alice"), 50000.5});
    table.addRow({2, std::string("Bob"), 60000.0});
    
    // 测试按索引访问
    EXPECT_EQ(std::any_cast<int>(table.getValue(0, 0)), 1);
    EXPECT_EQ(std::any_cast<std::string>(table.getValue(0, 1)), "Alice");
    EXPECT_NEAR(std::any_cast<double>(table.getValue(0, 2)), 50000.5, 0.001);
    
    // 测试按列名访问
    EXPECT_EQ(std::any_cast<int>(table.getValue(1, "id")), 2);
    EXPECT_EQ(std::any_cast<std::string>(table.getValue(1, "name")), "Bob");
    EXPECT_NEAR(std::any_cast<double>(table.getValue(1, "salary")), 60000.0, 0.001);
    
    // 测试获取整行
    const auto& row = table.getRow(0);
    EXPECT_EQ(row.size(), 3);
    EXPECT_EQ(std::any_cast<int>(row[0]), 1);
}

// 测试异常情况
TEST(DataTableTest, ExceptionHandling) {
    std::vector<std::string> columns = {"id", "name"};
    DataTable table(columns);
    table.addRow({1, std::string("Alice")});
    
    // 测试访问不存在的行
    EXPECT_THROW(table.getValue(100, 0), std::out_of_range);
    
    // 测试访问不存在的列索引
    EXPECT_THROW(table.getValue(0, 100), std::out_of_range);
    
    // 测试访问不存在的列名
    EXPECT_THROW(table.getValue(0, "invalid_column"), std::invalid_argument);
}

// 测试修改数据功能
TEST(DataTableTest, DataModification) {
    std::vector<std::string> columns = {"id", "name"};
    DataTable table(columns);
    table.addRow({1, std::string("Alice")});
    
    // 测试按索引修改
    EXPECT_TRUE(table.setValue(0, 1, std::string("Alicia")));
    EXPECT_EQ(std::any_cast<std::string>(table.getValue(0, 1)), "Alicia");
    
    // 测试按列名修改
    EXPECT_TRUE(table.setValue(0, "name", std::string("Alice")));
    EXPECT_EQ(std::any_cast<std::string>(table.getValue(0, "name")), "Alice");
    
    // 测试修改不存在的单元格
    EXPECT_FALSE(table.setValue(100, 0, 2));
    EXPECT_FALSE(table.setValue(0, "invalid_column", 2));
}

// 测试列操作限制
TEST(DataTableTest, ColumnOperations) {
    std::vector<std::string> columns = {"id", "name"};
    DataTable table(columns);
    
    // 测试添加新列（无数据行时）
    EXPECT_TRUE(table.addColumn("age"));
    EXPECT_EQ(table.getColumnCount(), 3);
    
    // 添加一行数据
    table.addRow({1, std::string("Alice"), 30});
    
    // 测试添加新列（有数据行时，应失败）
    EXPECT_FALSE(table.addColumn("email"));
    EXPECT_EQ(table.getColumnCount(), 3);
    
    // 测试添加重复列
    EXPECT_FALSE(table.addColumn("name"));
}

// 测试表格复制和列筛选
TEST(DataTableTest, CopyAndSelect) {
    std::vector<std::string> columns = {"id", "name", "age", "salary"};
    DataTable table(columns);
    
    // 添加测试数据
    table.addRow({1, std::string("Alice"), 30, 50000.5});
    table.addRow({2, std::string("Bob"), 25, 60000.0});
    
    // 测试复制表格
    DataTable copy;
    copy.copyFrom(table);
    EXPECT_EQ(copy.getColumnCount(), table.getColumnCount());
    EXPECT_EQ(copy.getRowCount(), table.getRowCount());
    EXPECT_EQ(std::any_cast<std::string>(copy.getValue(0, "name")), "Alice");
    
    // 测试筛选列
    std::vector<std::string> selectedCols = {"name", "salary"};
    DataTable filtered = table.selectColumns(selectedCols);
    
    EXPECT_EQ(filtered.getColumnCount(), 2);
    EXPECT_EQ(filtered.getRowCount(), 2);
    EXPECT_TRUE(filtered.hasColumn("name"));
    EXPECT_TRUE(filtered.hasColumn("salary"));
    EXPECT_FALSE(filtered.hasColumn("id"));
    EXPECT_NEAR(std::any_cast<double>(filtered.getValue(1, "salary")), 60000.0, 0.001);
    
    // 测试筛选不存在的列（应抛出异常）
    std::vector<std::string> invalidCols = {"name", "invalid"};
    EXPECT_THROW(table.selectColumns(invalidCols), std::invalid_argument);
}

// 测试清空功能
TEST(DataTableTest, ClearFunctionality) {
    std::vector<std::string> columns = {"id", "name"};
    DataTable table(columns);
    table.addRow({1, std::string("Alice")});
    table.addRow({2, std::string("Bob")});
    
    EXPECT_EQ(table.getRowCount(), 2);
    table.clear();
    EXPECT_EQ(table.getRowCount(), 0);
    EXPECT_EQ(table.getColumnCount(), 2); // 清空行后列应保留
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}