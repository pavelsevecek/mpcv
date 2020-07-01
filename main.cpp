#include "mainwindow.h"
#include <QStyleFactory>
#include <iostream>

#include <QApplication>

void setPalette(QApplication& a) {
    a.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    a.setPalette(darkPalette);

    a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    setPalette(a);

    MainWindow w;
#ifdef NDEBUG
    w.setWindowTitle(QString("Mesa build ") + __DATE__ + " " + __TIME__);
#else
    w.setWindowTitle(QString("Mesa DEBUG BUILD ") + __DATE__ + " " + __TIME__);
#endif
    w.showMaximized();
    QStringList files = a.arguments();
    for (int i = 1; i < files.size(); ++i) {
        w.open(files.at(i));
    }
    return a.exec();
}
