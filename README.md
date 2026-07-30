# 项目简介

基于 epoll 的高性能聊天/文件传输服务器，支持多客户端并发。

## 核心特性

- **网络模型**：单线程 epoll + 非阻塞 I/O，支持高并发连接
- **实时通信**：支持私聊、群聊（广播）
- **文件传输**：支持用户间文件转发、服务端文件下载、文件列表查询
- **并发模型**：线程池处理耗时的 I/O 操作，不阻塞主事件循环
- **日志系统**：异步日志（消息队列 + 独立日志线程），按天滚动

## 适用场景

- 学习 Linux 网络编程和 C++ 服务端开发的参考项目
- 小型聊天室或内部通信工具
- 可作为游戏服务器、物联网网关等需要长连接通信的项目的基础框架

## 协议格式

所有消息均为二进制协议：

[1字节消息类型][4字节长度(网络序)][负载数据]

详细消息类型见 `include/Message.h` 中的 `Msg_type` 枚举。

## 功能特性

- 用户注册/登录、私聊/广播、文件传输

---

## 技术栈

- C++17 / Linux / epoll
- MySQL / MySQL C API
- 多线程 / 连接池 / 异步日志

---

## 项目结构

```text
Chat_server_2.0/
├── CMakeLists.txt          # CMake 构建文件
├── README.md               # 项目说明
├── .gitignore              # Git 忽略文件
│
├── include/                # 头文件
│   ├── Manage_sock.h
│   ├── Message.h
│   ├── Msg_queue.h
│   ├── Thlog_w.h
│   ├── Logger.h
│   ├── Sql_table.h
│   └── Sql_con_que.h
│
├── src/                    # 源文件
│   ├── Manage_sock.cpp
│   ├── Message.cpp
│   ├── Msg_queue.cpp
│   ├── Thlog_w.cpp
│   ├── Logger.cpp
│   ├── Sql_table.cpp
│   ├── Sql_con_que.cpp
│   └── main.cpp
│
├── bin/                    # 编译输出目录（自动生成）
│   ├── chat_server         # 可执行文件
│   └── received_files/     # 接收文件存储目录
│
├── logs/                   # 日志目录（自动生成）
│   └── server_2026-07-30.log
│
└── build/                  # 临时构建目录（自动生成，可略）

### 依赖安装
本项目依赖以下库和工具，请根据你的 Linux 发行版安装

Linux 内核 2.6+（支持 epoll）
g++ 7.0+ 或 clang 5.0+（支持 C++17）
CMake 3.10+

sudo apt install build-essential cmake git
sudo apt install libmysqlclient-dev

注：本项目仅使用 C++ 标准库和系统调用（socket、epoll），无第三方运行时依赖。

### 数据库配置

创建数据库:
CREATE DATABASE IF NOT EXISTS chat_db_2 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

创建用户表:
USE chat_db_2;

CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

创建专用数据库账户:
CREATE USER 'chat_app'@'localhost' IDENTIFIED BY '你的密码';
GRANT ALL PRIVILEGES ON chat_db.* TO 'chat_app'@'localhost';
FLUSH PRIVILEGES;

修改数据库连接配置:
mysql_real_connect(
    conn_,
    "127.0.0.1",   // MySQL 服务器地址
    "root",        // 数据库用户名
    "你的密码",     // 数据库密码
    "chat_db",     // 数据库名
    3306,          // 端口号
    nullptr,
    0
);

启动服务器后，查看日志文件 logs/server_YYYY-MM-DD.log，应看到：[INFO] Connected to MySQL successfully
