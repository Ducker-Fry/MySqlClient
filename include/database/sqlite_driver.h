#pragma once

#include <json.hpp>
#include <sqlite/sqlite3.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sql
{
    namespace sqlite
    {
        class Driver;
        class Connection;
        class Statement;
        class PreparedStatement;
        class ResultSet;
        class ResultSetMetadata;

        enum class DataType
        {
            INT,
            FLOAT,
            VARCHAR,
            BOOLEAN,
            TEXT,
            DATETIME,
            UNKOWN
        };

        class SQLiteException : public std::runtime_error
        {
        private:
            int errorCode_;

        public:
            explicit SQLiteException(const std::string& errMsg, int errorCode = -1)
                : std::runtime_error(errMsg),
                  errorCode_(errorCode)
            {
            }

            explicit SQLiteException(sqlite3* db)
                : std::runtime_error(sqlite3_errmsg(db)),
                  errorCode_(sqlite3_errcode(db))
            {
            }

            int getErrorCode() const noexcept { return errorCode_; }
        };

        class Driver
        {
        private:
            Driver() = default;
            Driver(const Driver&) = delete;
            Driver& operator=(const Driver&) = delete;

            static std::unique_ptr<Driver> instance_;
            bool authenticate(const std::string& user, const std::string& password);

        public:
            static Driver& getInstance()
            {
                if (!instance_)
                {
                    instance_ = std::unique_ptr<Driver>(new Driver());
                }
                return *instance_;
            }

            std::unique_ptr<Connection> connect(
                const std::string& url,
                const std::string& user = "",
                const std::string& password = "");
        };

        class Connection
        {
        private:
            sqlite3* db_ = nullptr;
            std::string dbName_;
            bool autoCommit_ = true;
            int connectTimeout_ = 0;
            std::string encoding_ = "UTF-8";
            bool isValid_ = false;
            std::string url_;

        public:
            Connection() = default;
            Connection(sqlite3* db, std::string url, const std::string& dbName);
            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&& other) noexcept;
            Connection& operator=(Connection&& other) noexcept;
            ~Connection();

            std::unique_ptr<Statement> createStatement();
            std::unique_ptr<PreparedStatement> prepareStatement(const std::string& sql);

            void setAutoCommit(bool autoCommit) { autoCommit_ = autoCommit; }
            bool getAutoCommit() const { return autoCommit_; }
            void commit();
            void rollback();

            bool isValid() const { return isValid_; }
            void close();
            void* getNativeHandle() const { return db_; }
            sqlite3* rawHandle() const { return db_; }
            std::string getDatabaseName() const { return dbName_; }
            void setConnectTimeout(int seconds);
            std::string getEncoding() const { return encoding_; }
        };

        class ResultSetMetadata
        {
        private:
            std::vector<std::pair<std::string, DataType>> columns_;

        public:
            explicit ResultSetMetadata(std::vector<std::pair<std::string, DataType>> columns = {})
                : columns_(std::move(columns))
            {
            }

            size_t getColumnCount() const { return columns_.size(); }
            std::string getColumnName(size_t index) const;
            DataType getColumnType(size_t index) const;
        };

        class ResultSet
        {
        private:
            std::vector<nlohmann::json> rows_;
            std::shared_ptr<ResultSetMetadata> metaData_;
            size_t currentIndex_ = 0;
            bool hasCurrentRow_ = false;

            void ensureCurrentRow() const;

        public:
            ResultSet(
                std::vector<nlohmann::json> rows,
                std::shared_ptr<ResultSetMetadata> metaData);

            bool next();
            int getInt(const std::string& columnLabel);
            float getFloat(const std::string& columnLabel);
            std::string getString(const std::string& columnLabel);
            bool getBoolean(const std::string& columnLabel);
            std::string getDateTime(const std::string& columnLabel);
            std::shared_ptr<ResultSetMetadata> getMetaData() const { return metaData_; }
            void reset();
        };

        class Statement
        {
        private:
            Connection* connection_ = nullptr;

        public:
            explicit Statement(Connection* connection) : connection_(connection) {}

            std::unique_ptr<ResultSet> executeQuery(const std::string& sql);
            size_t executeUpdate(const std::string& sql);
            bool execute(const std::string& sql);
        };

        class PreparedStatement
        {
        private:
            Connection* connection_ = nullptr;
            sqlite3_stmt* statement_ = nullptr;

        public:
            PreparedStatement(Connection* connection, const std::string& sql);
            PreparedStatement(const PreparedStatement&) = delete;
            PreparedStatement& operator=(const PreparedStatement&) = delete;
            ~PreparedStatement();

            void setInt(size_t index, int value);
            void setFloat(size_t index, float value);
            void setString(size_t index, const std::string& value);
            void setBoolean(size_t index, bool value);
            void setDateTime(size_t index, const std::string& value);

            std::unique_ptr<ResultSet> executeQuery();
            size_t executeUpdate();
            bool execute();
        };
    }
}
