#include "mainwindow.h"
#include "ui_mainwindow.h"

// TODO: Use Modbus instead of ASCII data transfer!
// TODO: Check for warnings
// TODO: Check antialiasing

#define MAX_PLOT_ITEMS 12*3600

QStringList desiredData = {"NO","NOX","NO2","NH3","SAMPFLOW","OZONEFLOW","AUTOZERO","RCELLTEMP","BOXTEMP","RCELLPRESS","SAMPPRESS","CONVTEMP","PMT","NORMPMT","PMTTEMP","NOXSLOPE","NOSLOPE"};
// Device responses that correspond to commands above (desiredData):
QStringList responseStrings = {"NO","NOX","NO2","NH3","SAMP FLW","OZONE FL","AZERO","RCELL TEMP","BOX TEMP","RCEL","SAMP","MOLY TEMP","PMT","NORM PMT","PMT TEMP","NOX SLOPE","NO SLOPE"};
QMap<QString,QString> response2request;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    for(int i=0; i<desiredData.length(); i++) {
        response2request.insert(responseStrings.at(i),desiredData.at(i));
    }

    // Add custom statusbar (otherwise text disappears when using menus):
    statusLbl = new QLabel();
    statusLbl->setAlignment(Qt::AlignTop);
    // Add the statusbar inside ScrollArea to prevent it from resizing the window
    // in case of long message.
    QScrollArea *lblScrollArea = new QScrollArea();
    lblScrollArea->setAlignment(Qt::AlignTop);
    lblScrollArea->setWidgetResizable(true);
    lblScrollArea->setWidget(statusLbl);
    lblScrollArea->setMaximumHeight(20);
    lblScrollArea->verticalScrollBar()->setEnabled(false);
    lblScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lblScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lblScrollArea->setFrameShape(QFrame::NoFrame);
    ui->horizontalLayout_2->addWidget(lblScrollArea);

    // Load and show OQ logo:
    QGraphicsScene *scene = new QGraphicsScene();
    QGraphicsSvgItem *svgItem = new QGraphicsSvgItem(":/images/OQlogo.svg");
    svgItem->setScale(0.8);
    scene->addItem(svgItem);
    ui->graphicsView->setStyleSheet("background: transparent");
    ui->graphicsView->fitInView(svgItem->boundingRect(),Qt::KeepAspectRatio);
    ui->graphicsView->setScene(scene);
    //ui->graphicsView->fitInView(svgItem->boundingRect(),Qt::KeepAspectRatio);
    ui->graphicsView->show();

    Log("Program started.");

    // Set up a timer to detect if the device is not sending data:
    serialTimer = new QTimer;
    serialTimer->setSingleShot(true);
    serialTimer->setInterval(5000); // 5 sec timeout for serial connection
    connect(serialTimer,SIGNAL(timeout()),this,SLOT(noCommunication()));

    // Set up a timer to request data at one second interval:
    dataRequestTimer = new QTimer;
    dataRequestTimer->setSingleShot(false);
    dataRequestTimer->setInterval(1000); // 1 sec time interval for data request
    connect(dataRequestTimer,SIGNAL(timeout()),this,SLOT(requestData()));

    // Create the plot area:
    Plot = new OQPlot(ui->qwtPlot);
    QColor Colors[8] = {Qt::blue, Qt::red, Qt::green, Qt::cyan, Qt::magenta, Qt::darkCyan, Qt::gray, Qt::darkBlue};
    Plot->AddCurve("NOx", "ppb", Colors[0]);
    Plot->AddCurve("NO", "ppb", Colors[1]);
    Plot->AddCurve("NO2","ppb", Colors[2]);
    Plot->AddCurve("NH3", "ppb", Colors[3]);

    // Check available serial ports:
    comPortMenu = ui->menuSelect_port;
    refreshSerialPorts();

}

void MainWindow::Log(QString msg) {
    QString fullMsg = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") + ": " + msg;
    qDebug() << fullMsg;

    statusLbl->setText(fullMsg);
    allLogs.append(fullMsg);
    if(isLogSaving) {
        logOut << fullMsg << "\r\n";
        logOut.flush();
        logSaveFile->flush();
    }
    emit logTextAdded(fullMsg);
}

void MainWindow::noCommunication() {
    Log("No response from the COM port.");
    QMessageBox::information(this, tr("COM port error"),"No response from the COM port. Disconnecting.");
    closeSerialPort();
}

void MainWindow::refreshSerialPorts() {
    // First remove all serial ports from COM port menu, so that we can add the currently available ports later:
    QList<QAction*> actionsList = comPortMenu->actions();
    for(int i=0; i<actionsList.length(); i++){
        comPortMenu->removeAction(actionsList.at(i));
    }

    // Refresh serial ports and add them
    serialPorts = QSerialPortInfo::availablePorts();
    int i = 0;
    foreach(const QSerialPortInfo &serialPortInfo, serialPorts) {
        QString portName = serialPortInfo.portName();
        comPortMenu->addAction(portName,this,[this,portName,i]() {connectToPort(portName,i);});
        i ++;
    }
}

void MainWindow::connectToPort(QString portName, int index) {
    serialController = new SerialController(portName,115200);
    connect(serialController,&SerialController::openStatus,this,&MainWindow::handlePortOpening);
    connect(serialController,&SerialController::errorOccurred,this,&MainWindow::handleSerialError);
    connect(serialController,&SerialController::newData,this,&MainWindow::handleData);

    QList<QAction*> actionsList = comPortMenu->actions();
    for(int i=0; i<actionsList.length(); i++){
        actionsList.at(i)->setEnabled(false);
        if(i == index) {
            actionsList.at(i)->setText(portName + " (active)");
        }
    }
}

void MainWindow::serialThreadFinished() {
    Log("Serial thread finished.");
}

void MainWindow::requestData() {
    if(!requestComplete) {
        // The previous data request is not complete yet.
        //Log("Previous data request not complete yet.");
        return;
    }
    currentRequest = desiredData;
    receivedData.clear();
    receivedUnits.clear();
    requestComplete = false;
    requestIndex = 0;
    QString request = QString("T ") + currentRequest.at(requestIndex) + QString("\r\n");
    //Log("Requesting data...");

    serialController->write(request);
    dataRequestTimer->start(); // Restart the timer to keep the 1 sec interval in case the previous request was delayed.
}

void MainWindow::handleData(QString serialData) {
    //Log("Handling data...");
    if(serialTimer->isActive())
        serialTimer->stop(); // Disable timeout counting, because apparently we are connected...

    if(requestComplete) {
        Log("Data received but none requested. The row was: " + serialData);
        return;
    }

    // Let's handle the received data and add it to receivedData and receivedUnits:
    // The data received is in form: 'T   012:16:30   0000   NO=100 ppb' (T + time + DeviceID + data + [units])
    // First, let's remove the extra whitespaces:
    serialData = serialData.simplified();
    // Then find the part that contains '=', as this part contains the data:
    QStringList splittedStr = serialData.split(" ");
    // Sometimes a variable name has some white space around. So let's sum all strings between the device ID and '='-character,
    // as the data row is in format: "T [time] [device ID] [variable name]=[value] [unit]".

    // First check the validity of the data row:
    if(splittedStr.length() < 5) {
        Log("Invalid data row. The row was: " + serialData);
        Log("Lenght is: " + QString::number(splittedStr.length()));
        requestSameField();
        return;
    }

    int nameValueIndex = splittedStr.indexOf(QRegularExpression(".*=.*")); // Any string that contains '='
    if(nameValueIndex == -1) {
        Log("Invalid data row. The row was: " + serialData);
        Log("The splitted row was:");
        for(QString& currStr : splittedStr) {
            Log(currStr);
        }
        requestSameField();
        return;
    }
    QString dataNameAndValue = splittedStr.mid(3,nameValueIndex-2).join(" ");

    /*if(splittedStr.length() > 5) { // Normally we have 5 strings, but if variable name contains white spaces, we'll have more strings
        // Let's concatenate the variable name string into one
        QStringList newStrList;
        bool isVarnameComplete = false;
        QString varName = "";
        for(int i=0; i<splittedStr.length(); i++) {
            if(i < 3 || isVarnameComplete)
                newStrList.append(splittedStr.at(i));
            else
                varName += splittedStr.at(i) + " ";
            if(splittedStr.at(i).contains("=")) {
                isVarnameComplete = true;
                newStrList.append(varName.trimmed()); // Remove the extra whitespace at the end
            }
        }
        splittedStr = newStrList;
    }*/

    QStringList splittedDataStr = dataNameAndValue.split("=");
    if(splittedDataStr.length() != 2) {
        Log("Invalid data row. The dataNameAndValue was: " + dataNameAndValue);
        requestSameField();
        return;
    }
    QString dataName = splittedDataStr.at(0);
    //Log("Data name before conversion: " + dataName);
    if(response2request.contains(dataName)) {
        dataName = response2request.value(dataName); // The data name might be different to the requested name... E.g. request "SAMPFLOW" returns "SAMP FLW".
        //Log("Data name after conversion: " + dataName);
    }
    else {
        Log("Unknown dataName in response. The response dataName was: " + dataName + ". Continuing to next request.");
        requestNextField();
    }
    if(dataName != currentRequest.at(requestIndex)) {
        Log("The dataName requested does not correspond with the received. Requested was:  " + currentRequest.at(requestIndex) + " and received was: " + dataName + "Continuing to next field.");
        requestNextField();
    }

    QString dataUnit;
    if(splittedStr.length() > nameValueIndex + 1) {
        dataUnit = splittedStr.at(nameValueIndex+1);
    }
    else {
        dataUnit = "";
    }

    bool dataOk = false;
    double dataValue = splittedDataStr.at(1).toDouble(&dataOk);
    if(!dataOk)
        dataValue = NAN;

    /*int requestIndex = currentRequest.indexOf(QRegularExpression(dataName)); // Must match exactly to dataName
    if(requestIndex == -1) {
        Log("The requested data was not received. The row was: " + serialData);
        Log("The dataName was: " + dataName);
        Log("Current request was: ");
        for(QString& curr : currentRequest) {
            Log(curr);
        }
        Log("Continuing to next request.");
        dataValue = NAN;
    }*/

    receivedData.insert(currentRequest.at(requestIndex),dataValue);
    receivedUnits.insert(currentRequest.at(requestIndex),dataUnit);

    requestNextField();
    /*if(currentRequest.length() > 0) {
        // Let's move on to the next data request:
        //Log("Requesting additional data.");
        QString request = QString("T ") + currentRequest.at(0) + QString("\r\n");
        serialTimer->start();
        serialController->write(request);
        return;
    }
    else {
        //Log("Data handling complete.");
        saveAndPlotDataRow(receivedData,receivedUnits);
    }*/

}

void MainWindow::requestNextField() {
    requestIndex++;
    if(requestIndex > currentRequest.length()-1) {
        // Data handling complete
        saveAndPlotDataRow(receivedData,receivedUnits);
    }
    else {
        serialTimer->start();
        QString request = QString("T ") + currentRequest.at(requestIndex) + QString("\r\n");
        //Log("Current request: " + currentRequest.at(requestIndex));
        serialController->write(request);
    }
}

void MainWindow::requestSameField() {
    serialTimer->start();
    QString request = QString("T ") + currentRequest.at(requestIndex) + QString("\r\n");
    //Log("Current request: " + currentRequest.at(requestIndex));
    serialController->write(request);
}

void MainWindow::saveAndPlotDataRow(QMap<QString,double> data, QMap<QString,QString> units) {
    requestComplete = true; // Now the data and units are passed here so the request is complete and 'receivedData' and 'receivedUnits' can be cleared elsewhere.

    QDateTime currentTime = QDateTime::currentDateTime();

    QStringList data2Plot = {"NOX","NO","NO2","NH3"};

    for(int i=0; i < data2Plot.length(); i++) {
        if(!units.value(data2Plot.at(i)).isEmpty()) {
            // Let's assume that all three have the same units:
            Plot->setYLeftLabel(units.value(data2Plot.at(i)));
        }
        Plot->setUnit(i,units.value(data2Plot.at(i)));
        Plot->AddPoint(i,currentTime,data.value(data2Plot.at(i)));
    }

    // Update the RCEL pressure text:
    if(!qIsNaN(data["RCELLPRESS"])) {
        ui->RcelPressText->setText("RCEL pressure: " + QString::number(data["RCELLPRESS"]) + " in-Hg");
    }
    else
        ui->RcelPressText->setText("RCEL pressure: NaN");

    if(isSaving) {
        if(!saveTimer.isValid()) {
            saveTimer.start();
        }
        if(periodicSavingOn && saveTimer.elapsed() > saveFileNumber*savingPeriod) {
            autoGenerateNewFile();
        }

        // Write all data to file (comma separated):
        out << roundToClosestSecond(currentTime).toString("yyyy-MM-dd hh:mm:ss");

        for(int i=0; i<desiredData.length(); i++) {
            if(qIsNaN(data.value(desiredData.at(i))))
                out << ",NaN," << units.value(desiredData.at(i));
            else
                out << "," << QString::number(data.value(desiredData.at(i))) << "," << units.value(desiredData.at(i));
        }
        out << "\r\n";
        out.flush();
        saveFile->flush();
        rowsSaved = rowsSaved + 1;
        //ui->RowsSavedTxt->setText(QString("Rows saved: %1").arg(rowsSaved));
    }

    // Update currentData to be shown in separate window if it's open:
    QStringList headers = data.keys();
    QStringList dataStr;
    currentData.setHeaders(headers);
    for(QString& currHeader : headers) {
        dataStr.append(QString::number(data.value(currHeader)) + " " + units.value(currHeader));
    }
    currentData.populateTable(dataStr);
    emit dataUpdated(&currentData);

    // Check if we exceed the Plot limit:
    if(numDataPoints > MAX_PLOT_ITEMS) {
        for(int i=0; i<Plot->getNumCurves(); i++)
            Plot->RemoveFirstPoint(i);
        numDataPoints --;
    }

    numDataPoints ++;

}

void MainWindow::handlePortOpening(bool isOpen, QString portName) {
    //Log("Handling port opening.");
    if(isOpen) {
        //Log("Port is open.");
        requestComplete = true;
        isPortOpen = true;
        serialTimer->start(); // Start timer to disconnect after 5 secs if no data received
        // Start the data request timer
        //Log("Starting data request timer.");
        dataRequestTimer->start();
        //Log("Timer started.");
        Log("You connected to port " + portName);
        ui->actionDisconnect->setEnabled(true);
        if(!isSaving)
            startSaving();
    }
    else {
        serialTimer->stop();
        isPortOpen = false;
        refreshSerialPorts();
        QMessageBox::information(this, tr("COM port error"),"Could not open COM port.");
        isPortOpen = false;
        delete serialController;
    }
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError error) {
    switch(error) {
    case QSerialPort::NoError:
        break;
    case QSerialPort::DeviceNotFoundError:
        closeSerialPort();
        Log("COM port not found.");
        QMessageBox::information(this, tr("COM port error"),"COM port not found.");
        break;
    case QSerialPort::PermissionError:
        closeSerialPort();
        Log("COM port permission error (in use?)");
        QMessageBox::information(this, tr("COM port error"),"COM port permission error (in use?).");
        break;
    case QSerialPort::OpenError:
        closeSerialPort();
        Log("Attempting to open an already opened port.");
        QMessageBox::information(this, tr("COM port error"),"Attempting to open an already opened port.");
        break;
    case QSerialPort::NotOpenError:
        closeSerialPort();
        Log("COM port is not open.");
        QMessageBox::information(this, tr("COM port error"),"Port is not open.");
        break;
    case QSerialPort::WriteError:
        Log("Serial port I/O error while writing data.");
        break;
    case QSerialPort::ReadError:
        Log("Serial port I/O error while reading data.");
        break;
    case QSerialPort::ResourceError:
        closeSerialPort();
        Log("COM port unavailable (cable plugged out?).");
        QMessageBox::information(this, tr("COM port error"),"COM port unavailable (cable plugged out?).");
        break;
    case QSerialPort::TimeoutError:
        Log("Serial port time out error.");
        break;
    default:
        Log("Unknown serial port error.");
        break;
    }
}

void MainWindow::closeSerialPort() {
    serialTimer->stop();
    dataRequestTimer->stop();
    if(isPortOpen) {
        serialController->closeSerial();
        delete serialController;
    }
    isPortOpen = false;
    if(isSaving) {
        stopSaving();
    }
    ui->actionDisconnect->setEnabled(false);
    refreshSerialPorts();
}

void MainWindow::showDeviceStatus() {
    statuswindow *statusTable = new statuswindow(this);
    statusTable->setWindowFlag(Qt::Window);
    statusTable->refreshTable(&currentData);
    statusTable->show();
    connect(this,SIGNAL(dataUpdated(DeviceData*)),statusTable,SLOT(refreshTable(DeviceData*)));
}

void MainWindow::printHeadersToFile() {
    out << "[FILEVERSION]\r\n";
    out << "2.0\r\n";

    // Write headers

    out << "\r\n[DATA]\r\n";
    out << "Time (Y-m-d H:M:S)";
    for(QString& currStr : desiredData) {
        out << "," << currStr << "," << currStr << " unit";
    }
    out << "\r\n";
    out.flush();
    saveFile->flush();
}

void MainWindow::stopSaving() {
    isSaving = false;
    saveTimer.invalidate();
    saveFile->flush();
    saveFile->close();
    periodicSavingOn = false;
    ui->actionSave_as->setText(tr("Save as..."));
    rowsSaved = 0;
    ui->saveLabel->setText("NOT saving!");
    // Stop saving log:
    if(isLogSaving) {
        isLogSaving = false;
        logOut.flush();
        logSaveFile->flush();
        logSaveFile->close();
        ui->actionSave_log_as->setEnabled(false);
        ui->actionSave_log_as->setText("Save log as...");
    }
}

void MainWindow::startSaving() {
    if(isSaving) {
        stopSaving();
        return;
    }

    const QString DEFAULT_DIR_KEY("defaultSaveDir");
    QSettings appSettings;
    QString defaultDirectory;

    if(appSettings.contains(DEFAULT_DIR_KEY)) {
        defaultDirectory = appSettings.value(DEFAULT_DIR_KEY).toString();
        QDir testDir(defaultDirectory);
        if(!testDir.exists()) {
            qDebug() << "Default directory does not exist.";
            qDebug() << "Default directory is: " << defaultDirectory;
            defaultDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
    }
    else {
        defaultDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        qDebug() << "No default directory found.";
        qDebug() << "Using " << defaultDirectory << " as the directory";
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    QString currentTimeStr = currentTime.toString("yyyyMMdd_hhmm");
    QString defaultFilename = currentTimeStr + defaultFileSuffix + ".dat";

    OQFileDialog *fileDialog = new OQFileDialog(this, "Save as...",{"Data file (*.dat)","All files (*)"},defaultFilename,true);
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    if(!defaultDirectory.isEmpty()) {
        fileDialog->setDirectory(defaultDirectory);
    }
    fileDialog->setFileMode(QFileDialog::AnyFile);
    if(!fileDialog->exec()) {
        ui->saveLabel->setText("NOT saving!");
        isSaving = false;
        delete fileDialog;
        return;
    }

    QString selectedFile = fileDialog->selectedFiles().first();

    if(!selectedFile.isEmpty()) {
        QFileInfo fi(selectedFile);
        appSettings.setValue(DEFAULT_DIR_KEY,fi.absolutePath());

        saveFile = new QFile(selectedFile);
        originalSaveFile = new QFile(selectedFile);

        if(!saveFile->open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),saveFile->errorString());
            ui->saveLabel->setText("NOT saving!");
            isSaving = false;
            delete fileDialog;
            return;
        }

        out.setDevice(saveFile);

        printHeadersToFile();

        isSaving = true;
        ui->actionSave_as->setText(tr("Stop saving"));
        ui->actionSave_log_as->setEnabled(true);
        Log("Data saving started.");

        periodicSavingOn = false;
        if(fileDialog->periodCheckBox->isChecked()) {
            bool ok;
            savingPeriod = (qint64) (fileDialog->periodTextEdit->text().toDouble(&ok) * 3600 * 1000); // In msecs
            if(!ok || savingPeriod < 60*1000) {
                periodicSavingOn = false;
                // Show dialog
                QMessageBox wrnBox;
                wrnBox.setText("Could not resolve saving period. Automatic file generation is off.");
                wrnBox.setStandardButtons(QMessageBox::Ok);
                wrnBox.exec();
            }
            else {
                periodicSavingOn = true;
                saveFileNumber = 1;
            }
        }
        rowsSaved = 0;
        delete fileDialog;

        ui->saveLabel->setText("Saving on.");
    }
}

void MainWindow::autoGenerateNewFile() {
    QFileInfo oldFileInfo = QFileInfo(*originalSaveFile);
    QString oldBaseName = oldFileInfo.baseName();
    QString oldSuff = oldFileInfo.completeSuffix();
    QString oldPath = oldFileInfo.absolutePath();

    QDateTime currentTime = QDateTime::currentDateTime();
    QString currentTimeStr = currentTime.toString("yyyyMMdd_hhmm");

    QString newFileName = oldPath + QDir::separator() + oldBaseName + "_" + currentTimeStr + "." + oldSuff;

    int additionalSuffix = 0;
    while(fileExists(newFileName) && additionalSuffix < 100) {
        // If file already exists, let's add a number (0-99) and try with that:
        newFileName = oldPath + QDir::separator() + oldBaseName + "_" + currentTimeStr + "_" + QString::number(additionalSuffix) + "." + oldSuff;
        additionalSuffix ++;
    }

    if(additionalSuffix < 100) {
        out.flush();
        saveFile->flush();
        saveFile->close();
        saveFile = new QFile(newFileName);
        if(!saveFile->open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open save file"),saveFile->errorString());
            return;
        }
        out.setDevice(saveFile);
        printHeadersToFile();
    }
    else {
        Log("Could not create new save file; continuing in the current file.");
        return;
    }
    Log("New save file started (" + oldBaseName + "_" + currentTimeStr + "_" + QString::number(additionalSuffix-1) + "." + oldSuff + ")"); // FIXME: Assumes that additionalSuffix is included in file name.
    saveFileNumber ++;
}

bool MainWindow::fileExists(QString path) {
    return QFileInfo::exists(path) && QFileInfo(path).isFile();
}

QDateTime MainWindow::roundToClosestSecond(QDateTime datTimToRound) {
    int msecs = datTimToRound.time().msec();
    int secondsToAdd = 0;
    if(msecs >= 500) {
        secondsToAdd = 1;
    }

    QDateTime timeCopy = datTimToRound.addSecs(secondsToAdd);
    timeCopy = timeCopy.addMSecs(-msecs); // msecs -> 0

    return timeCopy;
}

MainWindow::~MainWindow()
{
    if(isSaving) {
        stopSaving();
    }
    if(isPortOpen) {
        closeSerialPort();
    }
    delete Plot;
    delete ui;
}


void MainWindow::on_actionRefresh_ports_triggered()
{
    refreshSerialPorts();
}


void MainWindow::on_actionSave_as_triggered()
{
    startSaving();
}


void MainWindow::on_actionAll_parameters_triggered()
{
    showDeviceStatus();
}


void MainWindow::on_actionDisconnect_triggered()
{
    closeSerialPort();
}


void MainWindow::on_actionQuick_guide_triggered()
{
    quickguidewindow *guideWindow = new quickguidewindow(this);
    guideWindow->setWindowFlags(Qt::Window);
    guideWindow->setAttribute(Qt::WA_DeleteOnClose);
    guideWindow->show();
}


void MainWindow::on_actionAbout_triggered()
{
    aboutWindow *aboutwindow = new aboutWindow(this);
    aboutwindow->setWindowFlags(Qt::Window);
    aboutwindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutwindow->show();
    //deleteLater();
}


void MainWindow::on_actionLog_triggered()
{
    logwindow *logWindow = new logwindow(this);
    logWindow->refreshTextBox(&allLogs);
    logWindow->setWindowFlags(Qt::Window);
    logWindow->setAttribute(Qt::WA_DeleteOnClose);
    logWindow->show();
    connect(this,SIGNAL(logTextAdded(QString)),logWindow,SLOT(addLogText(QString)));
}


void MainWindow::on_actionSave_log_as_triggered()
{
    if(isLogSaving) {
        isLogSaving = false;
        logOut.flush();
        logSaveFile->flush();
        logSaveFile->close();
        ui->actionSave_log_as->setText("Save log as...");
        return;
    }
    QFileInfo currentFileInfo = QFileInfo(*saveFile);
    QString defaultLogFilename = currentFileInfo.baseName() + ".log";
    QString defaultDirectory = currentFileInfo.absolutePath();

    OQFileDialog *fileDialog = new OQFileDialog(this, "Save log as...",{"Log file (*.log)","All files (*)"},defaultLogFilename,false);
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setDirectory(defaultDirectory);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    if(!fileDialog->exec()) {
        return;
    }

    QString selectedFile = fileDialog->selectedFiles().at(0);
    if(!selectedFile.isEmpty()) {
        logSaveFile = new QFile(selectedFile);

        if(!logSaveFile->open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),logSaveFile->errorString());
            return;
        }
        logOut.setDevice(logSaveFile);

        // Write all logs that already exist:
        for(int i=0; i < allLogs.length(); i++) {
            logOut << allLogs.at(i) << "\r\n";
        }

        logOut.flush();
        logSaveFile->flush();
        isLogSaving = true;
        ui->actionSave_log_as->setText("Stop saving log");
    }
}

