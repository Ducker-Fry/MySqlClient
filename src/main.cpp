#include <iostream>
#include <sqlite/sqlite3.h>


int main()
{
    sqlite3* db; //create a pointer to a database
    char* errMsg = 0; //create a pointer to an error message
    int exit; //create a variable to store the return value of the sqlite3_open() function
    exit = sqlite3_open("testdb\\test.db", &db); //open the database
    if (exit != SQLITE_OK) //check if the database was opened successfully
    {
        std::cout << "Error opening database: " << sqlite3_errmsg(db) << std::endl; //print an error message
        return 1; //exit the program
    }

    //create a table, execute the SQL statement
    const char* sql = "create table if not exists students (id integer primary key, name text, age integer);";
    exit = sqlite3_exec(db, sql, 0, 0, &errMsg); //execute the SQL statement
    if (exit != SQLITE_OK) //check if the SQL statement was executed successfully
    {
        std::cout << "Error creating table: " << errMsg << std::endl; //print an error message
        sqlite3_free(errMsg); //free the error message
        sqlite3_close(db); //close the database
        return 1; //exit the program
    }

    //insert a several rows into the table, execute the SQL statement
        const char* sql2 = "insert into students (name, age) values ('John', 20), ('Jane', 21), ('Bob', 22) ,('Alice',23),('Tom',24) ,('Jerry',25) ,('Spike',26) ,('Buffy',27) ,('Willow',23);";
        exit = sqlite3_exec(db, sql2, 0, 0, &errMsg); //execute the SQL statement
        if (exit != SQLITE_OK) //check if the SQL statement was executed successfully
        {
            std::cout << "Error inserting data: " << errMsg << std::endl; //print an error message
            sqlite3_free(errMsg); //free the error message
            sqlite3_close(db); //close the database
            return 1; //exit the program
        }

        //select all rows from the table, execute the SQL statement
        const char* sql3 = "select * from students;";
        sqlite3_stmt* stmt;
        exit = sqlite3_prepare_v2(db, sql3, -1, &stmt, 0); //prepare the SQL statement
        while (sqlite3_step(stmt) == SQLITE_ROW) //loop through the rows
        {
            std::cout << "ID: " << sqlite3_column_int(stmt, 0) << std::endl; //print the ID
            std::cout << "Name: " << sqlite3_column_text(stmt, 1) << std::endl; //print the name
            std::cout << "Age: " << sqlite3_column_int(stmt, 2) << std::endl; //print the age
        }

        sqlite3_finalize(stmt); //finalize the statement
        sqlite3_close(db); //close the database
        std::cout<<"Database closed successfully"<<std::endl;
        return 0; //exit the program
}