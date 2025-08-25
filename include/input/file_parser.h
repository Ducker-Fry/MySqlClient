#pragma once
#include <string>
#include <fstream>
#include <vector>


namespace FileParserUtils
{
    // Utility function to read a file and return its content as a string
    std::string trim(const std::string& str);
    std::fstream openFile(const std::string& filePath);
}

// FileParser Interface
class IFileParser
{
public:
    virtual ~IFileParser() = default;
    // Method to parse a file and return its content as a string
    virtual std::string parseFile(const std::string& filePath) = 0;
    // Method to get the type of the file parser
    virtual std::string getParserType() const = 0;
};

// Concrete Parser class
class CsvFileParser : public IFileParser
{
private:
    std::vector<std::string> parseCsvLine(const std::string& line, char delimiter, char quoteChar);
    std::string convertToJson(const std::vector<std::vector<std::string>>& csvData);
    std::string escapeJson(const std::string& s);
public:
    std::string parseFile(const std::string& filePath) override;
    std::string getParserType() const override { return "csv"; }
};

class SqlFileParser : public IFileParser
{
private:
    // 辅助方法：处理SQL文件中的注释、空白行等
    std::string processSqlContent(const std::string& rawContent);

public:
    // 解析SQL文件，返回处理后的内容（可根据需求格式化为特定结构）
    std::string parseFile(const std::string& filePath) override;

    // 返回解析器类型标识
    std::string getParserType() const override { return "sql"; }
};
