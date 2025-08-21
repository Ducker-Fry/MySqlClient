#ifndef CSVDATASOURCE_H
#define CSVDATASOURCE_H

#include "IDataSource.h"
#include <fstream>
#include <string>

/**
 * @brief CSV文件数据源实现
 */
class CsvDataSource : public IDataSource
{
public:
    using Ptr = std::shared_ptr<CsvDataSource>;

    CsvDataSource() = default;
    ~CsvDataSource() override;

    /**
     * @brief 连接CSV数据源
     * @param source CSV文件路径
     * @param params 连接参数，支持：
     *        - "delimiter": 字段分隔符(默认',')
     *        - "quote": 引号字符(默认'"')
     *        - "header": 是否包含表头("true"/"false", 默认"true")
     *        - "encoding": 编码格式(预留，暂不支持)
     * @return 连接成功返回true
     */
    bool connect(const std::string& source,
        const std::vector<std::pair<std::string, std::string>>& params = {}) override;

    /**
     * @brief 断开连接
     */
    void disconnect() override;

    /**
     * @brief 执行查询(支持简单的行筛选)
     * @param query 查询条件，格式支持：
     *        - "SELECT *"：查询所有行
     *        - "SELECT * WHERE 列名=值"：简单条件筛选
     * @return 查询结果
     */
    QueryResult query(const std::string& query) override;

    /**
     * @brief 写入CSV文件
     * @param table 待写入的数据表
     * @param target 目标文件路径
     * @return 写入成功返回true
     */
    bool write(const DataTable& table, const std::string& target) override;

    /**
     * @brief 检查是否已连接
     * @return 已连接返回true
     */
    bool isConnected() const override;

    /**
     * @brief 获取数据源类型
     * @return 返回CSV类型
     */
    DataSourceType getType() const override;

    /**
     * @brief 获取数据源描述
     * @return 返回CSV文件路径
     */
    std::string getDescription() const override;

private:
    /**
     * @brief 解析一行CSV数据
     * @param line 输入行字符串
     * @return 解析后的字段列表
     */
    std::vector<std::string> parseCsvLine(const std::string& line);

    /**
     * @brief 将字符串转换为对应的值类型
     * @param str 输入字符串
     * @return 转换后的ValueType
     */
    ValueType convertStringToValue(const std::string& str);

    /**
     * @brief 解析查询条件
     * @param query 查询字符串
     * @param column 输出列名
     * @param value 输出比较值
     * @return 解析成功返回true
     */
    bool parseQueryCondition(const std::string& query, std::string& column, std::string& value);

private:
    std::string m_filePath;       // CSV文件路径
    bool m_connected = false;     // 连接状态
    char m_delimiter = ',';       // 字段分隔符
    char m_quote = '"';           // 引号字符
    bool m_hasHeader = true;      // 是否包含表头
    std::ifstream m_fileStream;   // 文件输入流
};

#endif // CSVDATASOURCE_H