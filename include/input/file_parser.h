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
    std::vector<std::string> CsvFileParser::parseCsvLine(const std::string& line, char delimiter, char quoteChar);
    std::string CsvFileParser::convertToJson(const std::vector<std::vector<std::string>>& csvData);
    std::string CsvFileParser::escapeJson(const std::string& s);
public:
    std::string parseFile(const std::string& filePath) override;
    std::string getParserType() const override { return "csv"; }
};