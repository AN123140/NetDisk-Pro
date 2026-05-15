#ifndef COMMON_H
#define COMMON_H

#include<QString>
////////////////////文件信息/////////////////
struct FileInfo
{

    FileInfo():fileid(0) , size(0),pFile( nullptr )
      , pos(0) , isPause(0) , timestamp(0){

    }

    int fileid;
    QString name;
    QString dir;
    QString time;
    int size;
    QString md5;
    QString type;
    QString absolutePath;

    int pos; //上传或下载到什么位置
    int timestamp;
    int isPause; //暂停  0 1

    //文件指针
    FILE* pFile;
    static QString getSize(int size)
    {
        QString res;
        int count=0;
        int tmp=size;
        while(tmp!=0)
        {
            tmp/=1024;
            if(tmp!=0)count++;
        }
        switch(count) {
            case 0: // B -> KB (小于1KB的情况)
                res = QString("0.%1KB").arg((int)(size % 1024 / 1024.0 * 100), 2, 10, QChar('0')); // 0.0xKB
                // arg() 参数：第二个-宽度，第三个-进制，第四个-不够宽度时的填充字符
                if( size != 0 && res == "0.00KB" )
                    res = "0.01KB";
                break;
            case 1: // KB
                res = QString("%1.%2KB").arg(size / 1024)
                      .arg((int)(size % 1024 / 1024.0 * 100), 2, 10, QChar('0'));
                break;
            case 2:
            case 3: // MB
                res = QString("%1.%2MB").arg(size / 1024 / 1024)
                      .arg((int)(size / 1024 % 1024 / 1024.0 * 100), 2, 10, QChar('0'));
                break;
            default: // GB及以上
                break;
        }
        return res;
    }

};







#endif // COMMON_H
