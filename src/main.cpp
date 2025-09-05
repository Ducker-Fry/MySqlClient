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
		auto conn = driver.connect("testdb", "admin", "password");
		std::cout<<"Connection Success !"<<std::endl;

	}
	catch (const std::exception&)
	{
		std::cout << "Connection Fail !";
	}
}