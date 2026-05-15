#ifndef MAINDIALOG_H
#define MAINDIALOG_H
#include<QMenu>
#include <QDialog>
#include<QCloseEvent>
#include"common.h"
#include"mytablewidgetitem.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainDialog; }
QT_END_NAMESPACE
class CKernel;
class MainDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_close();
    void SIG_uploadFile(QString path,QString dir);
    //上传什么路径的文件夹，到什么目录下面
    void SIG_uploadFolder( QString path, QString dir);
    //什么文件id，什么目录下的文件 下载
    void SIG_downloadFile( int fileid , QString dir);
    //什么文件id，什么目录下的文件夹 下载
    void SIG_downloadFolder( int fileid , QString dir);
    void SIG_addFolder( QString name , QString dir);
    //改变路径
    void SIG_changeDir( QString dir);
    // 分享  什么目录下面的文件列表
    void SIG_shareFile( QVector<int> fieldArray , QString dir );
    //什么目录下面 添加什么分享码的文件
    void SIG_getShareByLink( int code , QString dir);
    //删除什么目录下的一系列文件（文件id 数组）
    void SIG_deleteFile( QVector<int> fileidArray , QString dir);
    //设置上传暂停 0 开始 1 暂停
    void SIG_setUploadPause( int timestamp , int isPause );
    //设置下载暂停 0 开始 1 暂停
    void SIG_setDownloadPause( int timestamp , int isPause );
    //在线播放
    void SIG_playOnline( int fileid , QString dir );
public:
    MainDialog(QWidget *parent = nullptr);
    ~MainDialog();

    void closeEvent(QCloseEvent * event);
private slots:

    void slot_setInfo(QString name);
    void on_pb_file_clicked();

    void on_pb_transmit_clicked();

    void on_pb_share_clicked();

    void on_pb_addfile_clicked();

    void slot_addFolder(bool flag);
    void slot_uploadFile(bool flag);
    void slot_uploadFolder(bool flag);
    void slot_downlodeFile(bool flag);
    void slot_shareFile(bool flag);
    void slot_deleteFile(bool flag);
    void slot_getShare(bool flag);

    void slot_uploadPause(bool flag);

    void slot_uploadResume( bool flag );

    void slot_downloadPause( bool flag );

    void slot_downloadResume( bool flag );

    void slot_insertUploadFile(FileInfo & info);
    void slot_insertDownloadFile(FileInfo & info);
    void slot_insertUploadComplete(FileInfo & info);
    void slot_insertShareFileInfo( QString name, int size , QString time ,int shareLink );
    void slot_updateUploadFileProgress(int timestamp,int pos);
    void slot_updateDownloadFileProgress(int timestamp,int pos);
    void slot_deleteUploadFileByRow(int row);
    void slot_deleteDownloadFileByRow(int row);


    void slot_insertFileInfo(FileInfo& info);
    void slot_insertDownloadComplete(FileInfo& info);
    void on_table_file_cellClicked(int row, int column);

    void on_table_file_customContextMenuRequested(const QPoint &pos);

    void slot_openPath(bool flag);
    void slot_deleteAllFileInfo();
    void slot_deleteAllShareInfo();
    void on_table_file_cellDoubleClicked(int row, int column);

    void on_pb_prev_clicked();

    void on_table_upload_cellClicked(int row, int column);

    void on_table_download_cellClicked(int row, int column);
    bool slot_getDownloadFileInfoByTimestamp(int timestamp,FileInfo &info);
    bool slot_getUploadFileInfoByTimestamp(int timestamp,FileInfo &info);
    void slot_playOnline(bool flag);
private:
    Ui::MainDialog *ui;
    QMenu m_menuAddFile;
    QMenu m_menuFileInfo;
    QMenu m_menuUpload;
    QMenu m_menuDownload;
    friend class CKernel;
};
#endif // MAINDIALOG_H
//点击x-》执行关闭事件 发送关闭信号 核心类接收 回收资源
