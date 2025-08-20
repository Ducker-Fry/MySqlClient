#ifndef DATATABLE_H
#define DATATABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <stdexcept>

class DataTable
{
private:
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

    // 获取指定单元格的值
    const std::any& getValue(size_t rowIndex, size_t columnIndex) const;
    const std::any& getValue(size_t rowIndex, const std::string& columnName) const;

    // 设置指定单元格的值
    bool setValue(size_t rowIndex, size_t columnIndex, const std::any& value);
    bool setValue(size_t rowIndex, const std::string& columnName, const std::any& value);

    // 获取整行数据
    const std::vector<std::any>& getRow(size_t rowIndex) const;

    // 清空表格数据
    void clear();

    // 从另一个DataTable复制数据
    void copyFrom(const DataTable& other);

    // 按列筛选数据（返回新的DataTable）
    DataTable selectColumns(const std::vector<std::string>& selectedColumns) const;
};

#endif // DATATABLE_H
