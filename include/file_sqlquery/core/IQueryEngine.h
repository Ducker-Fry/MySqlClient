#ifndef IQUERYENGINE_H
#define IQUERYENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <any>
#include "model/QueryResult.h"
#include "model/DataTable.h"
#include "IDataSource.h"
#include<file_sqlquery/QueryType.h>


// 表结构元数据（列信息）
struct ColumnSchema
{
    std::string name;        // 列名
    std::string type;        // 数据类型（如INT、VARCHAR）
    bool isPrimaryKey;       // 是否为主键
    bool isNullable;         // 是否允许为空
};

// 表元数据（包含表名和列结构）
struct TableSchema
{
    std::string tableName;                   // 表名
    std::vector<ColumnSchema> columns;       // 列结构列表
};

/**
 * @brief 查询引擎抽象接口，定义查询处理的核心规范
 */
class IQueryEngine
{
public:
    using Ptr = std::shared_ptr<IQueryEngine>;  // 智能指针类型

    virtual ~IQueryEngine() = default;  // 纯虚析构函数

    /**
     * @brief 设置数据源（查询引擎依赖数据源执行实际操作）
     * @param dataSource 数据源智能指针（实现IDataSource接口）
     */
    virtual void setDataSource(IDataSource::Ptr dataSource) = 0;

    /**
     * @brief 执行查询操作（核心方法）
     * @param query 原始查询语句（如SQL、简化查询语法）
     * @param params 查询参数（用于参数化查询，防止注入）
     * @return 封装查询结果的QueryResult对象
     */
    virtual QueryResult executeQuery(const std::string& query,
        const std::vector<std::any>& params = {}) = 0;

    /**
     * @brief 解析查询语句，获取查询类型（无需执行查询）
     * @param query 原始查询语句
     * @return 查询类型枚举（如SELECT/INSERT）
     */
    virtual QueryType parseQueryType(const std::string& query) = 0;

    /**
     * @brief 验证查询语句语法合法性（预处理检查）
     * @param query 原始查询语句
     * @param errorMsg 输出参数：若验证失败，返回错误信息
     * @return 语法合法返回true，否则返回false
     */
    virtual bool validateQuery(const std::string& query, std::string& errorMsg) = 0;

    /**
     * @brief 查询表结构元数据（如列名、类型）
     * @param tableName 目标表名
     * @return 表结构元数据（TableSchema），失败时返回空结构
     */
    virtual TableSchema describeTable(const std::string& tableName) = 0;

    /**
     * @brief 获取当前数据源支持的所有表名
     * @return 表名列表
     */
    virtual std::vector<std::string> listTables() = 0;
};

#endif // IQUERYENGINE_H