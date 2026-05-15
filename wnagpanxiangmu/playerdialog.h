#ifndef PLAYERDIALOG_H
#define PLAYERDIALOG_H

#include <QDialog>
#include"vedioplayer.h"
#include<QTimer>
QT_BEGIN_NAMESPACE
namespace Ui { class PlayerDialog; }
QT_END_NAMESPACE

class PlayerDialog : public QDialog
{
    Q_OBJECT

public:
    PlayerDialog(QWidget *parent = nullptr);
    ~PlayerDialog();

    // 新增的公共函数，用于设置播放URL并开始播放
    void setVideoUrl(const QString &url);

private slots:
    void on_pb_start_clicked();
    void slot_setImage(QImage img);

    void on_pb_resume_clicked();

    void on_pb_pause_clicked();

    void on_pb_stop_clicked();
    void slot_PlayerStateChanged(int state);
    void slot_getTotalTime(qint64 uSec);

    void slot_TimerTimeOut();

    //事件过滤器
    bool eventFilter(QObject* obj,QEvent* event);
private:
    void createNewPlayer();
    
    Ui::PlayerDialog *ui;

    VedioPlayer * m_player;
    QTimer m_timer;
    bool isStop;
    QString m_currentVideoUrl;
};
#endif // PLAYERDIALOG_H
