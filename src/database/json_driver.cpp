#include <database/json_driver.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
std::string Trim(const std::string& value)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos)
    {
        return "";
    }

    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::string ToLowerCopy(const std::string& value)
{
    std::string lower = value;
    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lower;
}

std::string EscapeSqlString(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value)
    {
        escaped += ch;
        if (ch == '\'')
        {
            escaped += '\'';
        }
    }
    return escaped;
}

std::string RemoveTrailingSemicolon(const std::string& sql)
{
    std::string trimmed = Trim(sql);
    if (!trimmed.empty() && trimmed.back() == ';')
    {
        trimmed.pop_back();
        trimmed = Trim(trimmed);
    }
    return trimmed;
}

std::string JsonSchemaFilePath(const std::string& dbPath, const std::string& tableName)
{
    fs::path path(dbPath);
    path /= tableName + ".schema.json";
    return path.string();
}

std::vector<std::string> SplitCommaAware(const std::string& input)
{
    std::vector<std::string> items;
    std::string current;
    char quoteChar = 0;
    int parenDepth = 0;

    for (const char ch : input)
    {
        if (quoteChar != 0)
        {
            current += ch;
            if (ch == quoteChar)
            {
                quoteChar = 0;
            }
            continue;
        }

        if (ch == '\'' || ch == '"')
        {
            quoteChar = ch;
            current += ch;
            continue;
        }

        if (ch == '(')
        {
            ++parenDepth;
        }
        else if (ch == ')' && parenDepth > 0)
        {
            --parenDepth;
        }

        if (ch == ',' && parenDepth == 0)
        {
            items.push_back(Trim(current));
            current.clear();
            continue;
        }

        current += ch;
    }

    if (!Trim(current).empty())
    {
        items.push_back(Trim(current));
    }

    return items;
}

std::vector<std::string> SplitConditions(const std::string& input)
{
    std::vector<std::string> parts;
    std::string current;
    char quoteChar = 0;

    for (std::size_t i = 0; i < input.size(); ++i)
    {
        const char ch = input[i];
        if (quoteChar != 0)
        {
            current += ch;
            if (ch == quoteChar)
            {
                quoteChar = 0;
            }
            continue;
        }

        if (ch == '\'' || ch == '"')
        {
            quoteChar = ch;
            current += ch;
            continue;
        }

        if (i + 4 <= input.size())
        {
            const std::string token = ToLowerCopy(input.substr(i, 4));
            const bool hasWordBoundaryBefore = (i == 0) || std::isspace(static_cast<unsigned char>(input[i - 1]));
            const bool hasWordBoundaryAfter =
                (i + 4 == input.size()) || std::isspace(static_cast<unsigned char>(input[i + 3]));
            if (token == "and " && hasWordBoundaryBefore && hasWordBoundaryAfter)
            {
                parts.push_back(Trim(current));
                current.clear();
                i += 3;
                continue;
            }
        }

        current += ch;
    }

    if (!Trim(current).empty())
    {
        parts.push_back(Trim(current));
    }

    return parts;
}

nlohmann::json ParseScalarValue(const std::string& token)
{
    const std::string value = Trim(token);
    if (value.empty())
    {
        return "";
    }

    if ((value.front() == '\'' && value.back() == '\'') || (value.front() == '"' && value.back() == '"'))
    {
        return value.substr(1, value.size() - 2);
    }

    const std::string lower = ToLowerCopy(value);
    if (lower == "true")
    {
        return true;
    }
    if (lower == "false")
    {
        return false;
    }
    if (lower == "null")
    {
        return nullptr;
    }

    try
    {
        std::size_t parsed = 0;
        const long long integerValue = std::stoll(value, &parsed);
        if (parsed == value.size())
        {
            return integerValue;
        }
    }
    catch (const std::exception&)
    {
    }

    try
    {
        std::size_t parsed = 0;
        const double floatingValue = std::stod(value, &parsed);
        if (parsed == value.size())
        {
            return floatingValue;
        }
    }
    catch (const std::exception&)
    {
    }

    return value;
}

sql::jsondb::DataType DetectJsonType(const nlohmann::json& value)
{
    using sql::jsondb::DataType;

    if (value.is_number_integer())
    {
        return DataType::INT;
    }
    if (value.is_number_float())
    {
        return DataType::FLOAT;
    }
    if (value.is_boolean())
    {
        return DataType::BOOLEAN;
    }
    if (value.is_string())
    {
        const std::string text = value.get<std::string>();
        if (std::regex_match(text, std::regex(R"(\d{4}-\d{2}-\d{2}(?:[T ]\d{2}:\d{2}:\d{2})?)")))
        {
            return DataType::DATETIME;
        }
        return DataType::VARCHAR;
    }
    return DataType::UNKOWN;
}

bool CompareValues(const nlohmann::json& left, const std::string& op, const nlohmann::json& right)
{
    if (left.is_number() && right.is_number())
    {
        const double lhs = left.get<double>();
        const double rhs = right.get<double>();
        if (op == "=")
        {
            return lhs == rhs;
        }
        if (op == "!=")
        {
            return lhs != rhs;
        }
        if (op == ">")
        {
            return lhs > rhs;
        }
        if (op == "<")
        {
            return lhs < rhs;
        }
        if (op == ">=")
        {
            return lhs >= rhs;
        }
        if (op == "<=")
        {
            return lhs <= rhs;
        }
    }

    if ((left.is_boolean() && right.is_boolean()) || (left.is_string() && right.is_string()) || (left.is_null() && right.is_null()))
    {
        if (op == "=")
        {
            return left == right;
        }
        if (op == "!=")
        {
            return left != right;
        }
    }

    return false;
}
}

namespace sql
{
    namespace jsondb
    {
        std::shared_ptr<Connection> Driver::connect(const std::string& dbPath, std::string user, std::string passwd)
        {
            return std::make_shared<Connection>(dbPath, std::move(user), std::move(passwd));
        }

        Connection::Connection(const std::string& dbPath, std::string user, std::string passwd)
            : dbPath(dbPath)
        {
            if (!authenticate(std::move(user), std::move(passwd)))
            {
                throw JsonDbException("Authentication failed.");
            }

            const fs::path path(dbPath);
            if (!fs::exists(path))
            {
                fs::create_directories(path);
            }
            else if (!fs::is_directory(path))
            {
                throw JsonDbException("Database path is not a directory: " + dbPath);
            }
        }

        Connection::~Connection() noexcept
        {
            close();
        }

        void Connection::close() noexcept
        {
            closed = true;
        }

        std::shared_ptr<Statement> Connection::createStatement()
        {
            return std::make_shared<Statement>(shared_from_this());
        }

        std::shared_ptr<PreparedStatement> Connection::prepareStatement(const std::string& sql)
        {
            return std::make_shared<PreparedStatement>(shared_from_this(), sql);
        }

        void Connection::commit()
        {
        }

        void Connection::rollback()
        {
        }

        bool Connection::authenticate(std::string user, std::string passwd)
        {
            (void)user;
            (void)passwd;
            return true;
        }

        bool Connection::validateConnection() const
        {
            return !closed && fs::exists(dbPath);
        }

        bool Connection::tableExists(const std::string& tableName) const
        {
            return fs::exists(getTableFilePath(tableName));
        }

        std::vector<std::string> Connection::getColumnNames(const std::string& tableName) const
        {
            if (!tableExists(tableName))
            {
                throw JsonDbException("Table does not exist: " + tableName);
            }

            const std::string schemaPath = JsonSchemaFilePath(dbPath, tableName);
            if (fs::exists(schemaPath))
            {
                std::ifstream schemaFile(schemaPath);
                if (!schemaFile.is_open())
                {
                    throw JsonDbException("Failed to open schema file: " + schemaPath);
                }

                nlohmann::json schemaJson;
                schemaFile >> schemaJson;
                return schemaJson.get<std::vector<std::string>>();
            }

            const std::vector<nlohmann::json> rows = getTableData(tableName);
            if (rows.empty())
            {
                return {};
            }

            std::vector<std::string> columns;
            for (auto it = rows.front().begin(); it != rows.front().end(); ++it)
            {
                columns.push_back(it.key());
            }
            return columns;
        }

        std::string Connection::getTableFilePath(const std::string& tableName) const
        {
            fs::path path(dbPath);
            path /= tableName + ".json";
            return path.string();
        }

        std::vector<nlohmann::json> Connection::getTableData(const std::string& tableName) const
        {
            const std::string tablePath = getTableFilePath(tableName);
            if (!fs::exists(tablePath))
            {
                throw JsonDbException("Table does not exist: " + tableName);
            }

            std::ifstream file(tablePath);
            if (!file.is_open())
            {
                throw JsonDbException("Failed to open table file: " + tablePath);
            }

            nlohmann::json tableData;
            file >> tableData;
            if (!tableData.is_array())
            {
                throw JsonDbException("Invalid table format for table: " + tableName);
            }

            return tableData.get<std::vector<nlohmann::json>>();
        }

        std::shared_ptr<ResultSet> Statement::executeQuery(const std::string& sql)
        {
            const std::string normalized = RemoveTrailingSemicolon(sql);
            std::smatch match;
            const std::regex selectPattern(
                R"(^SELECT\s+(.*?)\s+FROM\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)(?:\s+WHERE\s+(.+))?$)",
                std::regex::icase);
            if (!std::regex_match(normalized, match, selectPattern))
            {
                throw JsonDbException("Invalid SELECT statement: " + sql);
            }

            const std::string tableName = extractTableName(match[2].str());
            const std::string whereClause = match[3].matched ? Trim(match[3].str()) : "";
            std::vector<std::string> selectedColumns = SplitCommaAware(match[1].str());
            if (selectedColumns.empty())
            {
                throw JsonDbException("SELECT statement must include at least one column.");
            }

            const std::vector<nlohmann::json> tableRows = connection->getTableData(tableName);
            std::vector<nlohmann::json> filteredRows;
            for (const auto& row : tableRows)
            {
                if (whereClause.empty() || evaluateCondition(row, whereClause))
                {
                    filteredRows.push_back(row);
                }
            }

            if (selectedColumns.size() == 1 && selectedColumns.front() == "*")
            {
                return std::make_shared<ResultSet>(filteredRows);
            }

            std::vector<nlohmann::json> projectedRows;
            projectedRows.reserve(filteredRows.size());
            for (const auto& row : filteredRows)
            {
                nlohmann::json projected = nlohmann::json::object();
                for (const auto& column : selectedColumns)
                {
                    if (!row.contains(column))
                    {
                        throw JsonDbException("Column does not exist: " + column);
                    }
                    projected[column] = row.at(column);
                }
                projectedRows.push_back(std::move(projected));
            }

            return std::make_shared<ResultSet>(projectedRows, selectedColumns);
        }

        size_t Statement::executeUpdate(const std::string& sql)
        {
            const std::string normalized = RemoveTrailingSemicolon(sql);
            std::smatch match;

            const std::regex insertPattern(
                R"(^INSERT\s+INTO\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)\s*\((.*?)\)\s+VALUES\s+(.+)$)",
                std::regex::icase);
            if (std::regex_match(normalized, match, insertPattern))
            {
                return executeInsertWithColumns(extractTableName(match[1].str()), match[2].str(), match[3].str());
            }

            const std::regex insertWithoutColumnsPattern(
                R"(^INSERT\s+INTO\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)\s+VALUES\s+(.+)$)",
                std::regex::icase);
            if (std::regex_match(normalized, match, insertWithoutColumnsPattern))
            {
                return executeInsertWithoutColumns(extractTableName(match[1].str()), match[2].str());
            }

            const std::regex updatePattern(
                R"(^UPDATE\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+))?$)",
                std::regex::icase);
            if (std::regex_match(normalized, match, updatePattern))
            {
                return executeUpdateImpl(
                    extractTableName(match[1].str()),
                    match[2].str(),
                    match[3].matched ? Trim(match[3].str()) : "");
            }

            const std::regex deletePattern(
                R"(^DELETE\s+FROM\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)(?:\s+WHERE\s+(.+))?$)",
                std::regex::icase);
            if (std::regex_match(normalized, match, deletePattern))
            {
                return executeDeleteImpl(
                    extractTableName(match[1].str()),
                    match[2].matched ? Trim(match[2].str()) : "");
            }

            throw JsonDbException("Invalid mutation statement: " + sql);
        }

        bool Statement::executeCreate(const std::string& sql)
        {
            const std::string normalized = RemoveTrailingSemicolon(sql);
            std::smatch match;
            const std::regex createTablePattern(
                R"(^CREATE\s+TABLE\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?)\s*\((.*)\)$)",
                std::regex::icase);
            if (!std::regex_match(normalized, match, createTablePattern))
            {
                throw JsonDbException("Invalid CREATE TABLE statement: " + sql);
            }

            const std::string tableName = extractTableName(match[1].str());
            const std::string tablePath = connection->getTableFilePath(tableName);
            if (fs::exists(tablePath))
            {
                return false;
            }

            std::vector<std::string> columns;
            for (const auto& definition : SplitCommaAware(match[2].str()))
            {
                std::stringstream line(definition);
                std::string columnName;
                line >> columnName;
                if (!columnName.empty())
                {
                    columns.push_back(columnName);
                }
            }

            std::ofstream tableFile(tablePath);
            tableFile << "[]";

            std::ofstream schemaFile(JsonSchemaFilePath(connection->getDbPath(), tableName));
            schemaFile << std::setw(2) << nlohmann::json(columns);

            return true;
        }

        bool Statement::execute(const std::string& sql)
        {
            const std::string trimmedSql = Trim(sql);
            if (trimmedSql.empty())
            {
                throw JsonDbException("SQL statement is empty.");
            }

            std::string operation = ToLowerCopy(trimmedSql.substr(0, std::min<std::size_t>(trimmedSql.size(), 6)));
            if (operation.rfind("select", 0) == 0)
            {
                return executeQuery(sql) != nullptr;
            }
            if (operation.rfind("create", 0) == 0)
            {
                return executeCreate(sql);
            }
            return executeUpdate(sql) > 0;
        }

        std::map<std::string, std::string> Statement::parseSetClause(const std::string& setClause)
        {
            std::map<std::string, std::string> updates;
            for (const auto& assignment : SplitCommaAware(setClause))
            {
                const std::size_t equalsPos = assignment.find('=');
                if (equalsPos == std::string::npos)
                {
                    throw JsonDbException("Invalid SET assignment: " + assignment);
                }

                const std::string column = trim(assignment.substr(0, equalsPos));
                const std::string value = trim(assignment.substr(equalsPos + 1));
                updates[column] = value;
            }
            return updates;
        }

        std::vector<std::string> Statement::parseList(const std::string& list)
        {
            std::string normalized = trim(list);
            if (!normalized.empty() && normalized.front() == '(' && normalized.back() == ')')
            {
                normalized = normalized.substr(1, normalized.size() - 2);
            }
            return SplitCommaAware(normalized);
        }

        nlohmann::json Statement::createRowFromValues(
            const std::vector<std::string>& colNames,
            const std::vector<std::string>& colValues)
        {
            if (colNames.size() != colValues.size())
            {
                throw JsonDbException(
                    "Column-value count mismatch. columns=" + std::to_string(colNames.size()) +
                    ", values=" + std::to_string(colValues.size()));
            }

            nlohmann::json row = nlohmann::json::object();
            for (std::size_t index = 0; index < colNames.size(); ++index)
            {
                row[colNames[index]] = parseValue(colValues[index]);
            }
            return row;
        }

        nlohmann::json Statement::readTableData(const std::string& tablePath)
        {
            std::ifstream file(tablePath);
            if (!file.is_open())
            {
                throw JsonDbException("Failed to open table file: " + tablePath);
            }

            nlohmann::json tableData;
            file >> tableData;
            if (!tableData.is_array())
            {
                throw JsonDbException("Table data must be a JSON array: " + tablePath);
            }
            return tableData;
        }

        void Statement::writeTableData(const std::string& tablePath, const nlohmann::json& tableData)
        {
            std::ofstream outFile(tablePath, std::ios::trunc);
            if (!outFile.is_open())
            {
                throw JsonDbException("Failed to write table file: " + tablePath);
            }
            outFile << std::setw(2) << tableData;
        }

        bool Statement::evaluateCondition(const nlohmann::json& row, const std::string& condition)
        {
            for (const auto& clause : SplitConditions(condition))
            {
                std::smatch match;
                const std::regex conditionPattern(R"(^([A-Za-z0-9_]+)\s*(=|!=|>=|<=|>|<)\s*(.+)$)");
                if (!std::regex_match(clause, match, conditionPattern))
                {
                    throw JsonDbException("Unsupported WHERE clause: " + clause);
                }

                const std::string column = match[1].str();
                const std::string op = match[2].str();
                const nlohmann::json expectedValue = parseValue(match[3].str());

                if (!row.contains(column))
                {
                    throw JsonDbException("Column does not exist in WHERE clause: " + column);
                }

                if (!CompareValues(row.at(column), op, expectedValue))
                {
                    return false;
                }
            }

            return true;
        }

        std::vector<std::string> Statement::splitValueGroups(const std::string& valuesStr)
        {
            std::vector<std::string> groups;
            std::string current;
            char quoteChar = 0;
            int parenDepth = 0;

            for (const char ch : valuesStr)
            {
                if (quoteChar == 0 && parenDepth == 0 && (std::isspace(static_cast<unsigned char>(ch)) || ch == ','))
                {
                    continue;
                }

                if (quoteChar != 0)
                {
                    current += ch;
                    if (ch == quoteChar)
                    {
                        quoteChar = 0;
                    }
                    continue;
                }

                if (ch == '\'' || ch == '"')
                {
                    quoteChar = ch;
                    current += ch;
                    continue;
                }

                if (ch == '(')
                {
                    ++parenDepth;
                }
                else if (ch == ')' && parenDepth > 0)
                {
                    --parenDepth;
                }

                current += ch;
                if (parenDepth == 0 && ch == ')')
                {
                    groups.push_back(trim(current));
                    current.clear();
                }
            }

            return groups;
        }

        std::vector<std::string> Statement::splitValueGroup(const std::string& valueGroup)
        {
            std::string trimmedGroup = trim(valueGroup);
            if (!trimmedGroup.empty() && trimmedGroup.front() == '(' && trimmedGroup.back() == ')')
            {
                trimmedGroup = trimmedGroup.substr(1, trimmedGroup.size() - 2);
            }
            return SplitCommaAware(trimmedGroup);
        }

        nlohmann::json Statement::parseValue(const std::string& valueStr)
        {
            return ParseScalarValue(valueStr);
        }

        std::string Statement::trim(const std::string& s)
        {
            return Trim(s);
        }

        std::vector<std::string> Statement::split(const std::string& s, char delimiter)
        {
            std::vector<std::string> tokens;
            std::stringstream stream(s);
            std::string item;
            while (std::getline(stream, item, delimiter))
            {
                tokens.push_back(trim(item));
            }
            return tokens;
        }

        std::string Statement::extractTableName(const std::string& tableSpec)
        {
            const std::size_t dotPos = tableSpec.find('.');
            if (dotPos == std::string::npos)
            {
                return trim(tableSpec);
            }
            return trim(tableSpec.substr(dotPos + 1));
        }

        size_t Statement::executeUpdateImpl(
            const std::string& table,
            const std::string& setClause,
            const std::string& whereClause)
        {
            if (!connection->tableExists(table))
            {
                throw JsonDbException("Table does not exist: " + table);
            }

            const std::string tablePath = connection->getTableFilePath(table);
            nlohmann::json tableData = readTableData(tablePath);
            const std::map<std::string, std::string> updates = parseSetClause(setClause);

            size_t affectedRows = 0;
            for (auto& row : tableData)
            {
                if (whereClause.empty() || evaluateCondition(row, whereClause))
                {
                    for (const auto& [column, value] : updates)
                    {
                        row[column] = parseValue(value);
                    }
                    ++affectedRows;
                }
            }

            writeTableData(tablePath, tableData);
            return affectedRows;
        }

        size_t Statement::executeDeleteImpl(const std::string& table, const std::string& whereClause)
        {
            if (!connection->tableExists(table))
            {
                throw JsonDbException("Table does not exist: " + table);
            }

            const std::string tablePath = connection->getTableFilePath(table);
            nlohmann::json tableData = readTableData(tablePath);
            nlohmann::json keptRows = nlohmann::json::array();
            size_t affectedRows = 0;

            for (const auto& row : tableData)
            {
                if (whereClause.empty() || evaluateCondition(row, whereClause))
                {
                    ++affectedRows;
                }
                else
                {
                    keptRows.push_back(row);
                }
            }

            writeTableData(tablePath, keptRows);
            return affectedRows;
        }

        size_t Statement::executeInsertWithColumns(
            const std::string& table,
            const std::string& columns,
            const std::string& valuesStr)
        {
            if (!connection->tableExists(table))
            {
                throw JsonDbException("Table does not exist: " + table);
            }

            const std::vector<std::string> columnNames = SplitCommaAware(columns);
            const std::vector<std::string> valueGroups = splitValueGroups(valuesStr);
            if (valueGroups.empty())
            {
                throw JsonDbException("INSERT statement does not include any values.");
            }

            const std::vector<std::string> schemaColumns = connection->getColumnNames(table);
            if (!schemaColumns.empty())
            {
                for (const auto& column : columnNames)
                {
                    if (std::find(schemaColumns.begin(), schemaColumns.end(), column) == schemaColumns.end())
                    {
                        throw JsonDbException("Column does not exist in schema: " + column);
                    }
                }
            }

            const std::string tablePath = connection->getTableFilePath(table);
            nlohmann::json tableData = readTableData(tablePath);
            size_t insertedRows = 0;

            for (const auto& valueGroup : valueGroups)
            {
                const std::vector<std::string> values = splitValueGroup(valueGroup);
                tableData.push_back(createRowFromValues(columnNames, values));
                ++insertedRows;
            }

            writeTableData(tablePath, tableData);
            return insertedRows;
        }

        size_t Statement::executeInsertWithoutColumns(const std::string& table, const std::string& values)
        {
            const std::vector<std::string> columnNames = connection->getColumnNames(table);
            if (columnNames.empty())
            {
                throw JsonDbException("Table schema is empty. INSERT without columns is not supported.");
            }

            const std::vector<std::string> valueGroups = splitValueGroups(values);
            if (valueGroups.empty())
            {
                throw JsonDbException("INSERT statement does not include any values.");
            }

            const std::string tablePath = connection->getTableFilePath(table);
            nlohmann::json tableData = readTableData(tablePath);
            size_t insertedRows = 0;
            for (const auto& valueGroup : valueGroups)
            {
                const std::vector<std::string> rowValues = splitValueGroup(valueGroup);
                tableData.push_back(createRowFromValues(columnNames, rowValues));
                ++insertedRows;
            }

            writeTableData(tablePath, tableData);
            return insertedRows;
        }

        PreparedStatement::PreparedStatement(std::shared_ptr<Connection> conn, const std::string& sql)
            : connection(std::move(conn)),
              sql(sql),
              parameters(std::count(sql.begin(), sql.end(), '?')),
              stmt(connection)
        {
        }

        void PreparedStatement::bindParameter(size_t index, const std::string& value)
        {
            if (index == 0 || index > parameters.size())
            {
                throw JsonDbException("PreparedStatement parameter index out of range.");
            }
            parameters[index - 1] = value;
        }

        std::string PreparedStatement::materializeSql() const
        {
            std::string query = sql;
            std::size_t searchPos = 0;
            for (const auto& parameter : parameters)
            {
                const std::size_t placeholder = query.find('?', searchPos);
                if (placeholder == std::string::npos)
                {
                    throw JsonDbException("PreparedStatement placeholder count mismatch.");
                }
                query.replace(placeholder, 1, parameter);
                searchPos = placeholder + parameter.size();
            }
            return query;
        }

        void PreparedStatement::setInt(size_t index, int value)
        {
            bindParameter(index, std::to_string(value));
        }

        void PreparedStatement::setFloat(size_t index, float value)
        {
            bindParameter(index, std::to_string(value));
        }

        void PreparedStatement::setString(size_t index, const std::string& value)
        {
            bindParameter(index, "'" + EscapeSqlString(value) + "'");
        }

        void PreparedStatement::setDateTime(size_t index, const std::string& value)
        {
            bindParameter(index, "'" + EscapeSqlString(value) + "'");
        }

        std::shared_ptr<ResultSet> PreparedStatement::executeQuery()
        {
            return stmt.executeQuery(materializeSql());
        }

        size_t PreparedStatement::executeUpdate()
        {
            return stmt.executeUpdate(materializeSql());
        }

        bool PreparedStatement::execute()
        {
            return stmt.execute(materializeSql());
        }

        ResultSet::ResultSet(const std::vector<nlohmann::json>& data, const std::vector<std::string>& preferredColumns)
            : rows(data),
              metaData(std::make_shared<ResultSetMetaData>())
        {
            if (!rows.empty())
            {
                const nlohmann::json& firstRow = rows.front();
                if (!preferredColumns.empty())
                {
                    for (const auto& column : preferredColumns)
                    {
                        if (firstRow.contains(column))
                        {
                            metaData->columns.push_back({column, DetectJsonType(firstRow.at(column))});
                        }
                        else
                        {
                            metaData->columns.push_back({column, DataType::UNKOWN});
                        }
                    }
                }
                else
                {
                    for (auto it = firstRow.begin(); it != firstRow.end(); ++it)
                    {
                        metaData->columns.push_back({it.key(), DetectJsonType(it.value())});
                    }
                }
            }
            else
            {
                for (const auto& column : preferredColumns)
                {
                    metaData->columns.push_back({column, DataType::UNKOWN});
                }
            }
        }

        bool ResultSet::is_date(const std::string& str)
        {
            return std::regex_match(str, std::regex(R"(\d{4}-\d{2}-\d{2}(?:[T ]\d{2}:\d{2}:\d{2})?)"));
        }

        void ResultSet::ensureCurrentRow() const
        {
            if (!hasCurrentRow || currentIndex >= rows.size())
            {
                throw JsonDbException("ResultSet cursor is not positioned on a valid row.");
            }
        }

        bool ResultSet::next()
        {
            if (!hasCurrentRow)
            {
                if (rows.empty())
                {
                    return false;
                }
                currentIndex = 0;
                hasCurrentRow = true;
                return true;
            }

            if (currentIndex + 1 >= rows.size())
            {
                return false;
            }

            ++currentIndex;
            return true;
        }

        int ResultSet::getInt(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows.at(currentIndex).at(columnLabel).get<int>();
        }

        float ResultSet::getFloat(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows.at(currentIndex).at(columnLabel).get<float>();
        }

        std::string ResultSet::getString(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows.at(currentIndex).at(columnLabel).get<std::string>();
        }

        bool ResultSet::getBoolean(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows.at(currentIndex).at(columnLabel).get<bool>();
        }

        std::string ResultSet::getDateTime(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows.at(currentIndex).at(columnLabel).get<std::string>();
        }

        std::string ResultSetMetaData::getColumnName(size_t index) const
        {
            return columns.at(index).first;
        }

        DataType ResultSetMetaData::getColumnType(size_t index) const
        {
            return columns.at(index).second;
        }

        std::vector<std::string> DatabaseMetaData::getTables()
        {
            std::vector<std::string> tables;
            for (const auto& entry : fs::directory_iterator(connection->getDbPath()))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".json")
                {
                    continue;
                }

                const std::string stem = entry.path().stem().string();
                if (stem.size() >= 7 && stem.substr(stem.size() - 7) == ".schema")
                {
                    continue;
                }
                tables.push_back(stem);
            }
            return tables;
        }
    }
}
