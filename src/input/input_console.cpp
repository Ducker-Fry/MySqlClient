#include "input_console.h"
#include<csignal>

InputData ConsoleInputSource::readInput()
{
    InputData inputData;
    std::string query;
    std::string prompt = "mysql> ";
    bool isComplete = false;

    initializeSignalHandler();

    try
    {
        while (!isComplete)
        {
            displayPrompt(prompt);
            if (!readLine(query, prompt, isComplete))
            {
                // 处理输入流结束（如Ctrl+D）
                std::cout << std::endl;
                isComplete = true;
            }
        }
    }
    catch (const InterruptException&)
    {
        handleInterrupt(query);
    }

    query = trimQuery(query);
    inputData.setRawData(query);
    inputData.setSourceType("Console");
    return inputData;
}

// 初始化中断信号处理器
void ConsoleInputSource::initializeSignalHandler()
{
    static bool initialized = false;
    if (!initialized)
    {
        signal(SIGINT, [](int) {
            throw InterruptException();
            });
        initialized = true;
    }
}

// 显示输入提示符
void ConsoleInputSource::displayPrompt(const std::string& prompt)
{
    std::cout << prompt;
    std::cout.flush();
}

// 读取一行输入并更新状态
bool ConsoleInputSource::readLine(std::string& query, std::string& prompt, bool& isComplete)
{
    InputState state;
    char ch;

    while (std::cin.get(ch))
    {
        processCharacter(ch, state, query);

        if (ch == '\n' && !isComplete)
        {
            prompt = "    -> ";
            return true; // 继续多行输入
        }

        if (isStatementComplete(state, query))
        {
            truncateToCompleteStatement(query);
            isComplete = true;
            return true;
        }
    }

    return false; // 输入流结束
}

// 处理单个字符并更新状态
void ConsoleInputSource::processCharacter(char ch, InputState& state, std::string& query)
{
    if (state.inComment)
    {
        processCommentCharacter(ch, state, query);
    }
    else if (state.quoteChar != 0)
    {
        processQuotedCharacter(ch, state, query);
    }
    else
    {
        processNormalCharacter(ch, state, query);
    }
}

// 处理注释中的字符
void ConsoleInputSource::processCommentCharacter(char ch, InputState& state, std::string& query)
{
    query += ch;
    if (ch == '\n')
    {
        state.inComment = false; // 单行注释结束
    }
}

// 处理引号内的字符
void ConsoleInputSource::processQuotedCharacter(char ch, InputState& state, std::string& query)
{
    query += ch;
    if (ch == state.quoteChar)
    {
        state.quoteChar = 0; // 引号结束
    }
}

// 处理正常字符（非注释、非引号内）
void ConsoleInputSource::processNormalCharacter(char ch, InputState& state, std::string& query)
{
    // 检查是否进入注释
    if (ch == '-' && std::cin.peek() == '-')
    {
        state.inComment = true;
        query += ch;
        return;
    }

    // 检查引号开始
    if (ch == '\'' || ch == '"')
    {
        state.quoteChar = ch;
        query += ch;
        return;
    }

    // 更新括号平衡
    updateBracketBalance(ch, state);

    query += ch;
}

// 更新括号平衡状态
void ConsoleInputSource::updateBracketBalance(char ch, InputState& state)
{
    if (ch == '(' || ch == '[' || ch == '{')
    {
        state.bracketBalance++;
    }
    else if (ch == ')' || ch == ']' || ch == '}')
    {
        state.bracketBalance--;
    }
}

// 判断语句是否完整
bool ConsoleInputSource::isStatementComplete(const InputState& state, const std::string& query)
{
    // 语句完整条件：不在引号中、括号平衡、且包含分号
    if (state.quoteChar == 0 && state.bracketBalance == 0)
    {
        size_t lastSemicolon = query.find_last_of(';');
        if (lastSemicolon != std::string::npos)
        {
            // 分号后只能有空白字符
            std::string tail = query.substr(lastSemicolon + 1);
            return tail.find_first_not_of(" \t\n\r") == std::string::npos;
        }
    }
    return false;
}

// 将查询截断到完整语句（去除分号及后续内容）
void ConsoleInputSource::truncateToCompleteStatement(std::string& query)
{
    size_t lastSemicolon = query.find_last_of(';');
    if (lastSemicolon != std::string::npos)
    {
        query = query.substr(0, lastSemicolon);
    }
}

// 处理中断（如Ctrl+C）
void ConsoleInputSource::handleInterrupt(std::string& query)
{
    query.clear();
    std::cout << "\nAborted" << std::endl;
}

// 修剪查询前后的空白字符，并合并内部多余换行符
std::string ConsoleInputSource::trimQuery(const std::string& query)
{
    // 1. 先修剪首尾空白
    size_t start = query.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";

    size_t end = query.find_last_not_of(" \t\n\r");
    std::string trimmed = query.substr(start, end - start + 1);

    // 2. 处理内部多余换行符：将连续换行/空白合并为单个空格
    std::string result;
    result.reserve(trimmed.size()); // 预分配空间提升效率

    bool isPreviousWhitespace = false;
    for (char ch : trimmed)
    {
        // 判断当前字符是否为空白（空格/制表符/换行/回车）
        bool isWhitespace = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

        if (isWhitespace)
        {
            // 连续空白只保留一个空格
            if (!isPreviousWhitespace)
            {
                result += ' ';
                isPreviousWhitespace = true;
            }
        }
        else
        {
            // 非空白字符直接添加
            result += ch;
            isPreviousWhitespace = false;
        }
    }

    return result;
}

void ConsoleInputSource::testFriend(std::string& query)
{
    ConsoleInputSource inputSource;
    inputSource.handleInterrupt(query); // 调用处理函数
}

