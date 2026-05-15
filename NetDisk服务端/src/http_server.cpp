#include "http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "Mysql.h"

using namespace std;

extern CMysql* g_sql;

HttpServer* HttpServer::m_instance = NULL;

HttpServer* HttpServer::GetInstance() {
    if(m_instance == NULL) {
        m_instance = new HttpServer();
    }
    return m_instance;
}

HttpServer::HttpServer() {
    m_port = 8080;
    m_server_fd = -1;
    m_isRunning = false;
}

HttpServer::~HttpServer() {
    Stop();
}

void HttpServer::Start(int port) {
    if(m_isRunning) return;

    m_port = port;
    m_isRunning = true;

    pthread_create(&m_thread, NULL, ServerThread, this);

    cout << "HTTP服务器启动，端口: " << m_port << endl;
}

void HttpServer::Stop() {
    if(!m_isRunning) return;

    m_isRunning = false;

    if(m_server_fd != -1) {
        close(m_server_fd);
        m_server_fd = -1;
    }

    if(m_thread != 0) {
        pthread_join(m_thread, NULL);
    }

    cout << "HTTP服务器停止" << endl;
}

void* HttpServer::ServerThread(void* arg) {
    HttpServer* pThis = (HttpServer*)arg;

    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {
        perror("socket创建失败");
        return NULL;
    }

    pThis->m_server_fd = server_fd;

    // 设置地址重用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(pThis->m_port);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("绑定失败");
        close(server_fd);
        return NULL;
    }

    // 监听
    if(listen(server_fd, 128) == -1) {
        perror("监听失败");
        close(server_fd);
        return NULL;
    }

    cout << "HTTP服务器监听中..." << endl;

    while(pThis->m_isRunning) {
        // 接受连接
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if(client_fd == -1) {
            if(pThis->m_isRunning) {
                perror("接受连接失败");
            }
            continue;
        }

        cout << "新的HTTP连接来自: " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;

        // 创建新线程处理客户端
        int* pClientFd = new int(client_fd);
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, ClientHandler, pClientFd);
        pthread_detach(client_thread);
    }

    close(server_fd);
    pThis->m_server_fd = -1;

    return NULL;
}

void* HttpServer::ClientHandler(void* arg) {
    int client_fd = *(int*)arg;
    delete (int*)arg;

    char buffer[4096] = {0};
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if(bytes_read <= 0) {
        close(client_fd);
        return NULL;
    }

    string request(buffer);
    string method, path;
    ParseRequest(request, method, path);

    cout << "HTTP请求: " << method << " " << path << endl;

    // 处理/hls/路径的文件访问
    if(path.find("/hls/") == 0) {
        // 构建本地文件路径
        string filepath = "." + path;

        // 检查文件是否存在
        struct stat st;
        if(stat(filepath.c_str(), &st) == -1 || !S_ISREG(st.st_mode)) {
            SendResponse(client_fd, 404, "text/plain", "404 Not Found");
            close(client_fd);
            return NULL;
        }

        // 发送文件
        SendFile(client_fd, filepath);
    } 
    // 处理/files/路径的文件访问（通过fileid）
    else if(path.find("/files/") == 0) {
        // 解析fileid
        string fileid_str = path.substr(7); // 跳过"/files/"
        int fileid = atoi(fileid_str.c_str());

        // 查询数据库获取文件路径
        string filepath;
        if(g_sql) {
            char sqlbuf[1000];
            sprintf(sqlbuf, "select f_path from t_file where f_id = %d", fileid);
            list<string> lstRes;
            bool res = g_sql->SelectMysql(sqlbuf, 1, lstRes);

            if(res && lstRes.size() == 1) {
                filepath = lstRes.front();
            } else {
                // 如果t_file找不到，尝试user_file_info
                sprintf(sqlbuf, "select f_path from user_file_info where f_id = %d limit 1", fileid);
                lstRes.clear();
                res = g_sql->SelectMysql(sqlbuf, 1, lstRes);
                if(res && lstRes.size() == 1) {
                    filepath = lstRes.front();
                }
            }
        }

        // 检查文件是否存在
        if(filepath.empty()) {
            SendResponse(client_fd, 404, "text/plain", "404 Not Found");
            close(client_fd);
            return NULL;
        }

        struct stat st;
        if(stat(filepath.c_str(), &st) == -1 || !S_ISREG(st.st_mode)) {
            SendResponse(client_fd, 404, "text/plain", "404 Not Found");
            close(client_fd);
            return NULL;
        }

        // 发送文件
        SendFile(client_fd, filepath);
    }
    else {
        SendResponse(client_fd, 404, "text/plain", "404 Not Found");
        close(client_fd);
        return NULL;
    }

    close(client_fd);
    return NULL;
}

void HttpServer::SendResponse(int client_fd, int status, const string& content_type, const string& content) {
    char response[4096] = {0};
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status, content_type.c_str(), content.size());

    send(client_fd, response, strlen(response), 0);
    send(client_fd, content.c_str(), content.size(), 0);
}

void HttpServer::SendFile(int client_fd, const string& filepath) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if(fd == -1) {
        SendResponse(client_fd, 404, "text/plain", "404 Not Found");
        return;
    }

    struct stat st;
    fstat(fd, &st);

    // 发送HTTP头
    string mime_type = GetMimeType(filepath);
    char header[4096] = {0};
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        mime_type.c_str(), (long long)st.st_size);

    send(client_fd, header, strlen(header), 0);

    // 发送文件内容
    char buffer[4096];
    ssize_t bytes_read;
    while((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }

    close(fd);
}

void HttpServer::ParseRequest(const string& request, string& method, string& path) {
    size_t pos1 = request.find(' ');
    if(pos1 == string::npos) return;

    method = request.substr(0, pos1);

    size_t pos2 = request.find(' ', pos1 + 1);
    if(pos2 == string::npos) return;

    path = request.substr(pos1 + 1, pos2 - pos1 - 1);
}

string HttpServer::GetMimeType(const string& filepath) {
    size_t dot_pos = filepath.rfind('.');
    if(dot_pos == string::npos) {
        return "application/octet-stream";
    }

    string ext = filepath.substr(dot_pos + 1);
    for(char& c : ext) {
        c = tolower(c);
    }

    if(ext == "m3u8") {
        return "application/vnd.apple.mpegurl";
    } else if(ext == "ts") {
        return "video/MP2T";
    } else if(ext == "mp4") {
        return "video/mp4";
    } else if(ext == "avi") {
        return "video/x-msvideo";
    } else if(ext == "mkv") {
        return "video/x-matroska";
    } else if(ext == "mp3") {
        return "audio/mpeg";
    } else if(ext == "wav") {
        return "audio/wav";
    } else if(ext == "flac") {
        return "audio/flac";
    } else if(ext == "aac") {
        return "audio/aac";
    } else if(ext == "ogg") {
        return "audio/ogg";
    } else if(ext == "m4a") {
        return "audio/mp4";
    } else if(ext == "wma") {
        return "audio/x-ms-wma";
    } else if(ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    } else if(ext == "png") {
        return "image/png";
    } else {
        return "application/octet-stream";
    }
}
