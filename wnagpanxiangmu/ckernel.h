#ifndef CKERNEL_H
#define CKERNEL_H
#include"common.h"
#include <QObject>
#include"maindialog.h"
#include"packdef.h"
#include"logindialog.h"
#include"csqlite.h"
//核心处理类
//单例：拒绝无缘无故产生内存泄漏
//1.构造 拷贝构造，析构 私有化 2.提供静态的公有的获取对象方法

//协议映射表

//类成员函数指针
class CKernel;
typedef void (CKernel::*PFUN)(unsigned int lSendIP,char*buf,int nlen);
//#include<INetMediator.h>
class INetMediator;
class PlayerDialog;
//#define USE_SERVER 1
class CKernel : public QObject
{
    Q_OBJECT
private:
    //explicit:防止隐式类型进行转换（c++会内存泄漏）
    explicit CKernel(QObject *parent = nullptr);
    explicit CKernel(const CKernel & kernel){}
    ~CKernel(){}
    void loadIniFile();
    void setNetPackMap();
    void setSystemPath();
signals:
    void SIG_updateUploadFileProgress(int timestamp,int pos);
    void SIG_updateDownloadFileProgress(int timestamp,int pos);
public:
    static CKernel* GetInstance(){
       static CKernel kernel;
       return &kernel;
    }
private slots:

    //普通槽函数
    void slot_destory();

    void slot_registerCommit(QString tel,QString password,QString name);
    void slot_loginCommit(QString tel,QString password);
    void slot_uploadFile(QString path,QString dir);
    //上传什么路径的文件夹，到什么目录下面
    void slot_uploadFolder( QString path, QString dir);
    void slot_getCurDirFileList();
    //什么文件id，什么目录下的文件 下载
    void slot_downloadFile( int fileid , QString dir);
    //什么文件id，什么目录下的文件夹 下载
    void slot_downloadFolder( int fileid , QString dir);
    //什么路径下创建什么名字的文件夹
    void slot_addFolder( QString name , QString dir );
    //改变路径
    void slot_changeDir(QString dir);
    // 分享  什么目录下面的文件列表
    void slot_shareFile( QVector<int> fieldArray , QString dir );
    void slot_getMyShare();
    //什么目录下面 添加什么分享码的文件
    void slot_getShareByLink( int code , QString dir);
     void slot_deleteFile(QVector<int> fileidArray, QString dir);
     //设置下载暂停 0 开始 1 暂停
     void slot_setUploadPause( int timestamp , int isPause );
     //设置下载暂停 0 开始 1 暂停
     void slot_setDownloadPause( int timestamp , int isPause );
     //在线播放
     void slot_playOnline( int fileid , QString dir );
     //处理获取HLS播放地址的响应
     void slot_dealGetHlsUrlRs(unsigned int lSendIP,char*buf,int nlen);
    //网络槽函数
    void slot_dealClientData(unsigned int lSendIP,char*buf,int nlen);
    void slot_dealLoginRs(unsigned int lSendIP,char*buf,int nlen);
    void slot_dealRegisterRs(unsigned int lSendIP,char*buf,int nlen);
    void slot_dealUploadFileRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealFileContentRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealGetFileInfoRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealFileHeaderRq(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealFileContentRq(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealAddFolderRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealQuickUploadRs( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealShareFileRs( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealMyShareRs( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealGetShareRs( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealFolderHeadRq( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealDeleteFileRs( unsigned int lSendIP , char* buf , int nLen );
    void slot_dealContinueUploadRs( unsigned int lSendIP , char* buf , int nLen );

#ifdef USE_SERVER
    void slot_dealServerData(unsigned int lSendIP,char*buf,int nlen);
#endif

private:
    void sendData(char* buf,int len);
    void SendData(char * buf,int len);
private:
    MainDialog * m_mainDialog;
    LoginDialog * m_loginDialog;
    PlayerDialog * m_playerDialog;
    QString m_ip;
    QString m_port;

    QString m_name;
    int m_id;
    QString m_curDir;
    INetMediator *m_tcpClient;
    QString m_sysPath;//默认存储系统路径  exe同级下 NetDisk文件夹下

    std::map<int,FileInfo>m_mapTimeStampToFileInfo;
#ifdef USE_SERVER
    INetMediator *m_tcpServer;
#endif

    PFUN m_netPackMap[_DEF_PACK_COUNT];
    //退出标志
    bool m_quit;
    CSqlite * m_sql;
private:
    void InitDatabase(int id);
    void slot_writeUploadTask(FileInfo &info);
    void slot_writeDownloadTask(FileInfo &info);
    void slot_deleteUploadTask(FileInfo &info);
    void slot_deleteDownloadTask(FileInfo &info);
    void slot_getUploadTask(QList<FileInfo> &infoList);
    void slot_getDownloadTask(QList<FileInfo> &infoList);
};

#endif // CKERNEL_H
