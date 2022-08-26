#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set the application information in order to use QSettings:
    a.setApplicationName("noxLogger");
    a.setOrganizationName("tuni");
    a.setOrganizationDomain("tuni.fi");

    MainWindow w;
    w.setWindowTitle("NoxLogger");
    w.show();
    return a.exec();
}
