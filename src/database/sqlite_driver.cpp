#include <database/sqlite_driver.h>

#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

namespace
{
sql::sqlite::DataType MapSqliteType(sqlite3_stmt* statement, int columnIndex)
{
    using sql::sqlite::DataType;

    const int columnType = sqlite3_column_type(statement, columnIndex);
    switch (columnType)
    {
    case SQLITE_INTEGER:
        return DataType::INT;
    case SQLITE_FLOAT:
        return DataType::FLOAT;
    case SQLITE_TEXT:
    {
        const char* declaredType = sqlite3_column_decltype(statement, columnIndex);
        if (declaredType != nullptr)
        {
            std::string lowerType = declaredType;
            std::transform(
                lowerType.begin(),
                lowerType.end(),
                lowerType.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lowerType.find("date") != std::string::npos || lowerType.find("time") != std::string::npos)
            {
                return DataType::DATETIME;
            }
        }
        return DataType::VARCHAR;
    }
    default:
        return DataType::UNKOWN;
    }
}

nlohmann::json ReadColumnValue(sqlite3_stmt* statement, int columnIndex)
{
    switch (sqlite3_column_type(statement, columnIndex))
    {
    case SQLITE_INTEGER:
        return static_cast<long long>(sqlite3_column_int64(statement, columnIndex));
    case SQLITE_FLOAT:
        return sqlite3_column_double(statement, columnIndex);
    case SQLITE_TEXT:
    {
        const unsigned char* text = sqlite3_column_text(statement, columnIndex);
        return text == nullptr ? "" : reinterpret_cast<const char*>(text);
    }
    case SQLITE_NULL:
        return nullptr;
    default:
        return nullptr;
    }
}

std::unique_ptr<sql::sqlite::ResultSet> BuildResultSet(sqlite3_stmt* statement)
{
    using sql::sqlite::DataType;
    using sql::sqlite::ResultSet;
    using sql::sqlite::ResultSetMetadata;
    using sql::sqlite::SQLiteException;

    const int columnCount = sqlite3_column_count(statement);
    std::vector<std::pair<std::string, DataType>> columnMeta;
    columnMeta.reserve(columnCount);
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        columnMeta.push_back({
            sqlite3_column_name(statement, columnIndex),
            DataType::UNKOWN,
        });
    }

    std::vector<nlohmann::json> rows;
    bool metadataInitialized = false;
    while (true)
    {
        const int stepResult = sqlite3_step(statement);
        if (stepResult == SQLITE_DONE)
        {
            break;
        }
        if (stepResult != SQLITE_ROW)
        {
            throw SQLiteException(sqlite3_db_handle(statement));
        }

        nlohmann::json row = nlohmann::json::object();
        for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
        {
            row[columnMeta[static_cast<std::size_t>(columnIndex)].first] = ReadColumnValue(statement, columnIndex);
            if (!metadataInitialized)
            {
                columnMeta[static_cast<std::size_t>(columnIndex)].second = MapSqliteType(statement, columnIndex);
            }
        }
        metadataInitialized = true;
        rows.push_back(std::move(row));
    }

    auto metaData = std::make_shared<ResultSetMetadata>(std::move(columnMeta));
    return std::make_unique<ResultSet>(std::move(rows), std::move(metaData));
}

void ThrowIfSqliteError(int result, sqlite3* db)
{
    if (result != SQLITE_OK)
    {
        throw sql::sqlite::SQLiteException(db);
    }
}
}

namespace sql
{
    namespace sqlite
    {
        std::unique_ptr<Driver> Driver::instance_ = nullptr;

        bool Driver::authenticate(const std::string& user, const std::string& password)
        {
            (void)user;
            (void)password;
            return true;
        }

        std::unique_ptr<Connection> Driver::connect(
            const std::string& url,
            const std::string& user,
            const std::string& password)
        {
            if (!authenticate(user, password))
            {
                throw SQLiteException("Authentication failed.");
            }

            const fs::path dbPath(url);
            if (!dbPath.parent_path().empty())
            {
                fs::create_directories(dbPath.parent_path());
            }

            sqlite3* db = nullptr;
            const int openResult =
                sqlite3_open_v2(url.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
            if (openResult != SQLITE_OK)
            {
                std::string message = db != nullptr ? sqlite3_errmsg(db) : "Failed to open SQLite database.";
                if (db != nullptr)
                {
                    sqlite3_close(db);
                }
                throw SQLiteException(message, openResult);
            }

            return std::make_unique<Connection>(db, url, dbPath.filename().string());
        }

        Connection::Connection(sqlite3* db, std::string url, const std::string& dbName)
            : db_(db),
              dbName_(dbName),
              isValid_(db != nullptr),
              url_(std::move(url))
        {
        }

        Connection::Connection(Connection&& other) noexcept
            : db_(other.db_),
              dbName_(std::move(other.dbName_)),
              autoCommit_(other.autoCommit_),
              connectTimeout_(other.connectTimeout_),
              encoding_(std::move(other.encoding_)),
              isValid_(other.isValid_),
              url_(std::move(other.url_))
        {
            other.db_ = nullptr;
            other.isValid_ = false;
        }

        Connection& Connection::operator=(Connection&& other) noexcept
        {
            if (this != &other)
            {
                close();
                db_ = other.db_;
                dbName_ = std::move(other.dbName_);
                autoCommit_ = other.autoCommit_;
                connectTimeout_ = other.connectTimeout_;
                encoding_ = std::move(other.encoding_);
                isValid_ = other.isValid_;
                url_ = std::move(other.url_);
                other.db_ = nullptr;
                other.isValid_ = false;
            }
            return *this;
        }

        Connection::~Connection()
        {
            close();
        }

        std::unique_ptr<Statement> Connection::createStatement()
        {
            return std::make_unique<Statement>(this);
        }

        std::unique_ptr<PreparedStatement> Connection::prepareStatement(const std::string& sql)
        {
            return std::make_unique<PreparedStatement>(this, sql);
        }

        void Connection::commit()
        {
            ThrowIfSqliteError(sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr), db_);
        }

        void Connection::rollback()
        {
            ThrowIfSqliteError(sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr), db_);
        }

        void Connection::close()
        {
            if (db_ != nullptr)
            {
                sqlite3_close(db_);
                db_ = nullptr;
            }
            isValid_ = false;
        }

        void Connection::setConnectTimeout(int seconds)
        {
            connectTimeout_ = seconds;
            if (db_ != nullptr)
            {
                sqlite3_busy_timeout(db_, seconds * 1000);
            }
        }

        std::string ResultSetMetadata::getColumnName(size_t index) const
        {
            return columns_.at(index).first;
        }

        DataType ResultSetMetadata::getColumnType(size_t index) const
        {
            return columns_.at(index).second;
        }

        ResultSet::ResultSet(
            std::vector<nlohmann::json> rows,
            std::shared_ptr<ResultSetMetadata> metaData)
            : rows_(std::move(rows)),
              metaData_(std::move(metaData))
        {
        }

        void ResultSet::ensureCurrentRow() const
        {
            if (!hasCurrentRow_ || currentIndex_ >= rows_.size())
            {
                throw SQLiteException("ResultSet cursor is not positioned on a valid row.");
            }
        }

        bool ResultSet::next()
        {
            if (!hasCurrentRow_)
            {
                if (rows_.empty())
                {
                    return false;
                }
                currentIndex_ = 0;
                hasCurrentRow_ = true;
                return true;
            }

            if (currentIndex_ + 1 >= rows_.size())
            {
                return false;
            }

            ++currentIndex_;
            return true;
        }

        int ResultSet::getInt(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows_.at(currentIndex_).at(columnLabel).get<int>();
        }

        float ResultSet::getFloat(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows_.at(currentIndex_).at(columnLabel).get<float>();
        }

        std::string ResultSet::getString(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows_.at(currentIndex_).at(columnLabel).get<std::string>();
        }

        bool ResultSet::getBoolean(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows_.at(currentIndex_).at(columnLabel).get<bool>();
        }

        std::string ResultSet::getDateTime(const std::string& columnLabel)
        {
            ensureCurrentRow();
            return rows_.at(currentIndex_).at(columnLabel).get<std::string>();
        }

        void ResultSet::reset()
        {
            currentIndex_ = 0;
            hasCurrentRow_ = false;
        }

        std::unique_ptr<ResultSet> Statement::executeQuery(const std::string& sql)
        {
            sqlite3_stmt* statement = nullptr;
            const int prepareResult = sqlite3_prepare_v2(connection_->rawHandle(), sql.c_str(), -1, &statement, nullptr);
            if (prepareResult != SQLITE_OK)
            {
                throw SQLiteException(connection_->rawHandle());
            }

            try
            {
                std::unique_ptr<ResultSet> result = BuildResultSet(statement);
                sqlite3_finalize(statement);
                return result;
            }
            catch (...)
            {
                sqlite3_finalize(statement);
                throw;
            }
        }

        size_t Statement::executeUpdate(const std::string& sql)
        {
            sqlite3_stmt* statement = nullptr;
            const int prepareResult = sqlite3_prepare_v2(connection_->rawHandle(), sql.c_str(), -1, &statement, nullptr);
            if (prepareResult != SQLITE_OK)
            {
                throw SQLiteException(connection_->rawHandle());
            }

            try
            {
                const int stepResult = sqlite3_step(statement);
                if (stepResult != SQLITE_DONE && stepResult != SQLITE_ROW)
                {
                    throw SQLiteException(connection_->rawHandle());
                }

                const int affectedRows = sqlite3_changes(connection_->rawHandle());
                sqlite3_finalize(statement);
                return static_cast<size_t>(affectedRows);
            }
            catch (...)
            {
                sqlite3_finalize(statement);
                throw;
            }
        }

        bool Statement::execute(const std::string& sql)
        {
            sqlite3_stmt* statement = nullptr;
            const int prepareResult = sqlite3_prepare_v2(connection_->rawHandle(), sql.c_str(), -1, &statement, nullptr);
            if (prepareResult != SQLITE_OK)
            {
                throw SQLiteException(connection_->rawHandle());
            }

            const bool isReadonly = sqlite3_stmt_readonly(statement) != 0;
            sqlite3_finalize(statement);

            if (isReadonly)
            {
                return executeQuery(sql) != nullptr;
            }

            executeUpdate(sql);
            return true;
        }

        PreparedStatement::PreparedStatement(Connection* connection, const std::string& sql)
            : connection_(connection)
        {
            const int prepareResult = sqlite3_prepare_v2(connection_->rawHandle(), sql.c_str(), -1, &statement_, nullptr);
            if (prepareResult != SQLITE_OK)
            {
                throw SQLiteException(connection_->rawHandle());
            }
        }

        PreparedStatement::~PreparedStatement()
        {
            if (statement_ != nullptr)
            {
                sqlite3_finalize(statement_);
                statement_ = nullptr;
            }
        }

        void PreparedStatement::setInt(size_t index, int value)
        {
            ThrowIfSqliteError(
                sqlite3_bind_int(statement_, static_cast<int>(index), value),
                connection_->rawHandle());
        }

        void PreparedStatement::setFloat(size_t index, float value)
        {
            ThrowIfSqliteError(
                sqlite3_bind_double(statement_, static_cast<int>(index), value),
                connection_->rawHandle());
        }

        void PreparedStatement::setString(size_t index, const std::string& value)
        {
            ThrowIfSqliteError(
                sqlite3_bind_text(statement_, static_cast<int>(index), value.c_str(), -1, SQLITE_TRANSIENT),
                connection_->rawHandle());
        }

        void PreparedStatement::setBoolean(size_t index, bool value)
        {
            setInt(index, value ? 1 : 0);
        }

        void PreparedStatement::setDateTime(size_t index, const std::string& value)
        {
            setString(index, value);
        }

        std::unique_ptr<ResultSet> PreparedStatement::executeQuery()
        {
            try
            {
                std::unique_ptr<ResultSet> result = BuildResultSet(statement_);
                sqlite3_reset(statement_);
                return result;
            }
            catch (...)
            {
                sqlite3_reset(statement_);
                throw;
            }
        }

        size_t PreparedStatement::executeUpdate()
        {
            const int stepResult = sqlite3_step(statement_);
            if (stepResult != SQLITE_DONE && stepResult != SQLITE_ROW)
            {
                sqlite3_reset(statement_);
                throw SQLiteException(connection_->rawHandle());
            }

            const size_t affectedRows = static_cast<size_t>(sqlite3_changes(connection_->rawHandle()));
            sqlite3_reset(statement_);
            return affectedRows;
        }

        bool PreparedStatement::execute()
        {
            if (sqlite3_stmt_readonly(statement_) != 0)
            {
                return executeQuery() != nullptr;
            }

            executeUpdate();
            return true;
        }
    }
}
