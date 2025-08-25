#pragma once
#include <string>
#include "inputdata.h"
#include "file_parser.h"
#include <unordered_map>

// InputFile Class
class FileInputSource : public IInputSource
{
private:
    std::string filPath;
    std::unordered_map<std::string, std::shared_ptr<IFileParser>> parsers;

    // Method to get the appropriate parser based on the file type
    std::string getFileType() const;
    std::shared_ptr<IFileParser> getParser() const;
public:
    FileInputSource(const std::string& filePath);
    // Method to read data from the file
    void registerParser(const std::string& type, std::shared_ptr<IFileParser> parser);

    InputData readInput() override;
    std::string getSourceType() const override;
};
