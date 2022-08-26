#include "serialcontroller.h"

SerialController::SerialController(QString portName, qint32 baudRate)
    : QObject{}
{
    OqSerialPort *m_serialPort = new OqSerialPort(portName,baudRate);

    // We need to operate the serial communication in another thread. Otherwise the data transfer freezes
    // when e.g. window is being resized:
    m_serialPort->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, m_serialPort, &QObject::deleteLater);
    connect(this,&SerialController::readyToWrite,m_serialPort,&OqSerialPort::write);
    connect(&workerThread, &QThread::started, m_serialPort, &OqSerialPort::init);
    connect(m_serialPort,&OqSerialPort::openStatus,this,&SerialController::openStatus);
    connect(m_serialPort,&OqSerialPort::newData,this,&SerialController::newData);
    connect(m_serialPort,&OqSerialPort::errorOccurred,this,&SerialController::errorOccurred);
    connect(this,&SerialController::closeSerial,m_serialPort,&OqSerialPort::close);
    workerThread.start();
}
SerialController::~SerialController() {
    workerThread.quit();
    workerThread.wait();
}
void SerialController::write(QString dataToWrite) {
    //qDebug() << "Trying to write: " << dataToWrite;
    Q_EMIT(readyToWrite(dataToWrite));
}
