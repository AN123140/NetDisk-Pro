#include "maindialog.h"
#include"ckernel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);//系统初始化
    //MainDialog w;
    //w.show();
   CKernel::GetInstance();//创建所有窗口、对象，全部属于主线程

    return a.exec();//GUI事件循环
}
