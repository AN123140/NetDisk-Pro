# NetDisk Pro

C++ 网盘系统，包含 Qt 桌面客户端和 Linux 服务端，支持文件上传下载、断点续传、秒传、文件分享、在线视频播放等功能。

## 项目结构

```
├── wnagpanxiangmu/       # 客户端（Qt 5.12 + MinGW 32-bit）
│   ├── ckernel.cpp/h     # 核心控制类，协议映射与业务逻辑
│   ├── maindialog.cpp/h  # 主界面（文件列表、上传下载、分享）
│   ├── logindialog.cpp/h # 登录/注册界面
│   ├── playerdialog.cpp/h# 在线视频播放界面
│   ├── vedioplayer.cpp/h # FFmpeg + SDL2 视频播放器
│   ├── netapi/           # TCP 网络通信模块
│   ├── sqlapi/           # SQLite 本地数据库（断点续传任务记录）
│   ├── md5/              # MD5 文件校验
│   ├── opengl/           # OpenGL 渲染模块
│   ├── ffmpeg-4.2.2/     # FFmpeg 开发库
│   └── SDL2-2.0.10/      # SDL2 开发库
│
└── NetDisk服务端/         # 服务端（Linux + epoll + MySQL）
    ├── src/
    │   ├── main.cpp          # 入口，启动 TCP + HTTP 服务
    │   ├── TCPKernel.cpp     # TCP 核心服务，epoll 事件循环
    │   ├── block_epoll_net.cpp# epoll 网络封装
    │   ├── clogic.cpp        # 业务逻辑处理（协议分发）
    │   ├── Mysql.cpp         # MySQL 数据库操作
    │   ├── hls_transcode.cpp # FFmpeg HLS 视频转码
    │   ├── http_server.cpp   # HTTP 服务器（提供 HLS 播放流）
    │   └── Thread_pool.cpp   # 线程池
    └── include/              # 头文件
```

## 功能特性

- **用户系统** — 注册 / 登录（手机号 + 密码）
- **文件管理** — 上传、下载、新建文件夹、删除文件/文件夹
- **断点续传** — 上传和下载均支持断点续传，任务持久化到本地 SQLite
- **秒传** — 基于 MD5 校验的文件秒传
- **文件分享** — 生成分享链接，他人可通过分享码获取文件
- **文件夹操作** — 支持文件夹的上传、下载、分享
- **在线播放** — 服务端 FFmpeg 实时转码为 HLS，客户端 SDL2 + FFmpeg 播放
- **上传下载暂停** — 支持暂停/恢复传输任务

## 技术栈

| 模块 | 技术 |
|------|------|
| 客户端 UI | Qt 5.12 (Widgets) |
| 客户端网络 | 自封装 TCP 通信 |
| 客户端本地存储 | SQLite |
| 视频播放 | FFmpeg 4.2.2 + SDL2 2.0.10 |
| 服务端网络 | Linux epoll |
| 服务端数据库 | MySQL |
| 服务端转码 | FFmpeg HLS |
| 服务端 HTTP | 自实现 HTTP Server |
| 编译器 | MinGW 32-bit (客户端) / GCC (服务端) |

## 自定义协议

客户端与服务端之间使用自定义二进制协议通信，基于 `PackType` 字段区分协议类型，协议映射表驱动分发。主要协议：

| 协议号 | 说明 |
|--------|------|
| 10000-10001 | 注册请求/响应 |
| 10002-10003 | 登录请求/响应 |
| 10004-10007 | 文件上传/内容传输 |
| 10008-10009 | 获取文件列表 |
| 10010-10014 | 文件下载/文件头 |
| 10015-10016 | 新建文件夹 |
| 10017 | 秒传响应 |
| 10018-10023 | 文件分享相关 |
| 10025-10026 | 删除文件 |
| 10027-10029 | 断点续传 |
| 10030-10031 | HLS 在线播放 |

## 编译运行

### 客户端

1. 安装 Qt 5.12 + MinGW 32-bit
2. 用 Qt Creator 打开 `wnagpanxiangmu/wnagpanxiangmu.pro`
3. 编译运行

### 服务端

1. 安装依赖：MySQL 开发库、pthread
2. 编译：

```bash
cd NetDisk服务端/src
make
```

3. 运行：

```bash
./NetDisk [port]    # 默认端口 8000，HTTP 端口 8080
```

4. 确保 MySQL 服务已启动，数据库名 `NewNetDisk`，连接配置见 `include/packdef.h`

## 许可证

MIT
