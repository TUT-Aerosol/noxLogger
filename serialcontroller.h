#ifndef SERIALCONTROLLER_H
#define SERIALCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QDebug>

#include "oqserialport.h"

class SerialController : public QObject
{
    Q_OBJECT
    QThread workerThread;
public:
    SerialController(QString portName, qint32 baudRate);
    ~SerialController();
    void write(QString dataToWrite);

signals:
    void newData(QString serialData);
    void openStatus(bool status, QString portName);
    void finished();
    void errorOccurred(QSerialPort::SerialPortError);
    void readyToWrite(QString dataToWrite);
    void closeSerial();


};

#endif // SERIALCONTROLLER_H
