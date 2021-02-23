/*
 * @Author: your name
 * @Date: 2020-12-20 22:03:34
 * @LastEditTime: 2020-12-23 20:58:32
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /IM_Software/ClientMain.cpp
 */
#include "Client.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, "Usage: %s [ID] [Password].\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Client client(argv[1], argv[2]);

    client.Start();

    return 0;
}
