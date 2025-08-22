#include <gtest/gtest.h>
#include<input/input_console.h>
#include <sstream>
#include <iostream>
#include <string>

// 辅助类：用于临时重定向std::cin和std::cout
class InputRedirector
{
public:
    InputRedirector(const std::string& input) : inputStream(input)
    {
        // 保存原始缓冲区
        originalCinBuf = std::cin.rdbuf();
        originalCoutBuf = std::cout.rdbuf();
        // 重定向cin到输入流
        std::cin.rdbuf(inputStream.rdbuf());
        // 重定向cout到空流（避免测试时打印提示符）
        std::cout.rdbuf(nullStream.rdbuf());
    }

    ~InputRedirector()
    {
        // 恢复原始缓冲区
        std::cin.rdbuf(originalCinBuf);
        std::cout.rdbuf(originalCoutBuf);
    }

private:
    std::istringstream inputStream;
    std::ostringstream nullStream;
    std::streambuf* originalCinBuf;
    std::streambuf* originalCoutBuf;
};

// 测试1：单句完整输入（带分号和正确括号）
TEST(ConsoleInputSourceTest, SingleCompleteStatement)
{
    std::string testInput = "SELECT * FROM users; \n"; // 输入内容
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "SELECT * FROM users"); // 分号被截断
    EXPECT_EQ(result.getSourceType(), "Console");
}

// 测试2：多行输入（分号在最后一行）
TEST(ConsoleInputSourceTest, MultiLineInput)
{
    std::string testInput = "SELECT id, name\nFROM users\nWHERE age > 18;\n";
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "SELECT id, name FROM users WHERE age > 18");
}

// 测试3：括号不平衡需多行输入
TEST(ConsoleInputSourceTest, UnbalancedBrackets)
{
    std::string testInput = "INSERT INTO logs (message, \nlevel) VALUES ('error');\n";
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "INSERT INTO logs (message, level) VALUES ('error');");
}

// 测试4：引号内的分号（不视为结束符）
TEST(ConsoleInputSourceTest, SemicolonInQuotes)
{
    std::string testInput = "SELECT 'Hello; World' AS msg; \t\n";
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "SELECT 'Hello; World' AS msg");
}

// 测试5：注释中的内容（分号在注释前有效）
TEST(ConsoleInputSourceTest, CommentHandling)
{
    std::string testInput = "UPDATE config SET value=1; -- 这是注释\n";
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "UPDATE config SET value=1");
}

// 测试6：中断处理（模拟Ctrl+C）
TEST(ConsoleInputSourceTest, InterruptHandling)
{
    // 模拟输入到一半时触发中断（直接抛出异常）
    ConsoleInputSource inputSource;
    std::string query = "SELECT * FROM";

EXPECT_NO_THROW({
        try
        {
            throw InterruptException(); // 模拟信号触发的异常
        }
        catch (const InterruptException&)
        {
            ConsoleInputSource::testFriend(query); // 调用友元函数处理中断
        }
        });

    EXPECT_TRUE(query.empty()); // 中断后查询应被清空
}

// 测试7：空白字符处理（修剪首尾空白，合并内部空白）
TEST(ConsoleInputSourceTest, WhitespaceTrimming)
{
    std::string testInput = "   \n  SELECT 1 + 2;   \t\n";
    InputRedirector redirector(testInput);

    ConsoleInputSource inputSource;
    InputData result = inputSource.readInput();

    EXPECT_EQ(result.getRawData(), "SELECT 1 + 2"); // 首尾空白被修剪，内部换行合并为空格
}