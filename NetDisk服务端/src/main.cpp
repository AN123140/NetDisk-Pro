#include <TCPKernel.h>

#include "http_server.h"
#include <pthread.h>

//HTTP服务器线程
void* http_server_thread(void* arg)
{
    int http_port = 8080;
    HttpServer::GetInstance()->Start(http_port);
    return NULL;
}

int main(int argc,char *argv[])
{
    int port = 8000;
    if( argc >= 2 )
    {
        port = atoi(argv[1]);
    }
    TcpKernel * pKernel =  TcpKernel::GetInstance();

    //开启服务 给定端口, 可以使用输入的port
    pKernel->Open( port );
    cout << "port:" << port << endl ;
    //启动HTTP服务器线程
    pthread_t http_tid;
    pthread_create(&http_tid, NULL, http_server_thread, NULL);
    pthread_detach(http_tid);
    // 事件循环 : 循环监听事件
    pKernel->EventLoop();

    pKernel->Close();

    return 0;
}
