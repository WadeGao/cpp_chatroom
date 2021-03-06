# C++实现的TCP即时通讯软件

<p align="center">
<a href="#build" alt="build"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" /></a>
<a href="#platform" alt="platform"><img src="https://img.shields.io/badge/platform-MacOS%7C%20Linux%20%7C%20WSL-brightgreen" /></a>
<a href="#IPv6" alt="platform"><img src="https://img.shields.io/badge/network-IPv4%20%7C%20IPv6-brightgreen" /></a>
<a href="#Protocol" alt="platform"><img src="https://img.shields.io/badge/protocol-TCP-brightgreen" /></a>
</p>
<img src="https://i.loli.net/2021/03/06/bd5er7Tj3SGxMJA.png" alt="截图录屏_选择区域_20210306162652.png" style="zoom:100%;" />

## 基本功能

群聊:white_check_mark:、私聊:white_check_mark:、获取用户列表:white_check_mark:

用户数据持久化:white_check_mark:、支持IPv6网络:white_check_mark:

## 安装

### Linux:penguin:
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
+ 加密数据传输:closed_lock_with_key:
+ 增加图形界面:computer:
+ 聊天记录持久化:floppy_disk:
+ 文件传输:file_folder:

