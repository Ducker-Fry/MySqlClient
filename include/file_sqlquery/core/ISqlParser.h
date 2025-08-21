#ifndef ISQLPARSER_H
#define ISQLPARSER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>

// SQL语句类型枚举
enum class SqlStatementType
{
    UNKNOWN,
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE_TABLE,
    DROP_TABLE,
    DESCRIBE
};

// 表达式类型（用于WHERE条件等）
enum class ExpressionType
{
    COMPARISON,   // 比较表达式（如a = b）
    LOGICAL,      // 逻辑表达式（如a AND b）
    FUNCTION,     // 函数表达式（如COUNT(*)）
    LITERAL,      // 常量值（如123, 'abc'）
    COLUMN_REF    // 列引用（如user.name）
};

// 比较运算符
enum class ComparisonOp
{
    EQ,  // =
    NE,  // !=
    LT,  // <
    GT,  // >
    LE,  // <=
    GE,  // >=
    LIKE // LIKE
};

// 逻辑运算符
enum class LogicalOp
{
    AND,
    OR,
    NOT
};

// 表达式节点（抽象基类）
struct ExpressionNode
{
    ExpressionType type;
    virtual ~ExpressionNode() = default;
protected:
    explicit ExpressionNode(ExpressionType t) : type(t) {}
};

// 比较表达式（如column = value）
struct ComparisonExpr : ExpressionNode
{
    std::string column;          // 列名（如"age"）
    ComparisonOp op;             // 比较运算符
    std::variant<int, double, std::string, bool> value;  // 比较值

    ComparisonExpr() : ExpressionNode(ExpressionType::COMPARISON) {}
};

// 逻辑表达式（如a AND b）
struct LogicalExpr : ExpressionNode
{
    LogicalOp op;                        // 逻辑运算符
    std::shared_ptr<ExpressionNode> left;  // 左表达式
    std::shared_ptr<ExpressionNode> right; // 右表达式（NOT运算时可为空）

    LogicalExpr() : ExpressionNode(ExpressionType::LOGICAL) {}
};

// 列元数据（用于解析结果）
struct ColumnMetadata
{
    std::string name;         // 列名（如"id"）
    std::string tableAlias;   // 表别名（如"u" in "u.id"）
    std::string alias;        // 列别名（如"user_id" in "id AS user_id"）
};

// SELECT语句解析结果
struct SelectParseResult
{
    std::vector<ColumnMetadata> columns;  // 选中的列
    std::vector<std::string> tables;      // 涉及的表
    std::shared_ptr<ExpressionNode> whereClause;  // WHERE条件
    std::vector<std::string> groupBy;     // GROUP BY列
    std::shared_ptr<ExpressionNode> havingClause; // HAVING条件
    std::vector<std::pair<std::string, bool>> orderBy; // 排序（列名+是否升序）
    int limit = -1;                       // LIMIT值（-1表示无限制）
    int offset = 0;                       // OFFSET值
};

// INSERT语句解析结果
struct InsertParseResult
{
    std::string table;                   // 目标表名
    std::vector<std::string> columns;    // 插入的列
    std::vector<std::vector<std::variant<int, double, std::string, bool>>> values; // 插入值
};

// UPDATE语句解析结果
struct UpdateParseResult
{
    std::string table;                   // 目标表名
    std::map<std::string, std::variant<int, double, std::string, bool>> setValues; // 更新的列和值
    std::shared_ptr<ExpressionNode> whereClause;  // WHERE条件
};

// DELETE语句解析结果
struct DeleteParseResult
{
    std::string table;                   // 目标表名
    std::shared_ptr<ExpressionNode> whereClause;  // WHERE条件
};

// SQL解析结果（变体类型，根据语句类型存储不同结果）
struct SqlParseResult
{
    SqlStatementType type = SqlStatementType::UNKNOWN;
    std::string originalSql;             // 原始SQL语句

    // 存储具体语句的解析结果
    std::variant<
        SelectParseResult,
        InsertParseResult,
        UpdateParseResult,
        DeleteParseResult,
        std::string  // 用于CREATE/DROP/DESCRIBE等简单语句的表名
    > data;
};

// SQL解析错误信息
struct SqlParseError
{
    std::string message;  // 错误描述
    int line = -1;        // 错误所在行
    int column = -1;      // 错误所在列
};

/**
 * @brief SQL解析器抽象接口
 */
class ISqlParser
{
public:
    using Ptr = std::shared_ptr<ISqlParser>;

    virtual ~ISqlParser() = default;

    /**
     * @brief 解析SQL语句
     * @param sql 待解析的SQL字符串
     * @return 解析成功返回true，失败返回false
     */
    virtual bool parse(const std::string& sql) = 0;

    /**
     * @brief 获取解析结果
     * @return 解析结果结构体（需先调用parse且返回true）
     */
    virtual const SqlParseResult& getParseResult() const = 0;

    /**
     * @brief 获取解析错误信息
     * @return 错误信息结构体（parse返回false时有效）
     */
    virtual const SqlParseError& getError() const = 0;

    /**
     * @brief 检查SQL语句语法是否合法（不执行完整解析）
     * @param sql 待检查的SQL字符串
     * @return 语法合法返回true，否则返回false
     */
    virtual bool validateSyntax(const std::string& sql) = 0;

    /**
     * @brief 获取SQL语句类型（无需完整解析）
     * @param sql 待识别的SQL字符串
     * @return SQL语句类型枚举
     */
    virtual SqlStatementType getStatementType(const std::string& sql) = 0;
};

#endif // ISQLPARSER_H
