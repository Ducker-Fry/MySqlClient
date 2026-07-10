#include <gtest/gtest.h>

#include <query_app.h>

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

class StreamRedirector
{
public:
    explicit StreamRedirector(const std::string& input = "")
        : inputStream_(input)
    {
        originalCin_ = std::cin.rdbuf(inputStream_.rdbuf());
        originalCout_ = std::cout.rdbuf(outputStream_.rdbuf());
        originalCerr_ = std::cerr.rdbuf(errorStream_.rdbuf());
    }

    ~StreamRedirector()
    {
        std::cin.rdbuf(originalCin_);
        std::cout.rdbuf(originalCout_);
        std::cerr.rdbuf(originalCerr_);
    }

    std::string stdoutText() const { return outputStream_.str(); }
    std::string stderrText() const { return errorStream_.str(); }

private:
    std::istringstream inputStream_;
    std::ostringstream outputStream_;
    std::ostringstream errorStream_;
    std::streambuf* originalCin_ = nullptr;
    std::streambuf* originalCout_ = nullptr;
    std::streambuf* originalCerr_ = nullptr;
};

class QueryAppTest : public testing::Test
{
protected:
    fs::path tempDir = "test_temp_app";

    void SetUp() override
    {
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }
        fs::create_directories(tempDir);
    }

    void TearDown() override
    {
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }
    }
};

TEST_F(QueryAppTest, ExecuteModeWorksForJsonBackend)
{
    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp({"--backend", "json", "--db", tempDir.string(), "--execute", "CREATE TABLE users (id INT, name TEXT);"}),
            0);
    }

    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp({"--backend", "json", "--db", tempDir.string(), "--execute", "INSERT INTO users (id, name) VALUES (1, 'Alice');"}),
            0);
        EXPECT_NE(redirect.stdoutText().find("Affected rows: 1"), std::string::npos);
    }

    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp({"--backend", "json", "--db", tempDir.string(), "--execute", "SELECT id, name FROM users;"}),
            0);
        EXPECT_NE(redirect.stdoutText().find("Alice"), std::string::npos);
    }
}

TEST_F(QueryAppTest, ReplModeWorksForJsonBackend)
{
    const std::string input =
        "CREATE TABLE users (id INT, name TEXT);\n"
        "INSERT INTO users (id, name) VALUES (1, 'Alice');\n"
        "SELECT id, name FROM users;\n"
        "quit;\n";

    StreamRedirector redirect(input);
    EXPECT_EQ(RunQueryApp({"--backend", "json", "--db", tempDir.string()}), 0);
    EXPECT_NE(redirect.stdoutText().find("Alice"), std::string::npos);
}

TEST_F(QueryAppTest, ExecuteModeWorksForSqliteBackend)
{
    const fs::path dbPath = tempDir / "app.db";

    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp(
                {"--backend", "sqlite", "--db", dbPath.string(), "--execute", "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);"}),
            0);
    }

    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp(
                {"--backend", "sqlite", "--db", dbPath.string(), "--execute", "INSERT INTO users (name) VALUES ('Bob');"}),
            0);
        EXPECT_NE(redirect.stdoutText().find("Affected rows: 1"), std::string::npos);
    }

    {
        StreamRedirector redirect;
        EXPECT_EQ(
            RunQueryApp(
                {"--backend", "sqlite", "--db", dbPath.string(), "--execute", "SELECT id, name FROM users;"}),
            0);
        EXPECT_NE(redirect.stdoutText().find("Bob"), std::string::npos);
    }
}

TEST_F(QueryAppTest, InvalidSqlReturnsNonZeroAndPrintsError)
{
    StreamRedirector redirect;
    EXPECT_EQ(
        RunQueryApp({"--backend", "json", "--db", tempDir.string(), "--execute", "UPSERT INTO users VALUES (1);"}),
        1);
    EXPECT_NE(redirect.stderrText().find("ERROR"), std::string::npos);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
