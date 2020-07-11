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
    if (argc == 2 && (argv[1] == std::string("-h") || argv[1] == std::string("--help"))) {
        std::cout << "Mesh and Point Cloud Viewer" << std::endl;
        std::cout << "Usage: mpcv [FILE]..." << std::endl;
        return 0;
    }

    QApplication a(argc, argv);
    setlocale(LC_NUMERIC, "C"); // needed for sscanf
    setPalette(a);

    MainWindow w;
#ifdef NDEBUG
    w.setWindowTitle(QString("MPCV build ") + __DATE__ + " " + __TIME__);
#else
    w.setWindowTitle(QString("MPCV DEBUG BUILD ") + __DATE__ + " " + __TIME__);
#endif
    w.showMaximized();

    QStringList args = a.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (!w.open(args.at(i), i, args.size() - 1)) {
            // cancelled, skip the rest
            break;
        }
    }
    return a.exec();
}
