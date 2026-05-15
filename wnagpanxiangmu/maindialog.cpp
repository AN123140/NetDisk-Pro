#include "maindialog.h"
#include "ui_maindialog.h"
#include<QMessageBox>
#include<QDebug>
MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    ui->sw_page->setCurrentIndex(0);
    ui->tw_transmit->setCurrentIndex(2);
    //设置标题栏
    this->setWindowTitle("嘎啦game");
    this->setWindowFlags(Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint);

    //定义菜单项
    QAction * action_addFolder=new QAction(QIcon(":/images/folder.png"),"新建文件夹");
    QAction *action_uploadFile = new QAction("上传文件");
    QAction *action_uploadFolder = new QAction("上传文件夹");
    m_menuAddFile.addAction(action_addFolder);
    m_menuAddFile.addSeparator();
    m_menuAddFile.addAction(action_uploadFile);
    m_menuAddFile.addAction(action_uploadFolder);

    connect(action_addFolder, SIGNAL(triggered(bool)), this, SLOT(slot_addFolder(bool)));
    connect(action_uploadFile, SIGNAL(triggered(bool)), this, SLOT(slot_uploadFile(bool)));
    connect(action_uploadFolder, SIGNAL(triggered(bool)), this, SLOT(slot_uploadFolder(bool)));

    QAction *action_downloadFile = new QAction("下载文件");
    QAction *action_shareFile = new QAction("分享文件");
    QAction *action_deleteFile = new QAction("删除文件");
    QAction *action_getShare = new QAction("获取分享"); // 获取分享到本地目录
    QAction *action_playOnline = new QAction("在线播放"); // 在线播放视频

    m_menuFileInfo.addAction( action_addFolder );
    m_menuFileInfo.addSeparator(); //加入分隔符
    m_menuFileInfo.addAction( action_downloadFile );
    m_menuFileInfo.addAction( action_shareFile );
    m_menuFileInfo.addAction( action_deleteFile );
    m_menuFileInfo.addAction( action_playOnline );
    m_menuFileInfo.addAction( "收藏" );
    m_menuFileInfo.addSeparator();//加入分隔符
    m_menuFileInfo.addAction(action_getShare);

    connect( action_downloadFile , SIGNAL( triggered(bool) ) ,
    this , SLOT(slot_downlodeFile(bool)) );
    connect( action_shareFile , SIGNAL(triggered(bool)) , this ,
    SLOT( slot_shareFile(bool)) );
    connect( action_deleteFile , SIGNAL(triggered(bool)) , this ,
    SLOT( slot_deleteFile(bool)) );
    connect( action_getShare , SIGNAL(triggered(bool)) , this ,
             SLOT(slot_getShare(bool)) );
    connect( action_playOnline , SIGNAL(triggered(bool)) , this ,
                 SLOT(slot_playOnline(bool)) );
    // ui->table_download;
    // ui->table_upload;

    // 添加右键显示菜单 lambda表达式 匿名函数
    // [ ] 捕捉列表（） 函数参数列表 { } 函数体

    //connect( ui->table_download , SIGNAL() , this , SLOT() );
    connect( ui->table_download , &QTableWidget::customContextMenuRequested,this ,
        [ this ] ( QPoint ){ this->m_menuDownload.exec( QCursor::pos() ); } );
    connect( ui->table_upload , &QTableWidget::customContextMenuRequested,this ,
        [ this ] ( QPoint ){ this->m_menuUpload.exec( QCursor::pos() ); } );

    // 添加菜单项
    QAction *actionUploadPause = new QAction("暂停");
    QAction *actionUploadResume = new QAction("开始");
    QAction *actionDownloadPause = new QAction("暂停");
    QAction *actionDownloadResume = new QAction("开始");

    m_menuUpload.addAction( actionUploadPause );
    m_menuUpload.addAction( actionUploadResume );
    m_menuUpload.addAction( "全部暂停" );
    m_menuUpload.addAction( "全部开始" );

    m_menuDownload.addAction( actionDownloadPause );
    m_menuDownload.addAction( actionDownloadResume );
    m_menuDownload.addAction( "全部暂停" );
    m_menuDownload.addAction( "全部开始" );

    connect( actionUploadPause , SIGNAL(triggered(bool)) ,
        this , SLOT( slot_uploadPause(bool) ) );
    connect( actionUploadResume , SIGNAL(triggered(bool)) ,
        this , SLOT( slot_uploadResume(bool) ) );
    connect( actionDownloadPause , SIGNAL(triggered(bool)) ,
        this , SLOT( slot_downloadPause(bool) ) );
    connect( actionDownloadResume , SIGNAL(triggered(bool)) ,
        this , SLOT( slot_downloadResume(bool) ) );
}

MainDialog::~MainDialog()
{
    delete ui;
}

void MainDialog::closeEvent(QCloseEvent *event)
{
    if(QMessageBox::question(this,"操你妈逼","你他妈的要退出吗")
            ==QMessageBox::Yes)
    {
        //关闭
        event->accept();
        Q_EMIT SIG_close();
    }else{
        event->ignore();
    }
}

void MainDialog::slot_setInfo(QString name)
{
    ui->pb_name->setText(name);
}


void MainDialog::on_pb_file_clicked()
{
    ui->sw_page->setCurrentIndex(0);
}


void MainDialog::on_pb_transmit_clicked()
{
    ui->sw_page->setCurrentIndex(1);
}


void MainDialog::on_pb_share_clicked()
{
    ui->sw_page->setCurrentIndex(2);
}

//点击添加文件
void MainDialog::on_pb_addfile_clicked()
{
    m_menuAddFile.exec(QCursor::pos());
}
#include<QFileDialog>
#include<QInputDialog>
void MainDialog::slot_addFolder(bool flag)
{
    qDebug()<<__func__;
    bool ok;
    //弹出输入窗口
    QString name = QInputDialog::getText( this, "新建文件夹", "输入名称",QLineEdit::Normal, "",&ok, Qt::MSWindowsFixedSizeDialogHint);

    QString tmp = name;
    //空白符的处理
    if( name.isEmpty() || tmp.remove("").isEmpty() || name.length() > 100 ) {
        QMessageBox::about( this, "提示", "名字非法");
        return;
    }
    //不可以用名字......名字

    //一些非法 \ / : ? * < > | "
    if( name.contains("\\") || name.contains("/") || name.contains(":") ||name.contains("?") || name.contains("*") || name.contains("<")|| name.contains(">") || name.contains("|") || name.contains("\"")){
        QMessageBox::about( this, "提示", "名字非法");
        return;
    }
    //判断是否现在已经存在 todo

    QString dir = ui->lb_path->text();
    Q_EMIT SIG_addFolder( name , dir);
}



void MainDialog::slot_uploadFile(bool flag)
{
    qDebug()<<__func__;
    //弹出窗口 选择文件
    QString path = QFileDialog::getOpenFileName(this, "选择文件", "./");
    if (path.isEmpty()) return;

    //目前上传的有没有一样的文件 如果是取消 todo

    //发送信号 核心类处理 传递的信息: 上传什么文件到什么目录下
    QString dir = ui->lb_path->text();

    Q_EMIT SIG_uploadFile(path,dir);
}

void MainDialog::slot_uploadFolder(bool flag)
{
    qDebug()<<__func__;

    //点击 弹出文件选择对话框 选择路径
    QString path = QFileDialog::getExistingDirectory( this, "选择文件夹", "./");
    //判断非空
    if( path.isEmpty() ) return;
    //过滤 是否正在传 todo

    //发信号 上传什么路径的文件夹，到什么目录下面
    Q_EMIT SIG_uploadFolder( path , ui->lb_path->text() );
}

void MainDialog::slot_downlodeFile(bool flag)
{
    qDebug()<<__func__;
    //遍历列表
    int rows = ui->table_file->rowCount();
    QString dir = ui->lb_path->text(); //获取目录
    for( int i = 0 ; i < rows ; ++i ){
        //看选中的
        Mytablewidgetitem*item0 = (Mytablewidgetitem*)ui->table_file->item(i, 0);
        if( item0 ->checkState() == Qt::Checked ) {
        //列表中有这个下载，不能开始 todo  过滤

            //获取类型
            if( item0 ->m_info.type == "file"){ //发信号  下载文件  下载文件夹
                Q_EMIT SIG_downloadFile( item0 ->m_info.fileid , dir );
            }else{
                Q_EMIT SIG_downloadFolder( item0 ->m_info.fileid , dir );
            }
        }
    }
}

void MainDialog::slot_shareFile(bool flag)
{
    qDebug() << __func__;
    //申请数组
    QVector<int> array;
    int count = ui->table_file->rowCount();
    //遍历所有项
    for( int i = 0 ; i < count ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_file->item( i , 0 );
        //看是否是打钩的
        if( item0->checkState() == Qt::Checked ) {
        //添加到数组里面
        array.push_back( item0->m_info.fileid );
        }
    }
    //发送信号
    Q_EMIT SIG_shareFile( array , ui->lb_path->text() );
}

void MainDialog::slot_deleteFile(bool flag)
{
    qDebug()<<__func__;

    QVector<int> array;
    int count = ui->table_file->rowCount();
    for( int i = 0 ; i < count ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_file->item(
        i, 0 );
        //看看是否打钩的
        if( item0->checkState() == Qt::Checked ) {
        //添加到数组里面
        array.push_back( item0->m_info.fileid );
        }
    }

    //发送信号
    Q_EMIT SIG_deleteFile( array , ui->lb_path->text() );
}

void MainDialog::slot_getShare(bool flag)
{
    qDebug() << __func__;
    //弹窗 输入分享码
    QString txt = QInputDialog::getText( this, "获取分享", "输入分享码");
    //过滤
    int code = txt.toInt();
    if( txt.length() != 9 || code < 100000000 || code >= 1000000000 ) {
        QMessageBox::about( this, "提示", "分享码非法");
        return;
    }
    //发送信号 什么目录下面 添加什么分享码的文件
    Q_EMIT SIG_getShareByLink( code , ui->lb_path->text());
}

void MainDialog::slot_uploadPause(bool flag)
{
    qDebug() << "上传暂停";
    int rows = ui->table_upload->rowCount();
    //遍历表单
    for( int i = 0 ; i < rows ; ++i ){
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_upload->item(i, 0);
        //看是否打钩
        if( item0->checkState() == Qt::Checked ) {
            QPushButton * button = (QPushButton *)ui->table_upload->cellWidget(i, 5);
            //看按钮的状态 切换文字 发送信号
            if( button->text() == "暂停" ) {
                //信号 设置文件信息结构体暂停标志位
                button->setText("开始");
                Q_EMIT SIG_setUploadPause( item0->m_info.timestamp, 1 );
            }
        }
    }
}

void MainDialog::slot_uploadResume(bool flag)
{
    qDebug() << "上传开始";
    int rows = ui->table_upload->rowCount();
    //遍历表单
    for( int i = 0 ; i < rows ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_upload->item( i , 0 );
        //看是否打钩
        if( item0->checkState() == Qt::Checked ) {
            QPushButton * button = (QPushButton *)ui->table_upload->cellWidget( i , 5 );
            //看按钮的状态 切换文字 发送信号
            if( button->text() == "开始" ) {
                //信号 设置文件信息结构体暂停标志位
                button->setText("暂停");
                Q_EMIT SIG_setUploadPause( item0->m_info.timestamp , 0 );
            }
        }
    }
}

void MainDialog::slot_downloadPause(bool flag)
{
    qDebug() << "下载暂停";
    int rows = ui->table_download->rowCount();
    //遍历表单
    for( int i = 0 ; i < rows ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_download->item(i, 0);
        //看是否打钩
        if( item0->checkState() == Qt::Checked ) {
            QPushButton * button = (QPushButton *)ui->table_download->cellWidget(i, 5);
            //看按钮的状态 切换文字 发送信号
            if( button->text() == "暂停" ) {
                //信号 设置文件信息结构体暂停标志位
                button->setText("开始");
                Q_EMIT SIG_setDownloadPause( item0->m_info.timestamp, 1 );
            }
        }
    }
}

void MainDialog::slot_downloadResume(bool flag)
{
    qDebug() << "下载开始";
    int rows = ui->table_download->rowCount();
    //遍历表单
    for ( int i = 0 ; i < rows ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_download->item( i, 0 );
        //看是否打钩
        if ( item0->checkState() == Qt::Checked ) {
            QPushButton * button = (QPushButton *)ui->table_download->cellWidget( i, 5 );
            //看按钮的状态 切换文字 发送信号
            if ( button->text() == "开始" ) {
                //信号 设置文件信息结构体暂停标志位
                button->setText("暂停");
                Q_EMIT SIG_setDownloadPause( item0->m_info.timestamp , 0 );
            }
        }
    }
}
#include<QProgressBar>
void MainDialog::slot_insertUploadFile(FileInfo &info)
{
    //表格插入信息
    //列：文件名 大小 时间 速率 进度 按钮
    //新增一行 获取当前行+1 设置行数
    int rows=ui->table_upload->rowCount();
    ui->table_upload->setRowCount(rows+1);
    //设置这一行的每一列控件（添加对象）
    Mytablewidgetitem*item0=new Mytablewidgetitem;
    item0->slot_setInfo(info);
    ui->table_upload->setItem(rows,0,item0);

    QTableWidgetItem* item1=new QTableWidgetItem(QString::number(info.size));
    ui->table_upload->setItem(rows,1,item1);

    QTableWidgetItem* item2=new QTableWidgetItem(info.time);
    ui->table_upload->setItem(rows,2,item2);

    QTableWidgetItem* item3=new QTableWidgetItem("0KB/s");
    ui->table_upload->setItem(rows,3,item3);
    //进度条
    QProgressBar*progress=new QProgressBar;
    progress->setMaximum(info.size);
    ui->table_upload->setCellWidget(rows,4,progress);
    //按钮

    QPushButton* button=new QPushButton;
    if(info.isPause==0){
        button->setText("暂停");
    }else{
        button->setText("开始");
    }
    ui->table_upload->setCellWidget(rows,5,button);
}

void MainDialog::slot_insertDownloadFile(FileInfo &info)
{
    //列：文件名 大小 时间 速率 进度 按钮
    //1.新增一行 获取当前行+1 设置行数
    int rows = ui->table_download->rowCount();
    ui->table_download->setRowCount(rows + 1);
    //2.设置这一行的每一列控件（添加对象）

    Mytablewidgetitem *item0 = new Mytablewidgetitem;
    item0->slot_setInfo(info);

    ui->table_download->setItem(rows, 0, item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(
    FileInfo::getSize(info.size));

    ui->table_download->setItem(rows, 1, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem(info.time);
    ui->table_download->setItem(rows, 2, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem("OKB/s");
    ui->table_download->setItem(rows, 3, item3);

    //进度条
    QProgressBar* progress = new QProgressBar;
    progress->setMaximum( info.size );
    ui->table_download->setCellWidget( rows, 4, progress );

    //按钮
    QPushButton * button = new QPushButton;
    if( info.isPause == 0 ) {
        button->setText( "暂停" );
    } else{
        button->setText( "开始" );
    }
    ui->table_download->setCellWidget( rows, 5, button );
}

void MainDialog::slot_insertUploadComplete(FileInfo &info)
{

    //列：文件名 大小 时间 速率 进度 按钮
    //新增一行 获取当前行+1 设置行数
    int rows=ui->table_complete->rowCount();
    ui->table_complete->setRowCount(rows+1);
    //设置这一行的每一列控件（添加对象）
    Mytablewidgetitem*item0=new Mytablewidgetitem;
    item0->slot_setInfo(info);
    ui->table_complete->setItem(rows,0,item0);

    QTableWidgetItem* item1=new QTableWidgetItem(FileInfo::getSize(info.size));
    ui->table_complete->setItem(rows,1,item1);

    QTableWidgetItem* item2=new QTableWidgetItem(info.time);
    ui->table_complete->setItem(rows,2,item2);

    QTableWidgetItem* item3=new QTableWidgetItem("上传完成");
    ui->table_complete->setItem(rows,3,item3);
}

void MainDialog::slot_insertShareFileInfo(QString name, int size, QString time, int shareLink)
{
    //列：文件名 大小 时间 分享码
    //1.新增一行 获取当前行+1 设置行数
    int rows = ui->table_share->rowCount();
    ui->table_share->setRowCount(rows + 1);
    //2.设置这一行的每一列控件（添加对象）

    QTableWidgetItem *item0 = new QTableWidgetItem( name );

    ui->table_share->setItem( rows, 0, item0 );

    QTableWidgetItem * item1 = new QTableWidgetItem( FileInfo::getSize(size) );

    ui->table_share->setItem( rows, 1, item1 );

    QTableWidgetItem * item2 = new QTableWidgetItem( time );
    ui->table_share->setItem( rows, 2, item2 );

    QTableWidgetItem * item3 = new QTableWidgetItem( QString::number( shareLink ) );
    ui->table_share->setItem( rows, 3, item3 );
}

void MainDialog::slot_updateUploadFileProgress(int timestamp, int pos)
{
    //遍历所有项 第0列
    int row=ui->table_upload->rowCount();
    for(int i=0;i<row;++i){
        //取到每一个文件信息的时间戳 看是否一致
        Mytablewidgetitem* item0=(Mytablewidgetitem*)(ui->table_upload->item(i,0));
        if(item0->m_info.timestamp==timestamp){
            //一致，更新进度
            QProgressBar* item4=(QProgressBar*)(ui->table_upload->cellWidget(i,4));
            item0->m_info.pos=pos;
            item4->setValue(pos);
            //看是否结束
            if(item4->value()>=item4->maximum()){
                //是 删除这一项 添加到完成


                slot_insertUploadComplete(item0->m_info);
                slot_deleteUploadFileByRow(i);

                //return
                return ;
            }

        }

    }
}

void MainDialog::slot_updateDownloadFileProgress(int timestamp, int pos)
{
    //遍历所有项 第0列
    int row=ui->table_download->rowCount();
    for(int i=0;i<row;++i){
        //取到每一个文件信息的时间戳 看是否一致
        Mytablewidgetitem* item0=(Mytablewidgetitem*)(ui->table_download->item(i,0));
        if(item0->m_info.timestamp==timestamp){
            //一致，更新进度
            QProgressBar* item4=(QProgressBar*)(ui->table_download->cellWidget(i,4));
            item0->m_info.pos=pos;
            item4->setValue(pos);
            //看是否结束
            if(item4->value()>=item4->maximum()){
                //是 删除这一项 添加到完成


                slot_insertDownloadComplete(item0->m_info);
                slot_deleteDownloadFileByRow(i);

                //return
                return ;
            }

        }

    }
}

void MainDialog::slot_deleteUploadFileByRow(int row)
{
    ui->table_upload->removeRow( row );
}

void MainDialog::slot_deleteDownloadFileByRow(int row)
{
    ui->table_download->removeRow( row );
}

void MainDialog::slot_insertFileInfo(FileInfo &info)
{
    //列：文件名 大小 时间 速率 进度 按钮
    //新增一行 获取当前行+1 设置行数
    int rows=ui->table_file->rowCount();
    ui->table_file->setRowCount(rows+1);
    //设置这一行的每一列控件（添加对象）
    Mytablewidgetitem* item0=new Mytablewidgetitem;
    item0->slot_setInfo(info);
    ui->table_file->setItem(rows,0,item0);

    QString strSize ;
    if( info.type == "file" ){
        strSize = FileInfo::getSize (info.size);
    }else{
        strSize = "   ";

    }

    QTableWidgetItem* item1=new QTableWidgetItem(strSize);
    ui->table_file->setItem(rows,1,item1);

    QTableWidgetItem* item2=new QTableWidgetItem(info.time);
    ui->table_file->setItem(rows,2,item2);

    QTableWidgetItem * item3 = new QTableWidgetItem( "上传完成" );
    ui->table_complete->setItem( rows, 3, item3 );
}

void MainDialog::slot_insertDownloadComplete(FileInfo &info)
{
    //列：文件名 大小 时间 速率 进度 按钮
    //新增一行 获取当前行+1 设置行数
    int rows=ui->table_complete->rowCount();
    ui->table_complete->setRowCount(rows+1);
    //设置这一行的每一列控件（添加对象）
    Mytablewidgetitem*item0=new Mytablewidgetitem;
    item0->slot_setInfo(info);
    ui->table_complete->setItem(rows,0,item0);

    QTableWidgetItem* item1=new QTableWidgetItem(FileInfo::getSize(info.size));
    ui->table_complete->setItem(rows,1,item1);

    QTableWidgetItem* item2=new QTableWidgetItem(info.time);
    ui->table_complete->setItem(rows,2,item2);

    QPushButton * button = new QPushButton();
    button->setIcon( QIcon(":/images/folder.png"));
    button->setFlat(true);
    button->setToolTip(info.absolutePath);
    connect( button, SIGNAL(clicked(bool)), this, SLOT(slot_openPath(bool)) );

    ui->table_complete->setCellWidget( rows , 3 , button );
}
#include<QProcess>
void MainDialog::slot_openPath( bool flag )
{
    QPushButton * button = (QPushButton *)QObject::sender();
    QString path = button->toolTip();

    // 如何打开文件夹
    // 转化
    path.replace('/', '\\');

    qDebug() << path;

    //如何打开文件夹
    //通过Qt 打开进程
    QProcess process;
    QStringList lst;
    //    lst.push_back("/select,");
    //    lst.push_back(path);
    lst << QString("/select,") << path ;

    process.startDetached("explorer", lst);
}

void MainDialog::slot_deleteAllFileInfo()
{
    //ui->table_file->clear(); //删文字 不用这个 行数不变

    int rows = ui->table_file->rowCount();
    for( int i = rows -1 ; i >= 0 ; i--){
        ui->table_file->removeRow(i);
    }
}

void MainDialog::slot_deleteAllShareInfo()
{
    int rows = ui->table_share->rowCount();
    for( int i = rows -1 ; i >= 0 ; i--){
        ui->table_share->removeRow(i);
    }
}

void MainDialog::on_table_file_cellClicked(int row, int column)
{
    //切换 勾选和未勾选状态
    Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_file->item( row, 0 );
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    } else{
        item0->setCheckState( Qt::Checked );
    }
}


void MainDialog::on_table_file_customContextMenuRequested(const QPoint &pos)
{
    m_menuFileInfo.exec(QCursor::pos());
}


void MainDialog::on_table_file_cellDoubleClicked(int row, int column)
{

        // 首先 拿到双击的那行的文件名字
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_file->item( row, 0 );

        // 判断是不是文件夹 是文件夹 可以跳转（是文件 考虑 打开文件 todo ）
        if( item0->m_info.type != "file" ) {
        // 是文件夹 路径 拼接
        QString dir = ui->lb_path->text() + item0->m_info.name + "/";
        // 设置路径 lb_path ->text
        ui->lb_path->setText( dir );
        // 发送信号 -> 更新当前的目录 -> 刷新文件列表
        Q_EMIT SIG_changeDir( dir );
        }

}


void MainDialog::on_pb_prev_clicked()
{
    //获取目录
    QString path = ui->lb_path->text();
    //判断 "/" 结束
    if( path == "/" ) return;
    // 首先找到最右边的 "/" 从他左边开始再向右找 "/"
    // /03/ left 取多少个长度
    path = path.left( path.lastIndexOf( "/" ) ); // -> /03

    //新的目录就是 找到的"/" 以及左边的所有字符
    path = path.left( path.lastIndexOf( "/" ) + 1 ); //
    qDebug() << path ;
    ui->lb_path->setText(path);
    //跳转路径
    Q_EMIT SIG_changeDir( path );
}


void MainDialog::on_table_upload_cellClicked(int row, int column)
{
    //切换 勾选和未勾选状态
    Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_upload->item(
    row, 0);
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    }else{
        item0->setCheckState( Qt::Checked );
    }
}


void MainDialog::on_table_download_cellClicked(int row, int column)
{
    //切换 勾选和未勾选状态
    Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_download->item(
    row, 0);
    if( item0->checkState() == Qt::Checked ){
        item0->setCheckState( Qt::Unchecked );
    }else{
        item0->setCheckState( Qt::Checked );
    }
}

bool MainDialog::slot_getDownloadFileInfoByTimestamp(int timestamp,FileInfo & info)
{
    //遍历所有第0列
    int rows = ui->table_download->rowCount();

    for( int i = 0 ; i < rows ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_download->item( i , 0 );
        if( item0->m_info.timestamp == timestamp ) {
            info=item0->m_info;
            return true;
        }
    }
    return false;
}
bool MainDialog::slot_getUploadFileInfoByTimestamp(int timestamp,FileInfo & info)
{
    //遍历所有第0列
    int rows = ui->table_upload->rowCount();

    for( int i = 0 ; i < rows ; ++i ) {
        Mytablewidgetitem * item0 = (Mytablewidgetitem *)ui->table_upload->item( i , 0 );
        if( item0->m_info.timestamp == timestamp ) {
            info=item0->m_info;
            return true;
        }
    }
    return false;
}

void MainDialog::slot_playOnline(bool flag)
{
    qDebug()<<__func__;
    //遍历列表
    int rows = ui->table_file->rowCount();
    QString dir = ui->lb_path->text(); //获取目录
    for( int i = 0 ; i < rows ; ++i ){
        //看选中的
        Mytablewidgetitem*item0 = (Mytablewidgetitem*)ui->table_file->item(i, 0);
        if( item0 ->checkState() == Qt::Checked ) {
            //获取类型，只处理文件
            if( item0 ->m_info.type == "file"){ //发信号  在线播放
                Q_EMIT SIG_playOnline( item0 ->m_info.fileid , dir );
            }
        }
    }
}
