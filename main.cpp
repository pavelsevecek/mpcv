#include "mainwindow.h"
#include <iostream>

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    QStringList files = a.arguments();
    for (int i = 1; i < files.size(); ++i) {
        w.open(files.at(i));
    }
    return a.exec();
}
