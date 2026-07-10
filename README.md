# MySqlClient 1.0

MySqlClient is a lightweight local SQL engine and query tool built with C++20. It focuses on three things:

- parsing a practical SQL subset
- executing queries against JSON and SQLite backends
- validating behavior with automated tests

The project is intentionally positioned as a backend/system-design portfolio piece rather than a database tutorial demo.

## What It Supports

The 1.0 SQL subset is:

- `CREATE TABLE`
- `SELECT ... FROM ... [WHERE ...]`
- `INSERT INTO ... VALUES ...`
- `UPDATE ... SET ... [WHERE ...]`
- `DELETE FROM ... [WHERE ...]`

Backend behavior:

- `json`: file-backed tables stored as local JSON arrays with schema sidecars
- `sqlite`: SQLite file backend exposed through the same CLI flow

## Architecture

The project is organized around a simple pipeline:

1. `QuerySqlParser` identifies statement type and extracts high-level structure.
2. Backend-specific `Statement` implementations execute the SQL.
3. `ResultSet` and metadata wrappers normalize how query results are consumed.
4. `RunQueryApp()` powers both the shipped CLI and the CLI test suite.

Key files:

- [src/query_app.cpp](E:/Draft/MySqlClient/src/query_app.cpp:1)
- [src/core/sql_parser.cpp](E:/Draft/MySqlClient/src/core/sql_parser.cpp:1)
- [src/database/json_driver.cpp](E:/Draft/MySqlClient/src/database/json_driver.cpp:1)
- [src/database/sqlite_driver.cpp](E:/Draft/MySqlClient/src/database/sqlite_driver.cpp:1)

## Build And Test

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## CLI Usage

Single statement mode:

```powershell
bin\Debug\app.exe --backend json --db .\examples\jsondb --execute "SELECT id, name FROM users;"
bin\Debug\app.exe --backend sqlite --db .\examples\demo.db --execute "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);"
```

Interactive mode:

```powershell
bin\Debug\app.exe --backend json --db .\examples\jsondb
bin\Debug\app.exe --backend sqlite --db .\examples\demo.db
```

Type SQL ending with `;`. Use `quit;` or `exit;` to leave the REPL.

## Demo Assets

- [examples/demo.sql](E:/Draft/MySqlClient/examples/demo.sql:1): quick JSON/SQLite demo script
- [examples/jsondb/users.json](E:/Draft/MySqlClient/examples/jsondb/users.json:1): sample JSON table data
- [examples/jsondb/users.schema.json](E:/Draft/MySqlClient/examples/jsondb/users.schema.json:1): sample JSON schema file
- [docs/portfolio_showcase.md](E:/Draft/MySqlClient/docs/portfolio_showcase.md:1): portfolio/demo script, screenshot plan, and interview talking points

## Automated Coverage

The test suite now covers:

- console input behavior
- SQL parser behavior
- JSON backend CRUD, projection, metadata, and error cases
- SQLite backend CRUD and prepared statements
- CLI execute mode, REPL mode, backend switching, and error exit paths

## Resume-Friendly Highlights

- Implemented a lightweight SQL query tool in `C++20` with dual `JSON` and `SQLite` storage backends.
- Built parser-driven command dispatch, result-set abstractions, and backend-specific execution layers.
- Added automated tests for parser logic, storage backends, and end-to-end CLI flows with `CTest + GoogleTest`.
- Refactored a learning demo into a portfolio-ready local data system with reproducible demos and documentation.
