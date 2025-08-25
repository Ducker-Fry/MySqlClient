#include<iostream>
#include<input/input_file.h>
#include<filesystem>

int main()
{
    try
    {
        // 解析文件
        std::string filePath = std::filesystem::absolute("E:\\Draft\\MySqlClient\\tests\\test_sql.sql").string();
        FileInputSource fileInputSource(filePath);
        auto Data = fileInputSource.readInput();
        std::cout << "Data :\n" << Data.getRawData() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;

}