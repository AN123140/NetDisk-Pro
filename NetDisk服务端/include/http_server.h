#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "packdef.h"
#include <string>
#include <pthread.h>

using namespace std;

class HttpServer {
public:
    static HttpServer* GetInstance();

    // 启动服务器
    void Start(int port = 8080);

    // 停止服务器
    void Stop();

private:
    // 服务器线程
    static void* ServerThread(void* arg);

    // 处理客户端连接
    static void* ClientHandler(void* arg);

    // 发送HTTP响应
    static void SendResponse(int client_fd, int status, const string& content_type, const string& content);

    // 发送文件
    static void SendFile(int client_fd, const string& filepath);

    // 解析HTTP请求
    static void ParseRequest(const string& request, string& method, string& path);

    // 获取文件的MIME类型
    static string GetMimeType(const string& filepath);

    int m_port;
    int m_server_fd;
    bool m_isRunning;
    pthread_t m_thread;

    static HttpServer* m_instance;

    HttpServer();
    ~HttpServer();
    HttpServer(const HttpServer&);
    HttpServer& operator=(const HttpServer&);
};

#endif // HTTP_SERVER_H
