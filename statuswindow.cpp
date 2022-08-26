#include "statuswindow.h"
#include "ui_statuswindow.h"


statuswindow::statuswindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::statuswindow)
{
    ui->setupUi(this);
}

statuswindow::~statuswindow()
{
    delete ui;
}

void statuswindow::refreshTable(DeviceData *dataModel) {
    ui->tableView->setModel(dataModel);
    //ui->tableView->set
    ui->tableView->viewport()->update();

}

/*void statuswindow::onNewDataReceived(const QAbstractItemModel &newData) {
    refreshTable(newData);
}*/
