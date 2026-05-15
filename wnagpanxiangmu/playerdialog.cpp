#include "playerdialog.h"
#include "ui_playerdialog.h"


//#define _DEF_PATH "D:/tomcat111/1.mp4"
//#define _DEF_PATH "rtmp://192.168.23.132:1935/vod//101.mp4"
#define _DEF_LIVE_PATH "rtmp://192.168.23.132/videotest/user=100"
#define _DEF_PATH "rtmp://192.168.23.132:1935/vod//102.mp4"
#define _DEF_PATHB "http://192.168.23.132:80/hls/C278170B3D55098C3222C1B5E49B44D9/output.m3u8"

void PlayerDialog::createNewPlayer()
{
    // 如果已有播放器，先安全停止并删除
    if (m_player) {
        m_player->stop(true);
        m_player->wait();
        delete m_player;
    }
    // 创建新的播放器实例
    m_player = new VedioPlayer;
    connect( m_player , SIGNAL(SIG_getOneImage(QImage)) , this , SLOT(slot_setImage(QImage)) );
    connect( m_player , SIGNAL(SIG_PlayerStateChanged(int)) ,
        this , SLOT( slot_PlayerStateChanged(int)));
    connect( m_player , SIGNAL( SIG_TotalTime(qint64) ) ,
        this , SLOT( slot_getTotalTime(qint64) ) );
}

PlayerDialog::PlayerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PlayerDialog)
    , m_player(nullptr)
{
    ui->setupUi(this);
    createNewPlayer();
    slot_PlayerStateChanged(PlayerState::Stop);
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(slot_TimerTimeOut()));
    m_timer.setInterval(500);
    ui->slider_progress->installEventFilter(this);
}

PlayerDialog::~PlayerDialog()
{
    delete ui;
    if (m_player) {
        m_player->stop(true);
        m_player->wait();
        delete m_player;
    }
}

// 新增的实现函数
void PlayerDialog::setVideoUrl(const QString &url)
{
    m_currentVideoUrl = url;

    // 创建全新的播放器实例来避免状态问题
    createNewPlayer();

    m_player->setFileName( url );
    m_player->start();
    slot_PlayerStateChanged(PlayerState::Playing);
}

void PlayerDialog::on_pb_start_clicked()
{
    if( !m_player || m_player->playerState() == PlayerState::Playing ) {
        return;
    }

    if( m_currentVideoUrl.isEmpty() ) {
        return;
    }

    // 创建全新的播放器实例来避免状态问题
    createNewPlayer();

    m_player->setFileName( m_currentVideoUrl );
    m_player->start();
    slot_PlayerStateChanged(PlayerState::Playing);
}

void PlayerDialog::slot_setImage(QImage img)
{
    //pixmap image
        //视频加速渲染openGL

    ui->wdg_show->slot_setImage( img );
}

//界面一蹦出来点击开始按钮，调用m_player->start();，然后调用vedioplayer里面的run（）函数，获取路径，循环拿到图片,通过信号，槽函数循环调用图片

void PlayerDialog::on_pb_resume_clicked()
{
    if(m_player->playerState()!=PlayerState::Pause)return;
    m_player->play();

    ui->pb_resume->hide();
    ui->pb_pause->show();
}


void PlayerDialog::on_pb_pause_clicked()
{
    if(m_player->playerState()!=PlayerState::Playing)return;
    m_player->pause();

    ui->pb_resume->show();
    ui->pb_pause->hide();
}


void PlayerDialog::on_pb_stop_clicked()
{
    m_player->stop(true);
}
#include<QDebug>
void PlayerDialog::slot_PlayerStateChanged(int state)
{
    switch( state )
    {
    case PlayerState::Stop:
        qDebug()<< "VideoPlayer::Stop222";
        m_timer.stop();
        ui->slider_progress->setValue(0);
        ui->lb_totalTime->setText("00:00:00");
        ui->lb_curTime->setText("00:00:00");
        //ui->lb_videoName->setText("");
        ui->pb_pause->hide();
        ui->pb_resume->show();
    {
        QImage img;
        img.fill( Qt::black);
        slot_setImage(img);
    }
        this->update();
        isStop = true;
        break;
    case PlayerState::Playing:
        qDebug()<< "VideoPlayer::Playing";
        ui->pb_resume->hide();
        ui->pb_pause->show();
        m_timer.start();
        this->update();
        isStop = false;
        break;
    }
}

void PlayerDialog::slot_getTotalTime(qint64 uSec)
{
    qint64 Sec = uSec/1000000;
    ui->slider_progress->setRange(0,Sec);//精确到秒
    QString hStr = QString("00%1").arg(Sec/3600);
    QString mStr = QString("00%1").arg(Sec/60);
    QString sStr = QString("00%1").arg(Sec%60);
    QString str =
            QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));
    ui->lb_totalTime->setText(str);
}
//获取当前视频时间定时器
void PlayerDialog::slot_TimerTimeOut()
{
    if (QObject::sender() == &m_timer)
    {
        qint64 Sec = m_player->getCurrentTime()/1000000;
        ui->slider_progress->setValue(Sec);
        QString hStr = QString("00%1").arg(Sec/3600);
        QString mStr = QString("00%1").arg(Sec/60%60);
        QString sStr = QString("00%1").arg(Sec%60);
        QString str =
                QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));
        ui->lb_curTime->setText(str);
        if(ui->slider_progress->value() == ui->slider_progress->maximum()
                && m_player->playerState() == PlayerState::Stop)
        {
            slot_PlayerStateChanged( PlayerState::Stop );
        }else if(ui->slider_progress->value() + 1 ==
                 ui->slider_progress->maximum()
                 && m_player->playerState() == PlayerState::Stop)
        {
            slot_PlayerStateChanged( PlayerState::Stop );
        }
    }
}
#include<QMouseEvent>
#include<QStyle>
bool PlayerDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->slider_progress) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            int min=ui->slider_progress->minimum();
            int max=ui->slider_progress->maximum();
            int value = QStyle::sliderValueFromPosition(
                        min, max, mouseEvent->pos().x(), ui->slider_progress->width());
            m_timer.stop();
            ui->slider_progress->setValue(value);
            m_player->seek((qint64)value*1000000); //value 秒
            m_timer.start();
            return true;
        } else {
            return false;
        }
    }
    // pass the event on to the parent class
    return QDialog::eventFilter(obj, event);

}
