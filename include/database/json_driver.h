// 模仿mysql_connection.h的核心类结构
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <json.hpp>
#include <stdexcept>

namespace sql
{
    namespace jsondb
    {
        // Forward declaration of JsonDriverImpl
        class Connection;
        class Statement;
        class PreparedStatement;
        class ResultSet;
        class ResultSetMetaData;
        class DatabaseMetaData;

        //data type
        enum class  DataType
        {
            INT,
            FLOAT,
            VARCHAR,
            BOOLEAN,
            TEXT,
            DATETIME
        };

        // basic exception class for JSON database errors
        class JsonDbException : public std::runtime_error
        {
        public:
            explicit JsonDbException(const std::string& message) : std::runtime_error(message) {}
        };

        // JsonDriver class (singleton pattern)
        class Driver
        {
            public:
            static Driver& getInstance()
            {
                static Driver instance;
                return instance;
            }
            std::shared_ptr<Connection> connect(const std::string& dbPath,std::string user,std::string passwd);
        };

        //Connection class for managing database connections
        class Connection
        {
        private:
            std::string dbPath;
            bool closed;
            bool autoCommit;
        public:
            Connection(const std::string& dbPath,std::string user,std::string passwd);
            ~Connection();
            void close();
            bool isClosed() const { return closed; }
            std::shared_ptr<Statement> createStatement();
            std::shared_ptr<PreparedStatement> prepareStatement(const std::string& sql);
            void setAutoCommit(bool autoCommit) { this->autoCommit = autoCommit; }
            bool getAutoCommit() const { return autoCommit; }
            void commit();
            void rollback();
            // check user authentication
            bool authenticate(std::string user, std::string passwd);
            // check if connection is valid , partly check if file exists
            bool validateConnection() const;
            // check if the table exists
            bool tableExists(const std::string& tableName) const;   
            // get all column names of the specified table
            std::vector<std::string> getColumnNames(const std::string& tableName) const;
            //get table json file path
            std::string getTableFilePath(const std::string& tableName) const;
        };

        //Statement class for executing SQL statements
        class Statement
        {
        private:
            std::shared_ptr<Connection> connection;
            
            // Helper methods for executeUpdate
            size_t executeUpdateImpl(const std::string& table, const std::string& setClause, const std::string& whereClause);
            size_t executeDeleteImpl(const std::string& table, const std::string& whereClause);
            size_t executeInsertWithColumns(const std::string& table, const std::string& columns, const std::string& values);
            size_t executeInsertWithoutColumns(const std::string& table, const std::string& values);
            
            // Utility methods
            std::map<std::string, std::string> parseSetClause(const std::string& setClause);
            std::vector<std::string> parseList(const std::string& list);
            nlohmann::json createRowFromValues(const std::vector<std::string>& colNames, const std::vector<std::string>& colValues);
            nlohmann::json readTableData(const std::string& tablePath);
            void writeTableData(const std::string& tablePath, const nlohmann::json& tableData);
            bool evaluateCondition(const nlohmann::json& row, const std::string& condition);
            
            // Get table name from full table specification
            std::string extractTableName(const std::string& tableSpec);

        public:
            Statement(Connection* conn) : connection(conn) {}

            // Execute a SQL query and return a ResultSet
            std::shared_ptr<ResultSet> executeQuery(const std::string& sql);
            // Execute a SQL update and return the number of affected rows , for INSERT, UPDATE, DELETE, etc.
            size_t executeUpdate(const std::string& sql);
            // Generic execute method for any SQL statement
            bool execute(const std::string& sql);
        };

        //PreparedStatement class for executing parameterized SQL statements, combined with Statement not inheritance
        class PreparedStatement
        {
        private:
            std::shared_ptr<Connection> connection;
            std::string sql;
            std::vector<std::string> parameters; // Store parameters as strings for simplicity
            void bindParameter(size_t index, const std::string& value);
            Statement stmt; // composition with Statement
        public:
            PreparedStatement(Connection* conn, const std::string& sql) : connection(conn), sql(sql), stmt(conn) {}
            void setInt(size_t index, int value);
            void setFloat(size_t index, float value);
            void setString(size_t index, const std::string& value);
            void setBoolean(size_t index, bool value);
            void setDateTime(size_t index, const std::string& value); // Expecting ISO 8601 format
            std::shared_ptr<ResultSet> executeQuery();
            size_t executeUpdate();
            bool execute();
        };

        //ResultSet class for handling query results
        class ResultSet
        {
        private:
            std::vector<nlohmann::json> rows; // Each row is a JSON object
            std::shared_ptr<ResultSetMetaData> metaData;
            size_t currentIndex;
        public:
            ResultSet(const std::vector<nlohmann::json>& data);
            bool next();
            int getInt(const std::string& columnLabel);
            float getFloat(const std::string& columnLabel);
            std::string getString(const std::string& columnLabel);
            bool getBoolean(const std::string& columnLabel);
            std::string getDateTime(const std::string& columnLabel); // Return as ISO 8601 string
            std::shared_ptr<ResultSetMetaData> getMetaData() const;
            void close();
            bool isClosed() const;
            std::shared_ptr<DatabaseMetaData> getMetaData();
        };

        //ResultSetMetaData class for metadata about ResultSet
        class ResultSetMetaData
        {
        private:
            std::vector<std::pair<std::string, DataType>> columns; // column name and data type
        public:
            ResultSetMetaData(const std::vector<std::pair<std::string, DataType>> cols) : columns(std::move(cols)) {}
            size_t getColumnCount() const { return columns.size(); }
            std::string getColumnName(size_t index) const;
            DataType getColumnType(size_t index) const;
        };

        //DatabaseMetaData class for metadata about the database
        class DatabaseMetaData
        {
        private:
            std::shared_ptr<Connection> connection;
        public:
            DatabaseMetaData(Connection* conn) : connection(conn) {}
            std::vector<std::string> getTables();
            std::vector<std::pair<std::string, DataType>> getColumns(const std::string& tableName);
        };
    }
}
