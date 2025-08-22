#include<input/input_file.h>

std::shared_ptr<IFileParser> FileInputSource::getParser() const
{
    std::string tempFilePath = filPath;
    size_t dotPos = tempFilePath.find_last_of('.');

    if (dotPos == std::string::npos || dotPos == 0)
    {
        return nullptr; // No file extension found
    }

    std::string fileType = tempFilePath.substr(dotPos + 1);
    auto parser = parsers.find(fileType);
    if (parser == parsers.end()) return nullptr;
    else return parser->second;
}

void FileInputSource::registerParser(const std::string& type, std::shared_ptr<IFileParser> parser)
{
    parsers[type] = parser;
}

InputData FileInputSource::readInput()
{
    auto parser = getParser();
    std::string parsedData = parser->parseFile(filPath);
    // Create InputData object and set its raw data and source type
    InputData inputData;
    inputData.setRawData(parsedData);
    inputData.setSourceType("FileInputSource");

    return inputData;
}
