#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QDateTime>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QThread>
#include <QRegularExpression>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

#include "oqplot.h"
#include "oqserialport.h"
#include "oqfiledialog.h"
#include "devicedata.h"
#include "statuswindow.h"
#include "serialcontroller.h"
#include "quickguidewindow.h"
#include "aboutwindow.h"
#include "logwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QLabel *statusLbl;
    QFile *saveFile;
    QFile *originalSaveFile;
    QFile *logSaveFile;
    QMenu *comPortMenu;
    QList<QSerialPortInfo> serialPorts;
    QThread serialThread;
    OqSerialPort *serialPortReader;
    SerialController *serialController;

    QTextStream out;
    QTextStream logOut;

    QTimer *serialTimer;
    QTimer *dataRequestTimer;
    QElapsedTimer saveTimer;

    QStringList allLogs;
    QStringList currentRequest;
    QMap<QString,double> receivedData;
    QMap<QString,QString> receivedUnits;
    QString defaultFileSuffix = "Nox";
    DeviceData currentData;

    bool requestComplete = true;
    bool isLogSaving = false;
    bool isSaving = false;
    bool isPortOpen = false;
    bool periodicSavingOn = false;

    qint32 rowsSaved = 0;
    qint32 numDataPoints = 0;
    qint64 savingPeriod;
    qint16 saveFileNumber;
    int requestIndex;

    void Log(QString msg);
    void refreshSerialPorts();
    void closeSerialPort();
    void printHeadersToFile();
    void autoGenerateNewFile();
    void requestNextField();
    void requestSameField();

    bool fileExists(QString path);
    QDateTime roundToClosestSecond(QDateTime datTimToRound);

    void saveAndPlotDataRow(QMap<QString,double> data, QMap<QString,QString> units); // Pass the QMaps here as the original ones might be cleared
    void startSaving();
    void stopSaving();
    void showDeviceStatus();

signals:
    void logTextAdded(QString fullMsg);
    void dataUpdated(DeviceData *newData);
    void writeToSerial(const char *dataToWrite);

public slots:
    void noCommunication();
    void connectToPort(QString portName,int index);
    void handlePortOpening(bool isOpen, QString portName);
    void handleSerialError(QSerialPort::SerialPortError error);
    void handleData(QString serialData);
    void requestData();
    void serialThreadFinished();


private slots:
    void on_actionRefresh_ports_triggered();

    void on_actionSave_as_triggered();

    void on_actionAll_parameters_triggered();

    void on_actionDisconnect_triggered();

    void on_actionQuick_guide_triggered();

    void on_actionAbout_triggered();

    void on_actionLog_triggered();

    void on_actionSave_log_as_triggered();

private:
    Ui::MainWindow *ui;
    OQPlot *Plot;
};
#endif // MAINWINDOW_H
