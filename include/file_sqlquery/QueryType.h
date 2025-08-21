#pragma once

// 查询类型枚举
enum class QueryType
{
    SELECT,   // 查询操作
    INSERT,   // 插入操作
    UPDATE,   // 更新操作
    DELETE,   // 删除操作
    DDL,      // 数据定义语言（如CREATE TABLE）
    UNKNOWN,   // 未知类型
    DESCRIBE, // 查询表结构（元数据）
};
