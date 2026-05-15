#ifndef MYTABLEWIDGETITEM_H
#define MYTABLEWIDGETITEM_H

#include <QTableWidgetItem>
#include"common.h"

class MainDialog;
class Mytablewidgetitem : public QTableWidgetItem
{
public:
    Mytablewidgetitem();

public slots:
    void slot_setInfo(FileInfo & info);
private:
    FileInfo m_info;
    friend class MainDialog;
};

#endif // MYTABLEWIDGETITEM_H
