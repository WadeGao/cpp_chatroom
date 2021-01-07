/*
 * @Author: your name
 * @Date: 1970-01-01 08:00:00
 * @LastEditTime: 2020-12-22 21:47:38
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /4Database/Database.cpp
 */
#include "Database.h"

Database::Database()
{
    mysql_init(&mysql);
}

Database::~Database()
{
    mysql_close(&mysql);
    //fprintf(stdout, "Database Closed\n");
}

bool Database::ConnectMySQL(const char *host, const char *user, const char *db, bool IF_PROVIDE_PWD, const char *provided_pwd)
{
    if (IF_PROVIDE_PWD)
    {
        if (!mysql_real_connect(&mysql, host, user, provided_pwd, db, 3306, NULL, CLIENT_FOUND_ROWS))
        {
            //fprintf(stderr, "Connect Failed: %s\n\n", mysql_error(&mysql));
            return false;
        }
        //fprintf(stdout, "Connect successfully\n");
        return true;
    }

    std::string pwd;
    char failedTimes = 0;
    while (true)
    {
        fprintf(stdout, "Please input password: ");
        std::cin >> pwd;
        if (!mysql_real_connect(&mysql, host, user, pwd.c_str(), db, 3306, NULL, CLIENT_FOUND_ROWS))
        {
            fprintf(stderr, "Connect Failed: %s\n\n", mysql_error(&mysql));
            if (++failedTimes >= 3)
            {
                fprintf(stderr, "Failed too many times, deny to serve.\n");
                return false;
            }
        }
        else
        {
            //fprintf(stdout, "Connect successfully\n");
            break;
        }
    }
    return true;
}

bool Database::CUD_MySQL(const std::string &query)
{
    auto res = mysql_query(&mysql, query.c_str());
    bool isSuccessful = (res == 0);
    isSuccessful ? fprintf(stdout, "\033[32m%s\033[0m\tQuery Successfully!\n", query.c_str()) : fprintf(stderr, "\033[31m%s\033[0m\tSyntax Error!\n", query.c_str());
    return isSuccessful;
}

std::vector<std::string> Database::GetTables()
{
    std::vector<std::string> allTables;
    CUD_MySQL("SHOW TABLES;");

    MYSQL_RES *result = mysql_store_result(&mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        allTables.push_back(row[0]);
    }
    return allTables;
}

void Database::GetTables2Screen()
{
    auto tables = this->GetTables();
    for (const auto &th : tables)
        fprintf(stdout, "%s\n", th.c_str());
}

std::vector<std::vector<std::string>> Database::ReadMySQL(const std::string &query)
{
    if (mysql_query(&mysql, query.c_str()))
    {
        std::vector<std::vector<std::string>> ret;
        return ret;
    }
    MYSQL_RES *result = mysql_store_result(&mysql);

    auto row_count = mysql_num_rows(result);
    auto field_count = mysql_num_fields(result);

    MYSQL_ROW row;
    decltype(row_count) cur_line = 0;
    std::vector<std::vector<std::string>> res(row_count, std::vector<std::string>(field_count, ""));
    while ((row = mysql_fetch_row(result)))
    {
        for (decltype(field_count) i = 0; i < field_count; i++)
            res.at(cur_line).at(i) = row[i];
        cur_line++;
    }
    mysql_free_result(result);
    return res;
}

void Database::ReadMySQL2Screen(const std::string &query)
{
    if (mysql_query(&mysql, query.c_str()))
        exit(2);

    MYSQL_RES *result = mysql_store_result(&mysql);
    auto field_count = mysql_num_fields(result);
    MYSQL_FIELD *field = nullptr;

    for (decltype(field_count) i = 0; i < field_count; i++)
        fprintf(stdout, "\033[32m%s\033[0m\t", mysql_fetch_field_direct(result, i)->name);
    fprintf(stdout, "\n");

    auto ret = this->ReadMySQL(query);
    for (const auto &iter : ret)
    {
        for (const auto &it : iter)
            fprintf(stdout, "%s\t\t", it.c_str());
        fprintf(stdout, "\n");
    }
}