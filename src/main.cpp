#include<iostream>
#include<input/inputdata.h>
#include<core/sql_parser.h>
#include<json.hpp>


int main()
{
    std::string sql = "SELECT name, age FROM system.users WHERE age > 30 ORDER BY name LIMIT 10";
    InputData input;
    QuerySqlParser parser;
    input.setRawData(sql);
    auto result1 = parser.parse(input);
    auto result = std::dynamic_pointer_cast<SqlParseResultQuery>(result1);

    std::cout << "Database: " << result->getDatabase() << std::endl;
    std::cout << "Table: " << result->getTable() << std::endl;
    std::cout << "Columns: ";
    for (const auto& col : result->getColumns())
        std::cout << col << " ";
    std::cout << std::endl;
    std::cout << "where: " << result->getWhereClause() << std::endl;
    return 0;
}