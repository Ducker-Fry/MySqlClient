#include <iostream>
#include<input/inputdata.h>
#include<core/sql_parser.h>
#include<json.hpp>
#include<database/json_driver.h>


int main()
{
    // 测试Connection
    auto driver = sql::jsondb::Driver::getInstance();
	try
	{
		auto conn = driver.connect("E:\\Draft\\MySqlClient\\testdb", "admin", "password");
		auto stmt = conn->createStatement();
		auto res = stmt->executeUpdate("INSERT INTO test (id, name) VALUES (1, 'test')");
		std::cout << "Insert " << res << " rows." << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "Connection Fail !";
	}
}