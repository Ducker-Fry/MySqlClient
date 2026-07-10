#pragma once

#include <json.hpp>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sql
{
    namespace jsondb
    {
        class Connection;
        class Statement;
        class PreparedStatement;
        class ResultSet;
        class ResultSetMetaData;
        class DatabaseMetaData;

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

        class JsonDbException : public std::runtime_error
        {
        public:
            explicit JsonDbException(const std::string& message) : std::runtime_error(message) {}
        };

        class Driver
        {
        public:
            static Driver& getInstance()
            {
                static Driver instance;
                return instance;
            }

            std::shared_ptr<Connection> connect(
                const std::string& dbPath,
                std::string user = "",
                std::string passwd = "");
        };

        class Connection : public std::enable_shared_from_this<Connection>
        {
        private:
            std::string dbPath;
            bool closed = false;
            bool autoCommit = true;

        public:
            Connection(const std::string& dbPath, std::string user, std::string passwd);
            ~Connection() noexcept;

            void close() noexcept;
            bool isClosed() const { return closed; }
            std::shared_ptr<Statement> createStatement();
            std::shared_ptr<PreparedStatement> prepareStatement(const std::string& sql);
            void setAutoCommit(bool enabled) { autoCommit = enabled; }
            bool getAutoCommit() const { return autoCommit; }
            void commit();
            void rollback();
            bool authenticate(std::string user, std::string passwd);
            bool validateConnection() const;
            bool tableExists(const std::string& tableName) const;
            std::vector<std::string> getColumnNames(const std::string& tableName) const;
            std::string getTableFilePath(const std::string& tableName) const;
            std::string getDbPath() const { return dbPath; }
            std::vector<nlohmann::json> getTableData(const std::string& tableName) const;
        };

        class Statement
        {
        private:
            std::shared_ptr<Connection> connection;

            size_t executeUpdateImpl(const std::string& table, const std::string& setClause, const std::string& whereClause);
            size_t executeDeleteImpl(const std::string& table, const std::string& whereClause);
            size_t executeInsertWithColumns(const std::string& table, const std::string& columns, const std::string& values);
            size_t executeInsertWithoutColumns(const std::string& table, const std::string& values);

            std::map<std::string, std::string> parseSetClause(const std::string& setClause);
            std::vector<std::string> parseList(const std::string& list);
            nlohmann::json createRowFromValues(
                const std::vector<std::string>& colNames,
                const std::vector<std::string>& colValues);
            nlohmann::json readTableData(const std::string& tablePath);
            void writeTableData(const std::string& tablePath, const nlohmann::json& tableData);
            bool evaluateCondition(const nlohmann::json& row, const std::string& condition);
            std::vector<std::string> splitValueGroups(const std::string& valuesStr);
            std::vector<std::string> splitValueGroup(const std::string& valueGroup);
            nlohmann::json parseValue(const std::string& valueStr);
            std::string trim(const std::string& s);
            std::vector<std::string> split(const std::string& s, char delimiter);
            std::string extractTableName(const std::string& tableSpec);

        public:
            explicit Statement(std::shared_ptr<Connection> conn) : connection(std::move(conn)) {}

            std::shared_ptr<ResultSet> executeQuery(const std::string& sql);
            size_t executeUpdate(const std::string& sql);
            bool executeCreate(const std::string& sql);
            bool execute(const std::string& sql);
        };

        class PreparedStatement
        {
        private:
            std::shared_ptr<Connection> connection;
            std::string sql;
            std::vector<std::string> parameters;
            Statement stmt;

            void bindParameter(size_t index, const std::string& value);
            std::string materializeSql() const;

        public:
            PreparedStatement(std::shared_ptr<Connection> conn, const std::string& sql);

            void setInt(size_t index, int value);
            void setFloat(size_t index, float value);
            void setString(size_t index, const std::string& value);
            void setBoolean(size_t index, bool value) { bindParameter(index, value ? "true" : "false"); }
            void setDateTime(size_t index, const std::string& value);

            std::shared_ptr<ResultSet> executeQuery();
            size_t executeUpdate();
            bool execute();
        };

        class ResultSet
        {
        private:
            std::vector<nlohmann::json> rows;
            std::shared_ptr<ResultSetMetaData> metaData;
            size_t currentIndex = 0;
            bool hasCurrentRow = false;

            bool is_date(const std::string& str);
            void ensureCurrentRow() const;

        public:
            explicit ResultSet(
                const std::vector<nlohmann::json>& data,
                const std::vector<std::string>& preferredColumns = {});

            bool next();
            int getInt(const std::string& columnLabel);
            float getFloat(const std::string& columnLabel);
            std::string getString(const std::string& columnLabel);
            bool getBoolean(const std::string& columnLabel);
            std::string getDateTime(const std::string& columnLabel);
            std::shared_ptr<ResultSetMetaData> getMetaData() const { return metaData; }
            void reset()
            {
                currentIndex = 0;
                hasCurrentRow = false;
            }
        };

        class ResultSetMetaData
        {
        private:
            friend class ResultSet;
            std::vector<std::pair<std::string, DataType>> columns;

        public:
            explicit ResultSetMetaData(std::vector<std::pair<std::string, DataType>> cols = {})
                : columns(std::move(cols))
            {
            }

            size_t getColumnCount() const { return columns.size(); }
            std::string getColumnName(size_t index) const;
            DataType getColumnType(size_t index) const;
        };

        class DatabaseMetaData
        {
        private:
            std::shared_ptr<Connection> connection;

        public:
            explicit DatabaseMetaData(std::shared_ptr<Connection> conn) : connection(std::move(conn)) {}
            std::vector<std::string> getTables();
        };
    }
}
