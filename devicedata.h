#ifndef DEVICEDATA_H
#define DEVICEDATA_H

#include <QObject>
#include <QAbstractTableModel>

class DeviceData : public QAbstractTableModel
{
    Q_OBJECT
public:
    DeviceData(QObject *parent = 0);

    void setHeaders(const QList<QString> headerList);
    void populateTable(const QList<QString> tableValues);

    QStringList getHeaders();
    QStringList getData();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    QList<QString> headers;
    QList<QString> tableData;

};

#endif // DEVICEDATA_H
