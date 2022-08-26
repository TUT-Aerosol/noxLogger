#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QWidget>
#include <QAbstractItemModel>
#include "devicedata.h"


namespace Ui {
class statuswindow;
}

class statuswindow : public QWidget
{
    Q_OBJECT

public:
    explicit statuswindow(QWidget *parent = nullptr);
    ~statuswindow();
    //void refreshTable(QAbstractItemModel *dataModel);
public slots:
    //void onNewDataReceived(const QAbstractItemModel &newData);
    void refreshTable(DeviceData *dataModel);

private:
    Ui::statuswindow *ui;
};

#endif // STATUSWINDOW_H
