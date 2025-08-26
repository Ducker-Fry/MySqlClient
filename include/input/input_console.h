#pragma once
#include"inputdata.h"
#include<iostream>

// 辅助类：存储输入状态
struct InputState
{
    int bracketBalance = 0;    // 括号平衡计数器
    char quoteChar = 0;        // 当前引号类型（'/"，0表示无）
    bool inComment = false;    // 是否在注释中
};

// 自定义异常：处理中断
class InterruptException : public std::exception
{
    const char* what() const noexcept override
    {
        return "Input interrupted";
    }
};

class ConsoleInputSource : public IInputSource
{
public:
    InputData readInput() override;
    std::string getSourceType() const override { return "sql"; }
    //测试友元
    static void testFriend(std::string& query);
private:
    // 信号处理
    void initializeSignalHandler();

    // 输入交互
    void displayPrompt(const std::string& prompt);
    bool readLine(std::string& query, std::string& prompt, bool& isComplete);

    // 字符处理
    void processCharacter(char ch, InputState& state, std::string& query);
    void processCommentCharacter(char ch, InputState& state, std::string& query);
    void processQuotedCharacter(char ch, InputState& state, std::string& query);
    void processNormalCharacter(char ch, InputState& state, std::string& query);
    void updateBracketBalance(char ch, InputState& state);

    // 语句判断与处理
    bool isStatementComplete(const InputState& state, const std::string& query);
    void truncateToCompleteStatement(std::string& query);
    void handleInterrupt(std::string& query);
    std::string trimQuery(const std::string& query);

};

