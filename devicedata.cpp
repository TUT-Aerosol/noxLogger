#include "devicedata.h"

DeviceData::DeviceData(QObject *parent) : QAbstractTableModel(parent)
{
}

void DeviceData::setHeaders(QList<QString> headerList) {
    if(!tableData.isEmpty() && headerList.length() != tableData.length()) {
        return; // TODO: throw an error
    }
    headers = headerList;
}

void DeviceData::populateTable(const QList<QString> tableValues) {
    if(headers.isEmpty()) {
        // Make numbered headers:
        QList<QString> numberedHeaders;
        for(int i=0; i<tableValues.length(); i++)
            numberedHeaders.append(QString::number(i));
        setHeaders(numberedHeaders);
    }
    if(headers.length() != tableValues.length())
        return; // TODO: throw an error instead
    tableData = tableValues;
}

int DeviceData::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    //return 1;
    return tableData.length();
}

int DeviceData::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    //return tableData.length();
    return 1;
}

QVariant DeviceData::data(const QModelIndex &index, int role) const {
    if(!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }
    //if(index.row() == 0) {
    if(index.column() == 0) {
        //return tableData.at(index.column());
        return tableData.at(index.row());
    }
    return QVariant();
}

QVariant DeviceData::headerData(int section, Qt::Orientation orientation, int role) const {
    //if(role == Qt::DisplayRole && orientation == Qt::Horizontal && section < headers.length()) {
    if(role == Qt::DisplayRole && orientation == Qt::Vertical && section < headers.length()) {
        return headers.at(section);
    }
    return QVariant();
}


QStringList DeviceData::getHeaders() {
    return headers;
}

QStringList DeviceData::getData()  {
    return tableData;
}
