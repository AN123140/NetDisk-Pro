#ifndef _MYSQL_H
#define _MYSQL_H
#include "packdef.h"
#include <mysql/mysql.h>
#include<list>
#include<string>

using namespace  std;


class CMysql{
public:
    int ConnectMysql(const char *server, const char *user, const char *password, const char *database);
    int SelectMysql(char* szSql,int nColumn,list<string>& lst);
    int UpdataMysql(char *szsql);
    void DisConnect();


    // 视频转码相关操作
    int InsertVideoTranscode(int fileid, const char* md5, int status, const char* hls_path);
    int UpdateVideoTranscodeStatus(int fileid, int status, const char* hls_path = NULL);
    int GetVideoTranscode(int fileid, int& status, string& hls_path, string& md5);
    int GetVideoTranscodeByMD5(const char* md5, int& fileid, int& status, string& hls_path);
private:
    MYSQL *conn;

    pthread_mutex_t m_lock;
};




#endif
