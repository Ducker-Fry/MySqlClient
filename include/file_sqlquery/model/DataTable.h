#ifndef DATATABLE_H
#define DATATABLE_H

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <any>
#include <stdexcept>

class DataRow; // 前向声明

class DataTable
{
private:
    // 定义支持的数据类型变体
    using ValueType = std::variant<int, double, bool, std::string, std::nullptr_t>;
    // 表名
    std::string tableName;
    // 列名
    std::vector<std::string> columns;
    // 列名到索引的映射，用于快速查找
    std::unordered_map<std::string, size_t> columnIndices;
    // 存储数据行，每行是一个值的集合
    std::vector<std::vector<std::any>> rows;

public:
    // 构造函数
    DataTable() = default;
    explicit DataTable(const std::vector<std::string>& columnNames);

    // 获取列数
    size_t getColumnCount() const;

    // 获取行数
    size_t getRowCount() const;

    // 获取所有列名
    const std::vector<std::string>& getColumns() const;

    // 检查列是否存在
    bool hasColumn(const std::string& columnName) const;

    // 添加列（只能在没有数据行时添加）
    bool addColumn(const std::string& columnName);

    // 添加行（必须与列数匹配）
    bool addRow(const std::vector<std::any>& rowData);
    bool addRow(const DataRow& dataRow);

    // 获取指定单元格的值
    const std::any& getValue(size_t rowIndex, size_t columnIndex) const;
    const std::any& getValue(size_t rowIndex, const std::string& columnName) const;

    // 设置指定单元格的值
    bool setValue(size_t rowIndex, size_t columnIndex, const std::any& value);
    bool setValue(size_t rowIndex, const std::string& columnName, const std::any& value);

    // 设置表名
    void setTableName(const std::string& name);

    // 获取表名
    const std::string& getTableName() const;

    // 获取整行数据
    const std::vector<std::any>& getRow(size_t rowIndex) const;

    // 清空表格数据
    void clear();

    // 从另一个DataTable复制数据
    void copyFrom(const DataTable& other);

    // 按列筛选数据（返回新的DataTable）
    DataTable selectColumns(const std::vector<std::string>& selectedColumns) const;
};


/**
 * @brief 数据行类，用于存储一行数据的多个字段值
 * 支持多种数据类型（int、double、bool、string、nullptr）
 */
class DataRow
{
public:
    // 定义支持的数据类型变体
    using ValueType = std::variant<int, double, bool, std::string, std::nullptr_t>;

    DataRow() = default;
    ~DataRow() = default;

    /**
     * @brief 添加字段值到行中
     * @param value 字段值（支持int、double、bool、string、nullptr）
     */
    void addValue(const ValueType& value);

    /**
     * @brief 获取指定索引的字段值
     * @param index 字段索引（从0开始）
     * @return 字段值引用
     * @throw std::out_of_range 索引越界时抛出
     */
    const ValueType& operator[](size_t index) const;

    /**
     * @brief 获取行中字段的数量
     * @return 字段数量
     */
    size_t size() const;

    /**
     * @brief 检查行是否为空（无任何字段）
     * @return 空行返回true，否则返回false
     */
    bool empty() const;

    /**
     * @brief 清空行中所有字段
     */
    void clear();

    /**
     * @brief 获取字段值的字符串表示（用于调试或显示）
     * @param index 字段索引
     * @return 字符串形式的值
     */
    std::string getValueAsString(size_t index) const;

private:
    std::vector<ValueType> m_values;  // 存储行中所有字段的值
};

#endif // DATATABLE_H
