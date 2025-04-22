#include "mainwindow.h"
#include <QApplication>
#include "keymanager.h"
#include <QDebug>
#include <QByteArray>
#include "qaesencryption.h"
#include <QFile>
//#include <QRandomGenerator>
//#include <QCryptographicHash>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
