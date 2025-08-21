#include<model/DataTable.h>
#include <sstream>
#include <iomanip>

DataTable::DataTable(const std::vector<std::string>& columnNames)
{
    for (const auto& name : columnNames)
    {
        addColumn(name);
    }
}

size_t DataTable::getColumnCount() const
{
    return columns.size();
}

size_t DataTable::getRowCount() const
{
    return rows.size();
}

const std::vector<std::string>& DataTable::getColumns() const
{
    return columns;
}

bool DataTable::hasColumn(const std::string& columnName) const
{
    return columnIndices.find(columnName) != columnIndices.end();
}

bool DataTable::addColumn(const std::string& columnName)
{
    // 如果已经存在该列，返回失败
    if (hasColumn(columnName))
    {
        return false;
    }

    // 如果已有数据行，不能添加列（保持结构一致性）
    if (!rows.empty())
    {
        return false;
    }

    columns.push_back(columnName);
    columnIndices[columnName] = columns.size() - 1;
    return true;
}

bool DataTable::addRow(const std::vector<std::any>& rowData)
{
    // 行数据的列数必须与表格列数一致
    if (rowData.size() != columns.size())
    {
        return false;
    }

    rows.push_back(rowData);
    return true;
}

bool DataTable::addRow(const DataRow& dataRow)
{
    if( dataRow.size() != columns.size())
    {
        return false;
    }

}

const std::any& DataTable::getValue(size_t rowIndex, size_t columnIndex) const
{
    if (rowIndex >= rows.size())
    {
        throw std::out_of_range("Row index out of range");
    }

    if (columnIndex >= columns.size())
    {
        throw std::out_of_range("Column index out of range");
    }

    return rows[rowIndex][columnIndex];
}

const std::any& DataTable::getValue(size_t rowIndex, const std::string& columnName) const
{
    auto it = columnIndices.find(columnName);
    if (it == columnIndices.end())
    {
        throw std::invalid_argument("Column not found: " + columnName);
    }

    return getValue(rowIndex, it->second);
}

bool DataTable::setValue(size_t rowIndex, size_t columnIndex, const std::any& value)
{
    if (rowIndex >= rows.size() || columnIndex >= columns.size())
    {
        return false;
    }

    rows[rowIndex][columnIndex] = value;
    return true;
}

bool DataTable::setValue(size_t rowIndex, const std::string& columnName, const std::any& value)
{
    auto it = columnIndices.find(columnName);
    if (it == columnIndices.end())
    {
        return false;
    }

    return setValue(rowIndex, it->second, value);
}

void DataTable::setTableName(const std::string& name)
{
    if(name.empty())
    {
        throw std::invalid_argument("Table name cannot be empty");
    }
    tableName = name;
}

const std::string& DataTable::getTableName() const
{
    return tableName;
}

const std::vector<std::any>& DataTable::getRow(size_t rowIndex) const
{
    if (rowIndex >= rows.size())
    {
        throw std::out_of_range("Row index out of range");
    }

    return rows[rowIndex];
}

void DataTable::clear()
{
    rows.clear();
}

void DataTable::copyFrom(const DataTable& other)
{
    columns = other.columns;
    columnIndices = other.columnIndices;
    rows = other.rows;
}

DataTable DataTable::selectColumns(const std::vector<std::string>& selectedColumns) const
{
    DataTable result;

    // 验证并添加选中的列
    for (const auto& col : selectedColumns)
    {
        if (!hasColumn(col))
        {
            throw std::invalid_argument("Column not found in table: " + col);
        }
        result.addColumn(col);
    }

    // 复制选中列的数据
    for (size_t i = 0; i < getRowCount(); ++i)
    {
        std::vector<std::any> newRow;
        for (const auto& col : selectedColumns)
        {
            newRow.push_back(getValue(i, col));
        }
        result.addRow(newRow);
    }

    return result;
}


void DataRow::addValue(const ValueType& value)
{
    m_values.push_back(value);
}

const DataRow::ValueType& DataRow::operator[](size_t index) const
{
    if (index >= m_values.size())
    {
        throw std::out_of_range("DataRow index out of range");
    }
    return m_values[index];
}

size_t DataRow::size() const
{
    return m_values.size();
}

bool DataRow::empty() const
{
    return m_values.empty();
}

void DataRow::clear()
{
    m_values.clear();
}

std::string DataRow::getValueAsString(size_t index) const
{
    if (index >= m_values.size())
    {
        return "";
    }

    const auto& value = m_values[index];
    std::stringstream ss;

    // 根据实际类型转换为字符串
    if (std::holds_alternative<int>(value))
    {
        ss << std::get<int>(value);
    }
    else if (std::holds_alternative<double>(value))
    {
        ss << std::get<double>(value);
    }
    else if (std::holds_alternative<bool>(value))
    {
        ss << (std::get<bool>(value) ? "true" : "false");
    }
    else if (std::holds_alternative<std::string>(value))
    {
        ss << std::get<std::string>(value);
    }
    else if (std::holds_alternative<std::nullptr_t>(value))
    {
        ss << "NULL";
    }

    return ss.str();
}