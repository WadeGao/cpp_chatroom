<!--
 * @Author: your name
 * @Date: 2021-03-01 20:54:21
 * @LastEditTime: 2021-03-01 21:23:43
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/README.md
-->
# C++实现的TCP即时通讯软件
本仓库为Linux平台下的基于TCP协议的C++即时通讯软件的代码，支持IPv6网络

## 基本原理
本软件面向文本传输，为保证文本信息传输正确性，故采用TCP协议进行网络通信；
为便于数据迁移，采用账号数据存储于MySQL数据库的实现
+ 服务端：接收客户端告知的账号信息，通过查询MySQL服务器对账号信息进行校核，并拒绝重复登录等；通过```epoll```提高后台网络交互的并发性
+ 客户端：发送身份信息后，fork一个子进程，子进程负责读取标准输入流的用户输入，父进程负责处理并发送到服务端，两个进程采用管道进行通信；不同账号的父进程实体之间通过转发服务器进行通信

## 环境依赖
+ 服务端：```libmysqlclient-dev```, ```MySQL```
+ ```CMake```
## 安装过程
+ 克隆本库：```git clone https://gitee.com/wadegao/cpp-imsoftware.git```
+ 运行脚本：```sh compile.sh```
+ 进入```bin/```文件夹取得可执行文件

## 使用方法
+ 服务端：```./Server```
+ 客户端：```./Client <ID> <Password>```

## 未来优化方向
+ 对数据传输进行加密
+ 增加图形界面
+ 补充私聊、好友列表、用户组的功能