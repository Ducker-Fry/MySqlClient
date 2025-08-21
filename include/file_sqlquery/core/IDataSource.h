#ifndef IDATASOURCE_H
#define IDATASOURCE_H

#include <string>
#include <vector>
#include <memory>
#include "model/DataTable.h"
#include "model/QueryResult.h"

// 数据源类型枚举（标识不同的数据源实现）
enum class DataSourceType
{
    UNKNOWN,
    CSV,       // CSV文件数据源
    DATABASE,  // 数据库数据源（预留）
    EXCEL,     // Excel文件数据源（预留）
    JSON       // JSON文件数据源（预留）
};

/**
 * @brief 数据源抽象接口，定义所有数据源的统一访问规范
 */
class IDataSource
{
public:
    using Ptr = std::shared_ptr<IDataSource>;  // 智能指针类型，简化资源管理

    virtual ~IDataSource() = default;  // 纯虚析构函数，确保派生类正确析构

    /**
     * @brief 连接数据源
     * @param source 数据源标识（如文件路径、数据库连接字符串）
     * @param params 额外连接参数（如编码格式、分隔符等，键值对形式）
     * @return 连接成功返回true，否则返回false
     */
    virtual bool connect(const std::string& source,
        const std::vector<std::pair<std::string, std::string>>& params = {}) = 0;

    /**
     * @brief 断开与数据源的连接
     */
    virtual void disconnect() = 0;

    /**
     * @brief 执行查询操作（针对支持查询的数据源）
     * @param query 查询语句（如SQL、或数据源支持的查询格式）
     * @return 封装查询结果的QueryResult对象
     */
    virtual QueryResult query(const std::string& query) = 0;

    /**
     * @brief 执行写入操作（针对支持写入的数据源）
     * @param table 待写入的数据表
     * @param target 目标位置（如表名、文件路径）
     * @return 写入成功返回true，否则返回false
     */
    virtual bool write(const DataTable& table, const std::string& target) = 0;

    /**
     * @brief 检查数据源是否已连接
     * @return 已连接返回true，否则返回false
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief 获取数据源类型
     * @return 数据源类型枚举值
     */
    virtual DataSourceType getType() const = 0;

    /**
     * @brief 获取数据源描述信息（如文件路径、数据库名称）
     * @return 描述字符串
     */
    virtual std::string getDescription() const = 0;
};

#endif // IDATASOURCE_H