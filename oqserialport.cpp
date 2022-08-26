#include "oqserialport.h"
#include <QDebug>
#include<QThread>

OqSerialPort::OqSerialPort(QString portName, qint32 baudRate, QObject *parent) :
    QObject(parent)

{
    m_portName = portName;
    m_baudRate = baudRate;
}

void OqSerialPort::init() {
    // Create the QSerialPort here, after the OqSerialPort object is moved to its own thread,
    // so that the QSerialPort object will be created in the same thread.
    m_serialPort = new QSerialPort(this);
    qRegisterMetaType<QSerialPort::SerialPortError>();
    connect(m_serialPort, &QSerialPort::readyRead, this, &OqSerialPort::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &OqSerialPort::handleError);
    m_serialPort->setPortName(m_portName);
    m_serialPort->setBaudRate(m_baudRate);

    bool status = m_serialPort->open(QIODevice::ReadWrite);
    Q_EMIT(openStatus(status, m_portName));
    qDebug() << "Serial initialization finished.";
}

void OqSerialPort::write(QString dataToWrite) {
    m_serialPort->write(dataToWrite.toLatin1());
}

void OqSerialPort::close() {
    if(m_serialPort->isOpen())
        m_serialPort->close();
    qDebug() << "Serial closed.";
    qDebug() << "Serial finished";
    Q_EMIT(finished());
}

bool OqSerialPort::isOpen() {
    return m_serialPort->isOpen();
}

QString OqSerialPort::getPortName() {
    return m_serialPort->portName();
}

void OqSerialPort::handleReadyRead() {
    //qDebug() << "Ready read";
    m_readData.append(m_serialPort->readAll());
    QStringList rows = m_readData.split("\n");
    if(rows.last().endsWith("\n"))
        m_readData.clear();
    else {
        m_readData = rows.last();
        rows.removeLast();
    }
    for(int i=0; i < rows.length(); i ++) {
        //qDebug() << "Row " << i << ": " << rows.at(i);
        QString textToEmit = rows.at(i);
        //qDebug("Serial emitting.");
        Q_EMIT(newData(textToEmit));
        // qDebug() << "This line executed after QEMIT";
    }
}

void OqSerialPort::handleError(QSerialPort::SerialPortError error) {
    //if(m_serialPort->isOpen())
        Q_EMIT(errorOccurred(error));
}
