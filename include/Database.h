/*
 * @Author: your name
 * @Date: 1970-01-01 08:00:00
 * @LastEditTime: 2021-02-23 10:23:30
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /4Database/Database.h
 */
#ifndef DATABASE_H_
#define DATABASE_H_

#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <vector>

class Database
{
private:
    MYSQL mysql{};

public:
    Database();
    //断开数据库连接
    ~Database();
    //连接到数据库
    bool ConnectMySQL(const char *host, const char *user, const char *db, unsigned int port);
    //从数据库中创建表、更新表、 删除表
    bool CUD_MySQL(const std::string &query);
    //返回数据库中表的集合
    std::vector<std::string> GetTables();
    //返回数据库中表的集合到屏幕
    void GetTables2Screen();
    //读取数据库内容
    std::vector<std::vector<std::string>> ReadMySQL(const std::string &query);
    //读取数据库内容到屏幕
    void ReadMySQL2Screen(const std::string &query);
};

#endif
