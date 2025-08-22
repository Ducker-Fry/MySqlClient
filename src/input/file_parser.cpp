#include<input/file_parser.h>

std::string FileParserUtils::trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    do { end--; } while (distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

std::fstream FileParserUtils::openFile(const std::string& filePath)
{
    std::fstream fileStream(filePath, std::ios::in);
    if(fileStream.is_open())
    {
        return fileStream;
    }
    else
    {
        throw std::runtime_error("Could not open file: " + filePath);
    }
}

std::vector<std::string> CsvFileParser::parseCsvLine(const std::string& line, char delimiter, char quoteChar)
{
    std::vector<std::string> fields;
    std::string currentField;
    bool inQuotes = false;  // 是否在引号内
    size_t i = 0;
    size_t n = line.size();

    while (i < n)
    {
        if (line[i] == quoteChar)
        {
            // 处理引号：支持引号内包含分隔符和换行
            if (i + 1 < n && line[i + 1] == quoteChar)
            {
                // 连续两个引号视为转义（如""表示"）
                currentField += quoteChar;
                i += 2;  // 跳过两个引号
            }
            else
            {
                inQuotes = !inQuotes;  // 引号开关
                i++;  // 跳过当前引号
            }
        }
        else if (line[i] == delimiter && !inQuotes)
        {
            // 分隔符且不在引号内，结束当前字段
            fields.push_back(trim(currentField));
            currentField.clear();
            i++;
        }
        else
        {
            // 普通字符直接添加
            currentField += line[i];
            i++;
        }
    }

    // 添加最后一个字段
    fields.push_back(trim(currentField));

    return fields;
}

std::string CsvFileParser::convertToJson(const std::vector<std::vector<std::string>>& csvData)
{

    if (csvData.empty()) return "[]";

    std::stringstream json;
    json << "[\n";

    // 表头行
    const auto& headers = csvData[0];

    // 遍历数据行
    for (size_t i = 1; i < csvData.size(); ++i)
    {  // i=0是表头，从i=1开始是数据
        const auto& row = csvData[i];
        json << "  {\n";

        // 拼接键值对（表头为键，数据为值）
        for (size_t j = 0; j < headers.size(); ++j)
        {
            json << "    \"" << headers[j] << "\": \"" << escapeJson(row[j]) << "\"";
            if (j != headers.size() - 1) json << ",\n";
        }

        json << "\n  }";
        if (i != csvData.size() - 1) json << ",";
        json << "\n";
    }

    json << "]";
    return json.str();
}

std::string CsvFileParser::escapeJson(const std::string& s)
{
    std::string escaped;
    for (char c : s)
    {
        switch (c)
        {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += c; break;
        }
    }
    return escaped;
}


std::string CsvFileParser::parseFile(const std::string & filePath)
{
    // 1. 打开文件
    auto file = FileParserUtils::openFile(filePath);

    // 2. 读取并解析所有行
    std::vector<std::vector<std::string>> csvData;  // 存储结构化CSV数据
    std::string line;
    char delimiter = ',';  // 默认分隔符，可扩展为配置参数
    char quoteChar = '"';  // 默认引号字符

    // 读取表头行（第一行）
    if (std::getline(file, line))
    {
        csvData.push_back(parseCsvLine(line, delimiter, quoteChar));
    }

    // 读取数据行
    while (std::getline(file, line))
    {
        if (line.empty()) continue;  // 跳过空行
        std::vector<std::string> row = parseCsvLine(line, delimiter, quoteChar);

        // 校验列数与表头一致（可选，根据需求开启）
        if (!csvData.empty() && row.size() != csvData[0].size())
        {
            throw std::runtime_error("CSV行列数不一致，行内容: " + line);
        }
        csvData.push_back(row);
    }

    // 3. 将结构化数据转换为标准化字符串（示例：JSON格式，便于上层解析）
    return convertToJson(csvData);
}