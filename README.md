<!--
 * @Author: your name
 * @Date: 2021-03-01 20:54:21
 * @LastEditTime: 2021-03-02 22:43:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/README.md
-->
# C++实现的TCP即时通讯软件

<p align="center">
<a href="#build" alt="build"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" /></a>
<a href="#platform" alt="platform"><img src="https://img.shields.io/badge/platform-MacOS%7C%20Linux%20%7C%20WSL-brightgreen" /></a>
<a href="#IPv6" alt="platform"><img src="https://img.shields.io/badge/network-IPv4%20%7C%20IPv6-brightgreen" /></a>
<a href="#Protocol" alt="platform"><img src="https://img.shields.io/badge/protocol-TCP-brightgreen" /></a>
</p>

![截图录屏_deepin-terminal_20210302193612.png](https://i.loli.net/2021/03/02/fNZztWLxnYPr5hU.png)
## 安装
### Linux
#### 安装```libmysqlclient-dev```
```bash
sudo apt install libmysqlclient-dev
```
#### 安装CMake
```bash
sudo apt install cmake
```
#### 安装本体
```bash
git clone https://gitee.com/wadegao/cpp-imsoftware.git ./cpp-imsoftware/
cd cpp-imsoftware/
sh compile.sh
```
## 运行
<p align="center">
<a href="#Account4Evaluation" alt="platform"><img src="https://img.shields.io/badge/%E4%BD%93%E9%AA%8C%E8%B4%A6%E5%8F%B7-3221514781%20%7C%20623783787-brightgreen" /></a>
<a href="#Password4Evaluation" alt="platform"><img src="https://img.shields.io/badge/%E4%BD%93%E9%AA%8C%E5%AF%86%E7%A0%81-12345678-9cf" /></a>
<a href="#MySQL_Password4Evaluation" alt="platform"><img src="https://img.shields.io/badge/MySQL%E6%9C%8D%E5%8A%A1%E5%99%A8%E5%AF%86%E7%A0%81-12345678-9cf" /></a>
</p>

+ 服务端
```bash
cd bin/
./Server
```

+ 客户端
```bash
cd bin/
./Client <ID> <Password>
```

## 未来优化方向
+ 对数据传输进行加密
+ 增加图形界面
+ 补充私聊、好友列表、用户组的功能