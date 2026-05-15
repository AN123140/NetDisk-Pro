#include "ckernel.h"
#include<QDebug>
#include"TcpClientMediator.h"
#include"TcpServerMediator.h"
#include<QMessageBox>
#include<QCoreApplication>
#include<QFileInfo>
#include<QSettings>
#include<QThread>
#include"playerdialog.h"
#define NetMap( a ) m_netPackMap[a - _DEF_PACK_BASE]
void CKernel::setNetPackMap()
{
    memset(m_netPackMap,0,sizeof(PFUN)*_DEF_PACK_COUNT);
    //m_netPackMap[_DEF_PACK_LOGIN_RS-_DEF_PACK_BASE] =&CKernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_REGISTER_RS) =&CKernel::slot_dealRegisterRs;
    NetMap(_DEF_PACK_LOGIN_RS) =&CKernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_UPLOAD_FILE_RS) =&CKernel::slot_dealUploadFileRs;
    NetMap(_DEF_PACK_FILE_CONTENT_RS) =&CKernel::slot_dealFileContentRs;
    NetMap(_DEF_PACK_GET_FILE_INFO_RS) =&CKernel::slot_dealGetFileInfoRs;
    NetMap(_DEF_PACK_FILE_HEADER_RQ) =&CKernel::slot_dealFileHeaderRq;
    NetMap( _DEF_PACK_FILE_CONTENT_RQ ) = &CKernel::slot_dealFileContentRq;
    NetMap( _DEF_PACK_ADD_FOLDER_RS ) = &CKernel::slot_dealAddFolderRs;
    NetMap( _DEF_PACK_QUICK_UPLOAD_RS ) = &CKernel::slot_dealQuickUploadRs;
    NetMap( _DEF_PACK_SHARE_FILE_RS ) = &CKernel::slot_dealShareFileRs;
    NetMap( _DEF_PACK_MY_SHARE_RS ) = &CKernel::slot_dealMyShareRs;
    NetMap( _DEF_PACK_GET_SHARE_RS ) = &CKernel::slot_dealGetShareRs;
    NetMap( _DEF_PACK_FOLDER_HEADER_RQ ) = &CKernel::slot_dealFolderHeadRq;
    NetMap( _DEF_PACK_DELETE_FILE_RS ) = &CKernel::slot_dealDeleteFileRs;
    NetMap( _DEF_PACK_CONTINUE_UPLOAD_RS ) = &CKernel::slot_dealContinueUploadRs;
    NetMap( _DEF_PACK_GET_HLS_URL_RS ) = &CKernel::slot_dealGetHlsUrlRs;
}
#include<QDir>
void CKernel::setSystemPath()
{
    QString path = QCoreApplication::applicationDirPath() + "/NetDisk";

    QDir dir;
    // 没有文件夹 创建
    if( !dir.exists( path ) ) {
        dir.mkdir( path ); // 只能创建一层
    }
    m_sysPath = path;
}

#include"md5.h"
#define MD5_KEY "1234"
//password_1234
//生成MD5函数
static std::string getMD5( QString val)//static 函数 当前文件可用
{
    QString str = QString("%1_%2").arg(val).arg(MD5_KEY);
    MD5 md( str.toStdString());
    qDebug()<< str <<"md5:"<< md.toString().c_str();
    return md.toString();
}

#include<QTextCodec>

// QString -> char* gb2312
void Utf8ToGB2312( char* gbbuf , int nlen ,QString& utf8)
{
    //转码的对象
    QTextCodec * gb2312code = QTextCodec::codecForName( "gb2312");
    //QByteArray char 类型数组的封装类 里面有很多关于转码 和 写IO的操作
    QByteArray ba = gb2312code->fromUnicode( utf8 );// Unicode -> 转码对象的字符集

    strcpy_s ( gbbuf , nlen , ba.data() );
}
//获取文件Md5
static std::string getFileMD5(QString path)
{
    //打开文件，读取文件内容，读到MD5类，生成MD5
    FILE* pFile=nullptr;
    //fopen使用ascii码，qt默认utf-8，需要进行转码Utf8ToGB2312
    char buf[1000]="";
    Utf8ToGB2312(buf,1000,path);
    pFile = fopen(buf,"rb");
    if(!pFile)
    {
        qDebug()<<"file md5 open fail";
        return string();
    }
    int len=0;
    MD5 md;
    do{
        len=fread(buf ,1,1000,pFile);
        md.update(buf,len);
        //为了避免阻塞窗口线程，影响事件循环，加入下面的处理 将信号取出并执行
        QCoreApplication::processEvents( QEventLoop::AllEvents , 100 );
    }while(len>0);
    fclose(pFile);
    qDebug()<<"md5:"<<md.toString().c_str();

    return md.toString();
}


void CKernel::sendData(char *buf, int len)
{
    m_tcpClient->SendData(0,buf,len);
}
CKernel::CKernel(QObject *parent) : QObject(parent),m_id(0),m_curDir("/"),m_quit(false)
{
    setNetPackMap();
    //加载配置文件
    loadIniFile();



    //设置默认路径
    setSystemPath();
#ifdef USE_SERVER

    m_tcpServer=new TcpServerMediator;
    connect(m_tcpServer,SIGNAL(SIG_ReadyData(uint,char*,int)),this,SLOT(slot_dealServerData(uint,char*,int)));
    m_tcpServer->OpenNet();
#endif
    //客户端连接真实地址
    m_tcpClient=new TcpClientMediator;
    connect(m_tcpClient,SIGNAL(SIG_ReadyData(uint,char*,int)),this,SLOT(slot_dealClientData(uint,char*,int)));
    m_tcpClient->OpenNet(m_ip.toStdString().c_str(),m_port.toInt());
    m_loginDialog=new LoginDialog;

    connect(m_loginDialog,SIGNAL(SIG_registerCommit(QString,QString,QString)),this,SLOT(slot_registerCommit(QString,QString,QString)));
    connect(m_loginDialog,SIGNAL(SIG_loginCommit(QString,QString)),this,SLOT(slot_loginCommit(QString,QString)));
    m_loginDialog->show();
    m_mainDialog=new MainDialog;
    connect(m_mainDialog,SIGNAL(SIG_close()),this,SLOT(slot_destory()));
    connect(m_mainDialog,SIGNAL(SIG_uploadFile(QString,QString)),this,SLOT(slot_uploadFile(QString,QString)));
    connect(this,SIGNAL(SIG_updateUploadFileProgress(int,int)),m_mainDialog,SLOT(slot_updateUploadFileProgress(int ,int)));
    //m_mainDialog->show();
    connect( m_mainDialog, SIGNAL(SIG_downloadFile(int,QString)), this ,SLOT(slot_downloadFile(int,QString)) );
    connect( m_mainDialog, SIGNAL(SIG_downloadFolder(int,QString)), this ,SLOT(slot_downloadFolder(int,QString)) );
    connect( this ,SIGNAL(SIG_updateDownloadFileProgress(int,int)),m_mainDialog, SLOT(slot_updateDownloadFileProgress(int,int)));
    connect( m_mainDialog , SIGNAL(SIG_addFolder(QString,QString)) ,this , SLOT(slot_addFolder(QString,QString) ) );
    connect( m_mainDialog , SIGNAL( SIG_changeDir(QString) ) , this , SLOT( slot_changeDir(QString) ) );
    connect( m_mainDialog , SIGNAL( SIG_uploadFolder(QString,QString)) , this , SLOT( slot_uploadFolder(QString,QString)) );
    connect( m_mainDialog , SIGNAL( SIG_shareFile(QVector<int>,QString) ),this , SLOT( slot_shareFile(QVector<int>,QString) ) );
    connect( m_mainDialog , SIGNAL(SIG_getShareByLink(int,QString)) , this, SLOT( slot_getShareByLink(int,QString) ) );
    connect( m_mainDialog , SIGNAL(SIG_deleteFile(QVector<int>,QString)) , this ,
        SLOT(slot_deleteFile(QVector<int>,QString) ) );
    connect( m_mainDialog , SIGNAL(SIG_setUploadPause(int,int)) ,
        this , SLOT(slot_setUploadPause(int,int)) );
    connect( m_mainDialog , SIGNAL(SIG_setDownloadPause(int,int)) ,
        this , SLOT(slot_setDownloadPause(int,int)) );
    connect( m_mainDialog , SIGNAL(SIG_playOnline(int,QString)) ,
        this , SLOT(slot_playOnline(int,QString)) );
    m_playerDialog = nullptr;
#ifdef USE_SERVER

    //测试 对服务器发送数据
    char strBuf[100]="hello server";
    int len=strlen("hello server")+1;
    m_tcpClient->SendData(0,strBuf,len);//客户端一定发给服务器的，套接字参数随意
#endif

    //STRU_LOGIN_RQ rq;
    //m_tcpClient->SendData(0,(char*)&rq,sizeof(rq));
}

void CKernel::loadIniFile()
{
    //默认值
    m_ip="192.168.23.132";
    m_port="8004";
    //获取exe目录  C:/build-debug
    QString path = QCoreApplication::applicationDirPath()+"/config.ini";
    //根据目录
    //看文件是否存在 存在加载 不存在就创建写入默认值
    QFileInfo info(path);
    if(info.exists())
    {
        //存在
         QSettings setting(path,QSettings::IniFormat);
         //打开组
         setting.beginGroup("net");
         QVariant strIP=setting.value("ip","");
         QVariant strPort=setting.value("port","");
         if(!strIP.toString().isEmpty()) m_ip=strIP.toString();
         if(!strPort.toString().isEmpty()) m_port=strPort.toString();
         //关闭组
         setting.endGroup();
    }else{
        //不存在
        QSettings setting(path,QSettings::IniFormat);
        //打开组
        setting.beginGroup("net");
        //设置 key value
        setting.setValue("ip",m_ip);
        setting.setValue("port",m_port);
        //关闭组
        setting.endGroup();
    }
    qDebug()<<"ip:"<<m_ip<<"port:"<<m_port;
}

void CKernel::slot_destory()
{
    m_quit = true;
    qDebug()<<__func__;
    m_tcpClient->CloseNet();
    delete m_tcpClient;
    delete m_mainDialog;
    delete m_loginDialog;
}

void CKernel::slot_registerCommit(QString tel, QString password, QString name)
{
    STRU_REGISTER_RQ rq;
    strcpy(rq.tel,tel.toStdString().c_str());
    //strcpy(rq.password,password.toStdString().c_str());
    strcpy(rq.password,getMD5(password).c_str());
    std::string strName=name.toStdString();
    strcpy(rq.name,strName.c_str());
    sendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_loginCommit(QString tel, QString password)
{
    STRU_LOGIN_RQ rq;
    strcpy(rq.tel,tel.toStdString().c_str());
    //strcpy(rq.password,password.toStdString().c_str());
    strcpy(rq.password,getMD5(password).c_str());
    sendData((char*)&rq,sizeof(rq));
}
#include<QFileInfo>
#include<QDateTime>
void CKernel::SendData(char * buf,int len)
{
    m_tcpClient->SendData(0,buf,len);
}
void CKernel::slot_uploadFile(QString path, QString dir)
{
    QFileInfo qFileInfo(path);
    //文件存储信息
    FileInfo info;
    info.absolutePath=path;
    info.dir=dir;
    info.md5=QString::fromStdString(getFileMD5(path));
    info.name=qFileInfo.fileName();
    info.size=qFileInfo.size();
    info.time=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    info.type="file";

    char buf[1000]="";
    Utf8ToGB2312(buf,1000,path);
    info.pFile=fopen(buf,"rb");

    if(!info.pFile){
        qDebug()<<"file open fail";
        return ;
    }

    int timestamp=QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    while (m_mapTimeStampToFileInfo.count( timestamp ) > 0 )
    {
        timestamp++;
    }
    info.timestamp=timestamp;
    qDebug()<<"timestamp:"<<timestamp;
    //存储到map里面 key 时间戳 value 文件信息
    m_mapTimeStampToFileInfo[timestamp]=info;
    //发上传文件请求

    STRU_UPLOAD_FILE_RQ rq;
    //兼容中文
    std::string strDir=dir.toStdString();
    strcpy(rq.dir,strDir.c_str());

    std::string strName=info.name.toStdString();
    strcpy(rq.fileName,strName.c_str());

    strcpy(rq.fileType, "file");
    strcpy(rq.md5,info.md5.toStdString().c_str());
    rq.size=info.size;
    strcpy(rq.time,info.time.toStdString().c_str());
    rq.timestamp = timestamp;
    rq.userid =m_id;
    SendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_uploadFolder(QString path, QString dir)
{
    qDebug() << __func__;
    QFileInfo info( path ); // 用于获取文件名字
    QDir dr(path);
    //当前文件夹的处理 addFolder c: /项目    下面 是有 /0314/    /0527/    1.txt  上传到/05/
    //qDebug() << "folder:" << info.fileName() << " dir:" << dir;
    slot_addFolder( info.fileName(), dir );
    //获取文件夹下面一层 所有文件的路径(文件信息)
    QFileInfoList lst = dr.entryInfoList(); //获取路径下所有文件的文件信息列表
    // 遍历所有文件
    QString newDir = dir + info.fileName() + "/";
    for( int i = 0 ; i < lst.size() ; ++i )
    {
        QFileInfo file = lst.at( i );
        // 如果是 . 继续
        if( file.fileName() == "." ) continue;
        // 如果是 .. 继续
        if( file.fileName() == ".." ) continue;
        // 如果是文件 uploadFile -> 路径 文件信息的绝对路径 传到什么目录 /05/项目
        if( file.isFile() ){
        qDebug() << "file:" << file.absoluteFilePath() << " dir:" << newDir;
         slot_uploadFile( file.absoluteFilePath(), newDir); // 1.txt -> /05/
        }
        // 如果是文件夹 slot_uploadFolder 递归
        if( file.isDir() )
            slot_uploadFolder( file.absoluteFilePath(), newDir );
    }
}

void CKernel::slot_getCurDirFileList()
{
    // 向服务器发送获取当前目录文件列表
        STRU_GET_FILE_INFO_RQ rq;          // 创建获取文件信息请求结构体
        rq.userid = m_id;                  // 设置用户ID

        // 兼容中文
        std::string strDir = m_curDir.toStdString();  // 将QString转换为std::string
        strcpy(rq.dir, strDir.c_str());    // 复制目录路径到请求结构体

        SendData((char*)&rq, sizeof(rq));  // 发送数据
}

void CKernel::slot_downloadFile(int fileid, QString dir)
{
    //写请求
    STRU_DOWNLOAD_FILE_RQ rq;
    //兼容中文
    std::string strDir = dir.toStdString();
    strcpy( rq.dir , strDir.c_str() );    // Call to function 'str
    rq.fileid = fileid;
    int timestamp =QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    // bug修复 反复检测时间戳是否存在
    while ( m_mapTimeStampToFileInfo.count( timestamp ) > 0 )
    {
        timestamp++;
    }

    rq.timestamp = timestamp;
    rq.userid = m_id;

    SendData( ( char* )&rq , sizeof(rq) );
}

void CKernel::slot_downloadFolder(int fileid, QString dir)
{
    STRU_DOWNLOAD_FOLDER_RQ rq;
    string strDir = dir.toStdString();
    strcpy( rq.dir , strDir.c_str() );

    rq.fileid = fileid;
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    while( m_mapTimeStampToFileInfo.count( timestamp ) > 0 ){
        timestamp ++;
    }
    rq.timestamp = timestamp;
    rq.userid = m_id;

    SendData( (char*)&rq , sizeof(rq));
}

void CKernel::slot_addFolder(QString name, QString dir)
{
    // 发请求包
    STRU_ADD_FOLDER_RQ rq;

    string strDir = dir.toStdString();
    strcpy( rq.dir, strDir.c_str() );

    string strName = name.toStdString();
    strcpy(rq.fileName, strName.c_str());

    string strTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
    strcpy( rq.time, strTime.c_str() );

    rq.timestamp =
    QDateTime::currentDateTime().toString("hhmmsszzz").toInt();

    rq.userid = m_id;

    SendData( (char*)&rq, sizeof(rq) );
}

void CKernel::slot_changeDir(QString dir)
{
    //更新当前的目录
    m_curDir = dir ;
    //刷新列表
    m_mainDialog->slot_deleteAllFileInfo();
    slot_getCurDirFileList();
}

void CKernel::slot_shareFile(QVector<int> fieldArray, QString dir)
{
    //打包
    int packlen = sizeof(STRU_SHARE_FILE_RQ) +
    sizeof(int)*fieldArray.size();

    STRU_SHARE_FILE_RQ * rq = (STRU_SHARE_FILE_RQ *)malloc(packlen);
    rq->init();
    rq->itemCount = fieldArray.size();
    for(int i = 0; i < fieldArray.size(); ++i){
        rq->fileidArray[i] = fieldArray[i];
    }
    rq->userid = m_id;
    std::string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    strcpy(rq->shareTime, time.toStdString().c_str());

    SendData((char*)rq, packlen );
    free(rq);
}
#include<QDebug>
void CKernel::slot_dealClientData(unsigned int lSendIP, char *buf, int nlen)
{
    //QString str = QString("来自服务端的%1").arg(QString::fromStdString(buf));
    //QMessageBox::about(NULL,"提示",str);
    int type=*(int*)buf;
    qDebug()<<__func__;
    //switch(type)
    //{
      //  case _DEF_PACK_LOGIN_RS:
       // slot_dealLoginRs(lSendIP,buf,nlen);
       // break;
    //}

    //通过协议头 拿到处理函数 并执行
    if( type >= _DEF_PACK_BASE && type < _DEF_PACK_BASE + _DEF_PACK_COUNT ){
        PFUN pf = NetMap( type );
        if( pf ){
            (this->*pf) ( lSendIP , buf , nlen );
        }
    }
    //回收空间
        delete[] buf;
}
void CKernel::slot_dealLoginRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_LOGIN_RS * rs=(STRU_LOGIN_RS*)buf;
    switch(rs->result)
    {
    case tel_not_exist:
        QMessageBox::about( m_loginDialog , "提示", "手机号不在，登录失败");
        break;
    case password_error:
        QMessageBox::about( m_loginDialog , "提示", "密码错误，登录失败");
        break;
    case login_success:
        //前台
        m_loginDialog->hide();
        m_mainDialog->show();
        //后台
        m_name=rs->name;
        m_id=rs->userid;
        m_mainDialog->slot_setInfo(m_name);
        //获取根目录下文件列表
        m_curDir="/";
        slot_getCurDirFileList();

        slot_getMyShare();
        InitDatabase(m_id);
        break;
    }
}

void CKernel::slot_dealRegisterRs(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    qDebug()<<__func__;
       STRU_REGISTER_RS* rs = (STRU_REGISTER_RS*)buf;
       //根据不同的结果 有不同的提示
       switch( rs->result ) {
       case tel_is_exist:
           QMessageBox::about( m_loginDialog , "提示", "手机号已存在，注册失败");
            break;
       case name_is_exist:
           QMessageBox::about( m_loginDialog , "提示", "昵称已存在，注册失败");
            break;
       case register_success:
           QMessageBox::about( m_loginDialog , "提示", "注册成功" );
            break;
       }
}

void CKernel::slot_dealUploadFileRs(unsigned int lSendIP, char *buf, int nlen)
{
    // 拆包
        STRU_UPLOAD_FILE_RS* rs=(STRU_UPLOAD_FILE_RS*)buf;

        //首先看结果是否为真
        if(!rs->result){
            qDebug()<<"上传失败";
            return ;
        }
        //为真
            //获取文件信息
            if(m_mapTimeStampToFileInfo.count(rs->timestamp)==0){
                qDebug()<<"not found";
                return ;
            }
            FileInfo& info=m_mapTimeStampToFileInfo[rs->timestamp];
            //更新fileid

            info.fileid=rs->fileid;
            //插入上传信息到“上传中”的控件里 todo
            slot_writeUploadTask(info);
            m_mainDialog->slot_insertUploadFile(info);

            //发送文件块（内容）请求
            STRU_FILE_CONTENT_RQ rq;
            rq.userid=m_id;
            rq.fileid=rs->fileid;
            rq.timestamp=rs->timestamp;
            rq.len=fread(rq.content,1,_DEF_BUFFER,info.pFile);

            SendData((char*)&rq,sizeof(rq));

}

void CKernel::slot_dealFileContentRs(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    STRU_FILE_CONTENT_RS* rs=(STRU_FILE_CONTENT_RS*)buf;
    //找文件信息结构体
    if(m_mapTimeStampToFileInfo.count(rs->timestamp)==0){
        qDebug()<<"file not found";
        return ;
    }
    FileInfo& info=m_mapTimeStampToFileInfo[rs->timestamp];

    //判断是否暂停
    while( info.isPause )
    {
        //sleep();
        QThread::msleep( 100 );
        //为了避免阻塞窗口线程，影响事件循环，加入下面的处理 将信号取出并执行
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
        if(m_quit) return;
    }

    //结果
    if(rs->result==0){
        //假 跳回
        fseek(info.pFile,-1*(rs->len),SEEK_CUR);
    }else{
        //真 pos+len
        info.pos+=rs->len;
        //更新上传进度todo
        Q_EMIT SIG_updateUploadFileProgress(info.timestamp,info.pos);
        //判断是否结束
        if(info.pos>=info.size){
            slot_deleteUploadTask(info);
            //是  关闭文件  回收  返回
            fclose(info.pFile);
            m_mapTimeStampToFileInfo.erase(rs->timestamp);



            m_mainDialog->slot_deleteAllFileInfo();
            slot_getCurDirFileList();
            return ;
        }
    }
    //发文件块
    STRU_FILE_CONTENT_RQ rq;
    rq.userid=m_id;
    rq.fileid=rs->fileid;
    rq.timestamp=rs->timestamp;
    rq.len=fread(rq.content,1,_DEF_BUFFER,info.pFile);

    SendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_dealGetFileInfoRs(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    STRU_GET_FILE_INFO_RS * rs = (STRU_GET_FILE_INFO_RS *)buf;

    if( m_curDir != QString::fromStdString( rs->dir ) ) return;

    //删除原来的
    m_mainDialog->slot_deleteAllFileInfo();
    //获取元素
    int count = rs->count;
    for( int i = 0 ; i < count ; ++i ){
        FileInfo info;

        info.fileid = rs->fileInfo[i].fileid;
        info.type = QString::fromStdString( rs->fileInfo[i].fileType );
        info.name = QString::fromStdString( rs->fileInfo[i].name);
        info.size = rs->fileInfo[i].size;
        info.time = rs->fileInfo[i].time;

        //插入到控件
        m_mainDialog->slot_insertFileInfo( info );
    }
}

void CKernel::slot_dealFileHeaderRq(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    STRU_FILE_HEADER_RQ * rq = (STRU_FILE_HEADER_RQ *)buf;
    //rq->dir;
    //rq->fileid;
    //rq->fileName;
    //rq->fileType;
    //rq->md5;
    //rq->size;
    //rq->timestamp;
    //创建文件信息结构体 赋值
    FileInfo info;
    QString tmpDir = QString::fromStdString( rq->dir ); // /NetDisk/111
    QStringList dirList = tmpDir.split( "/" ); // 分割函数 NetDisk 111

    QString pathsum = m_sysPath;
    for( QString & node : dirList )
    {
        if( !node.isEmpty() ) {
        pathsum += "/";
        pathsum += node;
        }
        QDir dir;
        if( !dir.exists( pathsum )) {
        dir.mkdir( pathsum );
        }
    }
    info.dir = QString::fromStdString( rq->dir );
    info.name = QString::fromStdString( rq->fileName );
    info.absolutePath= QString( "%1%2%3" ).arg(m_sysPath).arg(info.dir).arg( info.name );
    info.fileid = rq->fileid;
    info.md5 = QString::fromStdString( rq->md5 );


    info.size = rq->size;
    info.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    info.timestamp = rq->timestamp;
    info.type = "file";
    //打开文件
    char pathbuf[1000] = "";
    Utf8ToGB2312( pathbuf, 1000 , info.absolutePath );
    info.pFile = fopen( pathbuf , "wb" );
    if( !info.pFile ){
        qDebug() << "file opene fail";
        return;
    }

    //todo 保存下载信息到控件
    slot_writeDownloadTask(info);
    m_mainDialog->slot_insertDownloadFile( info );
    // 保存map里面
    m_mapTimeStampToFileInfo[rq->timestamp] = info;
    // 写回复
    STRU_FILE_HEADER_RS rs;
    rs.fileid = rq->fileid;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = m_id;

    SendData((char*)&rs, sizeof(rs));
}

void CKernel::slot_dealFileContentRq(unsigned int lSendIP, char *buf, int nlen)
{
    // 拆包
    STRU_FILE_CONTENT_RQ * rq = (STRU_FILE_CONTENT_RQ *)buf;

    // 拿到文件信息结构
    if( m_mapTimeStampToFileInfo.count( rq->timestamp ) == 0 ) return;
    FileInfo & info = m_mapTimeStampToFileInfo[ rq->timestamp ];

    //判断是否暂停
    while( info.isPause )
    {
        //sleep();
        QThread::msleep( 100 );
        //为了避免阻塞窗口线程，影响事件循环，加入下面的处理 将信号取出并执行
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
        if(m_quit) return;
    }




    STRU_FILE_CONTENT_RS rs;
    //写文件
    int len = fwrite( rq->content , 1 , rq->len , info.pFile );
    if( len != rq->len ) {
        //不成功 跳回去
        rs.result = 0;
        fseek( info.pFile , -1*len , SEEK_CUR );
    } else{
        //成功 pos+=len
        rs.result = 1;
        info.pos += len;
        //更新进度 todo
        Q_EMIT SIG_updateDownloadFileProgress( rq->timestamp , info.pos);
        //要看 有没有到末尾 是否结束
        if( info.pos >= info.size ) {
            slot_deleteDownloadTask(info);
            //结束 关闭文件 回收
            fclose( info.pFile );
            m_mapTimeStampToFileInfo.erase( rq->timestamp );
        }
    }
    //写回复
    rs.fileid = rq->fileid;
    rs.len = rq->len;
    rs.timestamp = rq->timestamp ;
    rs.userid = m_id;

    //发送
    SendData( (char*)&rs ,sizeof(rs) );
}

void CKernel::slot_dealAddFolderRs(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    STRU_ADD_FOLDER_RS * rs = (STRU_ADD_FOLDER_RS *)buf;

    //判断是否成功
    if( rs->result != 1 ) return;

    //更新文件列表
    slot_getCurDirFileList();
}

void CKernel::slot_dealQuickUploadRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_QUICK_UPLOAD_RS* rs = (STRU_QUICK_UPLOAD_RS*)buf;

    //获取文件信息
    if( m_mapTimeStampToFileInfo.count( rs->timestamp ) == 0 ) return;
    FileInfo & info = m_mapTimeStampToFileInfo[rs->timestamp ];

    //关闭文件
    if( info.pFile )
        fclose( info.pFile );

    //写入上传已完成信息
    m_mainDialog->slot_insertUploadComplete( info );
    //发送刷新文件列表
    if(m_curDir==info.dir)
    {
        m_mainDialog->slot_deleteAllFileInfo();
        slot_getCurDirFileList();

    }
        //删除节点
    m_mapTimeStampToFileInfo.erase( rs->timestamp );
}

void CKernel::slot_dealShareFileRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_SHARE_FILE_RS * rs = (STRU_SHARE_FILE_RS *)buf;

    //判断是否成功
    if( rs->result != 1 ) return;

    //刷新 发获取请求
    slot_getMyShare();
}

void CKernel::slot_dealMyShareRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_MY_SHARE_RS * rs = (STRU_MY_SHARE_RS *)buf;
    int count = rs->itemCount;

    // 遍历 分享文件的信息 添加到控件上面
    m_mainDialog->slot_deleteAllShareInfo();
    for( int i = 0 ; i < count ; ++i ){
        m_mainDialog->slot_insertShareFileInfo( rs->items[i].name, rs->items[i].size , rs->items[i].time , rs->items[i].shareLink );
    }
}

void CKernel::slot_dealGetShareRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_GET_SHARE_RS * rs = (STRU_GET_SHARE_RS *)buf;

    //根据结果
    if( rs->result == 0 ) {
        //错误返回提示
        QMessageBox::about( this->m_mainDialog, "提示", "获取分享失败");
    }else{
        //正确刷新
        if( QString::fromStdString( rs->dir ) == m_curDir ) {
        slot_getCurDirFileList();
        }
    }
}

void CKernel::slot_dealFolderHeadRq(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_FOLDER_HEADER_RQ *rq = (STRU_FOLDER_HEADER_RQ *)buf;

    //rq->dir;
    //rq->fileName;

    //创建目录
    //dir 可能有很多层 需要循环创建目录
    QString tmpDir = QString::fromStdString( rq->dir ); // /NetDisk/111 I 1.txt
    QStringList dirList = tmpDir.split( "/" ); // 分割函数 NetDisk 111

    QString pathsum = m_sysPath;
    for( QString & node : dirList )
    {
        if( !node.isEmpty() ) {
            pathsum += "/";
            pathsum += node;

            QDir dir;
            if( !dir.exists( pathsum )){
            dir.mkdir( pathsum );
            }
        }
    }
    pathsum += "/";
    pathsum += QString::fromStdString( rq->fileName );

    QDir dir;
    if( !dir.exists( pathsum )){
        dir.mkdir( pathsum );
    }
}

void CKernel::slot_dealDeleteFileRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_DELETE_FILE_RS * rs = (STRU_DELETE_FILE_RS * )buf;

    //看是否刷新
    if( rs->result == 1){
        if( QString::fromStdString( rs->dir ) == m_curDir ) {
            m_mainDialog->slot_deleteAllFileInfo();
            slot_getCurDirFileList();
        }
    }
}

void CKernel::slot_dealContinueUploadRs(unsigned int lSendIP, char *buf, int nLen)
{
    //拆包
    STRU_CONTINUE_UPLOAD_RS* rs = (STRU_CONTINUE_UPLOAD_RS*)buf;

    //通过map拿到文件信息
    if( m_mapTimeStampToFileInfo.count( rs->timestamp ) == 0 ) return;
    FileInfo & info = m_mapTimeStampToFileInfo[rs->timestamp];
    //文件位置跳转 pos 更新 界面显示 百分比更新
    info.pos = rs->pos;
    fseek( info.pFile, rs->pos, SEEK_SET ); //从起始位置跳 pos那么多

    m_mainDialog->slot_updateUploadFileProgress( info.timestamp , info.pos );
    //发送文件块请求

    STRU_FILE_CONTENT_RQ rq;
    // 读文件
    int len = fread(rq.content, 1, _DEF_BUFFER, info.pFile );
    rq.len = len;

    rq.fileid = info.fileid;
    rq.timestamp = info.timestamp;
    rq.userid = m_id;

    SendData((char*)&rq, sizeof(rq) );
}
void CKernel::slot_getMyShare()
{
    STRU_MY_SHARE_RQ rq;
    rq.userid = m_id;

    SendData( (char*)&rq , sizeof(rq) );
}

void CKernel::slot_getShareByLink(int code, QString dir)
{
    // 发请求
    STRU_GET_SHARE_RQ rq;
    string tmpDir = dir.toStdString();
    strcpy( rq.dir , tmpDir.c_str() );
    rq.shareLink = code;
    string time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
    strcpy( rq.time , time.c_str() );
    rq.userid = m_id;

    SendData( (char*)&rq , sizeof(rq) );
}

void CKernel::slot_deleteFile(QVector<int> fileidArray, QString dir)
{
    // 发请求
    int packlen = sizeof(STRU_DELETE_FILE_RQ) + fileidArray.size() * sizeof(int);

    STRU_DELETE_FILE_RQ *rq = (STRU_DELETE_FILE_RQ *)malloc(packlen);
    rq->init();
    string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    rq->fileCount = fileidArray.size();

    rq->userid = m_id;
    for(int i = 0; i < rq->fileCount; ++i){
        rq->fileidArray[i] = fileidArray[i];
    }

    SendData((char*)rq, packlen);

    free(rq);
}

void CKernel::slot_setUploadPause(int timestamp, int isPause)
{
    // isPause 1 从正在上传变为暂停   isPause 0 从暂停 变为开始继续上传
    // 需要找到文件信息结构体
    // map里面有 程序未退的情况   直接置位
    // map中没有 证明程序退出的    断点续传 需要走协议
    if( m_mapTimeStampToFileInfo.count( timestamp ) > 0 ){
        m_mapTimeStampToFileInfo[timestamp].isPause = isPause;
    }else{
        //断点续传 todo
        //断点续传 todo
        //创建fileinfo 然后打开文件 放到map里面
        FileInfo info ;
        bool res = m_mainDialog->slot_getUploadFileInfoByTimestamp(timestamp,info);
        if(!res)
        {
            return;
        }
        //转化 路径转成ASCII
        char pathbuf[1000] = "";
        Utf8ToGB2312( pathbuf, 1000 , info.absolutePath );
        //打开文件 二进制只读
        info.pFile = fopen( pathbuf, "rb");
        //打开文件 如果文件无法打开，就退出
        if( !info.pFile ){
            qDebug() << "打开失败" << info.absolutePath; return;
        }
        info.isPause = 0 ; // 避免开始 就停在循环那里

        m_mapTimeStampToFileInfo[ timestamp ] = info;

        // 发送上传续传请求
        STRU_CONTINUE_UPLOAD_RQ rq;
        string strDir = info.dir.toStdString();
        strcpy( rq.dir , strDir.c_str() );
        rq.fileid = info.fileid;
        rq.timestamp = timestamp;
        rq.userid = m_id;

        SendData( (char*)&rq , sizeof(rq) );
    }
}

void CKernel::slot_setDownloadPause(int timestamp, int isPause)
{
    // isPause 1 从正在上传变为暂停   isPause 0 从暂停 变为开始继续上传
    // 需要找到文件信息结构体
    // map里面有 程序未退的情况   直接置位
    // map中没有 证明程序退出的    断点续传 需要走协议
    if( m_mapTimeStampToFileInfo.count( timestamp ) > 0 ){
        m_mapTimeStampToFileInfo[timestamp].isPause = isPause;
    }else{
        //断点续传 todo
        if( isPause == 0 ){
            //断点续传
            //1.创建信息结构体 装到map里面
            //信息 在哪里？ 可以直接从空间里面取出
            FileInfo info ;
            bool res = m_mainDialog->slot_getDownloadFileInfoByTimestamp( timestamp,info );
            if(!res)
            {
                return;
            }
            //转化 路径转成ASCII
            char pathbuf[1000] = "";
            Utf8ToGB2312( pathbuf , 1000 , info.absolutePath );
            //打开文件 二进制追加 不能是 w 因为 会清空
            info.pFile = fopen( pathbuf , "ab");

            if( !info.pFile ){
                qDebug() << "打开失败" << info.absolutePath; return;
            }
            info.isPause = 0 ; // 避免开始 就停在循环那里

            m_mapTimeStampToFileInfo[ timestamp ] = info;
            //2.发协议 告诉服务器 文件下载到哪里了，然后服务器跳转到哪里，从那开始继续读，然后
            //文件块发送
            //服务器接收 有两种可能 1. 文件信息还在（客户端出现异常很快好了，没有超过预订删除客户端所有信息的时间） 2. 不在（超过了时间）
            STRU_CONTINUE_DOWNLOAD_RQ rq;
            rq.fileid = info.fileid;
            string dirstr = info.dir.toStdString();
            strcpy( rq.dir , dirstr.c_str() );
            rq.pos = info.pos;
            rq.timestamp = info.timestamp;
            rq.userid = m_id;
            SendData( (char*)&rq , sizeof(rq) );
            }
        }
}

#include<QDir>
#include<QDebug>
void CKernel::InitDatabase(int id)
{
        m_sql = new CSqlite;
    // 首先 找到 exe  去同级目录  /database/id.db
    QString path = QCoreApplication::applicationDirPath() + "/database/";
    // 先看路径是否存在 要不要创建
    QDir dir;
    if(!dir.exists( path )){
        dir.mkdir( path );
    }
    path = path + QString("%1.db").arg( id );
    // 查看有没有这个文件
    QFileInfo info( path );
    if( info.exists() )
    {
        //有直接加载
        //连接
        m_sql->ConnectSql( path );

        //测试 读取数据
        //QString sqlbuf = "select count(*) from t_upload;";
        //QStringList lst;
        //m_sql->SelectSql( sqlbuf, 1, lst );
        //qDebug() << "upload item count:" << lst.front();
        //lst.clear();
        //sqlbuf = "select count(*) from t_download;";
        //m_sql->SelectSql( sqlbuf, 1, lst );
        //qDebug() << "download item count:" << lst.front();
        //lst.clear();

        QList<FileInfo> uploadTaskList;
        QList<FileInfo> downloadTaskList;

        slot_getUploadTask( uploadTaskList );
        slot_getDownloadTask( downloadTaskList );

        //加载上传任务
        for( FileInfo & info : uploadTaskList ) {
            // 如果这个文件没有了不能继续
            QFileInfo fi( info.absolutePath );
            if( !fi.exists() ) continue;

            // 修改任务的初始状态
            info.isPause = 1;
            m_mainDialog->slot_insertUploadFile( info );
            //上传续传 控件 上看不到进行到多少
            //todo 获取当前位置

            //同步控件的位
        }
        //加载下载任务
        for( FileInfo & info : downloadTaskList )
        {
            //如果这个文件没有了不能继续
            QFileInfo fi( info.absolutePath );
            if( !fi.exists() ) continue;

            //修改任务的初始状态
            info.isPause = 1;
            //进行到多少 可以知道
            info.pos = fi.size();

            m_mainDialog->slot_insertDownloadFile( info );

            //控件同步位置
            m_mainDialog->slot_updateDownloadFileProgress( info.timestamp, fi.size() );
        }

    }else{
        //没有 创建表
        QFile file( path );
        if( !file.open( QIODevice::WriteOnly ) ) return;
        file.close();
        //连接
        m_sql->ConnectSql( path );
        //创建表
        QString sqlbuf = "create table t_upload(timestamp int ,f_id int , f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260));";
        m_sql->UpdateSql(sqlbuf );

        sqlbuf = "create table t_download(timestamp int ,f_id int ,f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260));";
        m_sql->UpdateSql(sqlbuf );
    }
}

void CKernel::slot_writeUploadTask(FileInfo &info)
{
    QString sqlbuf = QString("insert into t_upload values (%1, %2, '%3', '%4', '%5', %6, '%7', '%8', '%9');")
        .arg(info.timestamp)
        .arg(info.fileid)
        .arg(info.name)
        .arg(info.dir)
        .arg(info.time)
        .arg(info.size)
        .arg(info.md5)
        .arg(info.type)
        .arg(info.absolutePath);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_writeDownloadTask(FileInfo &info)
{
    QString sqlbuf = QString("insert into t_download values(%1, %2, '%3', '%4', '%5', %6, '%7', '%8', '%9');"
    ).arg(info.timestamp).arg(info.fileid).arg(info.name).arg(info.dir).arg(info.time).arg(info.size).arg(info.md5).arg(info.type).arg(info.absolutePath);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_deleteUploadTask(FileInfo &info)
{
    QString sqlbuf = QString("delete from t_upload where timestamp = %1;").arg(info.timestamp);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_deleteDownloadTask(FileInfo &info)
{
    QString sqlbuf = QString("delete from t_download where timestamp = %1 and f_absolutePath = '%2';").arg(info.timestamp).arg(info.absolutePath);
    m_sql->UpdateSql( sqlbuf );
}

void CKernel::slot_getUploadTask(QList<FileInfo> &infoList)
{
    //获取所有的任务
    QString sqlbuf = "select * from t_upload;";
    QStringList lst;
    m_sql->SelectSql( sqlbuf , 9 , lst );
    /*timestamp int ,f_id int ,f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260)*/
    while(lst.size() != 0){
        FileInfo info;
        info.timestamp = QString(lst.front()).toInt();   lst.pop_front();
        info.fileid = QString(lst.front()).toInt();   lst.pop_front();
        info.name = lst.front();   lst.pop_front();
        info.dir = lst.front();   lst.pop_front();
        info.time = lst.front();   lst.pop_front();
        info.size = QString(lst.front()).toInt();   lst.pop_front();
        info.md5 = lst.front();   lst.pop_front();
        info.type = lst.front();   lst.pop_front();
        info.absolutePath = lst.front();   lst.pop_front();

        infoList.push_back( info );
    }
}

void CKernel::slot_getDownloadTask(QList<FileInfo> &infoList)
{
    //获取所有的任务
    QString sqlbuf = "select * from t_download;";
    QStringList lst;
    m_sql->SelectSql( sqlbuf , 9 , lst );
    /*timestamp int ,f_id int ,f_name varchar(260),f_dir varchar(260),f_time varchar(60),f_size int,f_md5 varchar(60),f_type varchar(60),f_absolutePath varchar(260)*/
    while(lst.size() != 0){
        FileInfo info;
        info.timestamp = QString(lst.front()).toInt();   lst.pop_front();
        info.fileid = QString(lst.front()).toInt();   lst.pop_front();
        info.name = lst.front();   lst.pop_front();
        info.dir = lst.front();   lst.pop_front();
        info.time = lst.front();   lst.pop_front();
        info.size = QString(lst.front()).toInt();   lst.pop_front();
        info.md5 = lst.front();   lst.pop_front();
        info.type = lst.front();   lst.pop_front();
        info.absolutePath = lst.front();   lst.pop_front();

        infoList.push_back( info );
    }
}

#ifdef USE_SERVER
void CKernel::slot_dealServerData(unsigned int lSendIP, char *buf, int nlen)
{
    QString str = QString("来自客户端的%1").arg(QString::fromStdString(buf));
    QMessageBox::about(NULL,"提示",str);//about 阻塞的 模态窗口
    m_tcpServer->SendData(lSendIP,buf,nlen);
    delete[] buf;
}
#endif
void CKernel::slot_playOnline(int fileid, QString dir)
{
    qDebug()<<__func__;
    //发送获取HLS播放地址的请求
    STRU_GET_HLS_URL_RQ rq;

    rq.userid = m_id;
    rq.fileid = fileid;
    string strDir = dir.toStdString();
    strcpy(rq.dir, strDir.c_str());
    SendData((char*)&rq, sizeof(rq));
}

void CKernel::slot_dealGetHlsUrlRs(unsigned int lSendIP, char *buf, int nlen)
{
    qDebug()<<__func__;
    //拆包
    STRU_GET_HLS_URL_RS *rs = (STRU_GET_HLS_URL_RS *)buf;

    qDebug()<<"GetHlsUrlRs: result="<<rs->result<<", status="<<rs->status;

    if(rs->result == 0) {
        //失败
        QMessageBox::about(m_mainDialog, "提示", "获取播放地址失败");
        return;
    }

    if(rs->status == transcode_processing) {
        //正在转码
        QMessageBox::about(m_mainDialog, "提示", "视频正在转码中，请稍后重试");
        return;
    }

    if(rs->status == transcode_done) {
        //转码完成，可以播放
        //创建播放器对话框
        if(!m_playerDialog) {
            m_playerDialog = new PlayerDialog;
        }

        //构造播放URL，例如 http://192.168.23.132:8080/hls/md5/output.m3u8
        QString url = QString("http://%1:8080%2").arg(m_ip).arg(QString::fromStdString(rs->url));

        qDebug()<<"播放URL:"<<url;

        //显示播放器并播放
        m_playerDialog->show();
        m_playerDialog->setVideoUrl(url);
    } else {
        //其他状态，提示用户
        QMessageBox::about(m_mainDialog, "提示", "视频暂不可播放，请稍后重试");
    }
}

//配置文件  位置：exe同级目录
//思路：根据目录 看文件是否存在，存在加载，不存在就创建并且写入默认值
//.ini
//[组名]
//key=value
//
//eg:
//[net]
//ip=192.168.5.198(例子）
//port=8004
