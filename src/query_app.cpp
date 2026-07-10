#include <query_app.h>

#include <core/sql_parser.h>
#include <database/json_driver.h>
#include <database/sqlite_driver.h>
#include <input/input_console.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace
{
struct CliOptions
{
    std::string backend;
    std::string dbPath;
    std::string executeSql;
};

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

void PrintUsage(std::ostream& stream)
{
    stream << "Usage:\n"
           << "  app --backend <json|sqlite> --db <path> --execute \"<sql>\"\n"
           << "  app --backend <json|sqlite> --db <path>\n";
}

bool ParseArgs(const std::vector<std::string>& args, CliOptions& options, std::ostream& err)
{
    for (std::size_t index = 0; index < args.size(); ++index)
    {
        const std::string& arg = args[index];
        if (arg == "--backend" && index + 1 < args.size())
        {
            options.backend = args[++index];
        }
        else if (arg == "--db" && index + 1 < args.size())
        {
            options.dbPath = args[++index];
        }
        else if (arg == "--execute" && index + 1 < args.size())
        {
            options.executeSql = args[++index];
        }
        else
        {
            err << "Unknown or incomplete argument: " << arg << '\n';
            return false;
        }
    }

    if (options.backend.empty() || options.dbPath.empty())
    {
        err << "Both --backend and --db are required.\n";
        return false;
    }

    options.backend = ToLowerCopy(options.backend);
    if (options.backend != "json" && options.backend != "sqlite")
    {
        err << "Unsupported backend: " << options.backend << '\n';
        return false;
    }

    return true;
}

SqlType ParseSqlOperation(const std::string& sql)
{
    InputData input;
    input.setRawData(sql);
    QuerySqlParser parser;
    auto parsed = std::dynamic_pointer_cast<SqlParseResultQuery>(parser.parse(input));
    return parsed == nullptr ? SqlType::UNKNOWN : parsed->getOperationType();
}

template <typename Metadata, typename Getter>
void RenderRows(Metadata metadata, Getter getter, std::ostream& out)
{
    if (metadata->getColumnCount() == 0)
    {
        out << "(0 rows)\n";
        return;
    }

    for (size_t columnIndex = 0; columnIndex < metadata->getColumnCount(); ++columnIndex)
    {
        if (columnIndex > 0)
        {
            out << " | ";
        }
        out << metadata->getColumnName(columnIndex);
    }
    out << '\n';
    for (size_t columnIndex = 0; columnIndex < metadata->getColumnCount(); ++columnIndex)
    {
        if (columnIndex > 0)
        {
            out << "-+-";
        }
        out << "--------";
    }
    out << '\n';

    getter();
}

void PrintValue(std::ostream& out, sql::jsondb::DataType type, sql::jsondb::ResultSet& resultSet, const std::string& column)
{
    switch (type)
    {
    case sql::jsondb::DataType::INT:
        out << resultSet.getInt(column);
        break;
    case sql::jsondb::DataType::FLOAT:
        out << resultSet.getFloat(column);
        break;
    case sql::jsondb::DataType::BOOLEAN:
        out << (resultSet.getBoolean(column) ? "true" : "false");
        break;
    case sql::jsondb::DataType::DATETIME:
        out << resultSet.getDateTime(column);
        break;
    default:
        out << resultSet.getString(column);
        break;
    }
}

void PrintValue(std::ostream& out, sql::sqlite::DataType type, sql::sqlite::ResultSet& resultSet, const std::string& column)
{
    switch (type)
    {
    case sql::sqlite::DataType::INT:
        out << resultSet.getInt(column);
        break;
    case sql::sqlite::DataType::FLOAT:
        out << resultSet.getFloat(column);
        break;
    case sql::sqlite::DataType::BOOLEAN:
        out << (resultSet.getBoolean(column) ? "true" : "false");
        break;
    case sql::sqlite::DataType::DATETIME:
        out << resultSet.getDateTime(column);
        break;
    default:
        out << resultSet.getString(column);
        break;
    }
}

void RenderResultSet(sql::jsondb::ResultSet& resultSet, std::ostream& out)
{
    auto metadata = resultSet.getMetaData();
    RenderRows(
        metadata,
        [&]() {
            while (resultSet.next())
            {
                for (size_t columnIndex = 0; columnIndex < metadata->getColumnCount(); ++columnIndex)
                {
                    if (columnIndex > 0)
                    {
                        out << " | ";
                    }
                    PrintValue(
                        out,
                        metadata->getColumnType(columnIndex),
                        resultSet,
                        metadata->getColumnName(columnIndex));
                }
                out << '\n';
            }
        },
        out);
}

void RenderResultSet(sql::sqlite::ResultSet& resultSet, std::ostream& out)
{
    auto metadata = resultSet.getMetaData();
    RenderRows(
        metadata,
        [&]() {
            while (resultSet.next())
            {
                for (size_t columnIndex = 0; columnIndex < metadata->getColumnCount(); ++columnIndex)
                {
                    if (columnIndex > 0)
                    {
                        out << " | ";
                    }
                    PrintValue(
                        out,
                        metadata->getColumnType(columnIndex),
                        resultSet,
                        metadata->getColumnName(columnIndex));
                }
                out << '\n';
            }
        },
        out);
}

int ExecuteJsonSql(const CliOptions& options, const std::string& sql, std::ostream& out)
{
    auto connection = sql::jsondb::Driver::getInstance().connect(options.dbPath);
    auto statement = connection->createStatement();
    switch (ParseSqlOperation(sql))
    {
    case SqlType::SELECT:
    {
        auto resultSet = statement->executeQuery(sql);
        RenderResultSet(*resultSet, out);
        return 0;
    }
    case SqlType::CREATE:
        if (!statement->executeCreate(sql))
        {
            out << "Table already exists.\n";
            return 0;
        }
        out << "OK\n";
        return 0;
    case SqlType::INSERT:
    case SqlType::UPDATE:
    case SqlType::DELETE:
    {
        const size_t affectedRows = statement->executeUpdate(sql);
        out << "Affected rows: " << affectedRows << '\n';
        return 0;
    }
    default:
        throw sql::jsondb::JsonDbException("Unsupported SQL statement.");
    }
}

int ExecuteSqliteSql(const CliOptions& options, const std::string& sql, std::ostream& out)
{
    auto connection = sql::sqlite::Driver::getInstance().connect(options.dbPath);
    auto statement = connection->createStatement();
    switch (ParseSqlOperation(sql))
    {
    case SqlType::SELECT:
    {
        auto resultSet = statement->executeQuery(sql);
        RenderResultSet(*resultSet, out);
        return 0;
    }
    case SqlType::CREATE:
        statement->execute(sql);
        out << "OK\n";
        return 0;
    case SqlType::INSERT:
    case SqlType::UPDATE:
    case SqlType::DELETE:
    {
        const size_t affectedRows = statement->executeUpdate(sql);
        out << "Affected rows: " << affectedRows << '\n';
        return 0;
    }
    default:
        throw sql::sqlite::SQLiteException("Unsupported SQL statement.");
    }
}

int ExecuteSql(const CliOptions& options, const std::string& sql, std::ostream& out)
{
    if (options.backend == "json")
    {
        return ExecuteJsonSql(options, sql, out);
    }
    return ExecuteSqliteSql(options, sql, out);
}

int RunRepl(const CliOptions& options)
{
    ConsoleInputSource inputSource;

    while (true)
    {
        InputData input = inputSource.readInput();
        const std::string sql = Trim(input.getRawData());
        if (sql.empty())
        {
            if (std::cin.eof())
            {
                std::cout << '\n';
                return 0;
            }
            continue;
        }

        const std::string lower = ToLowerCopy(sql);
        if (lower == "quit" || lower == "exit")
        {
            return 0;
        }

        try
        {
            ExecuteSql(options, sql, std::cout);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "ERROR: " << ex.what() << '\n';
        }
    }
}
}

int RunQueryApp(const std::vector<std::string>& args)
{
    CliOptions options;
    if (!ParseArgs(args, options, std::cerr))
    {
        PrintUsage(std::cerr);
        return 1;
    }

    try
    {
        if (!options.executeSql.empty())
        {
            return ExecuteSql(options, options.executeSql, std::cout);
        }

        return RunRepl(options);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
}
