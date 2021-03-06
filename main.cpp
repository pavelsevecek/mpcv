#include "mainwindow.h"
#include "parameters.h"
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

void parseArgument(const std::string& arg, const std::string& param) {
    if (arg == "--extents") {
        auto extents = Mpcv::parseExtents(param);
        std::cout << "Setting point extents to " << extents.lower()[0] << "," << extents.lower()[1] << ":"
                  << extents.upper()[0] << "," << extents.upper()[1] << std::endl;
        Mpcv::Parameters::global().extents = extents;
    } else if (arg == "--stride") {
        int stride = std::stoi(param);
        std::cout << "Setting point stride to " << stride << std::endl;
        Mpcv::Parameters::global().pointStride = stride;
    } else if (arg == "--subset") {
        if (param == "street") {
            Mpcv::Parameters::global().subset = Mpcv::CloudSubset::STREET_ONLY;
        } else if (param == "aerial") {
            Mpcv::Parameters::global().subset = Mpcv::CloudSubset::AERIAL_ONLY;
        } else {
            std::cout << "Unknown subset type, expected 'street' or 'aerial'" << std::endl;
            exit(-1);
        }
    } else if (arg == "--textureScale") {
        float scale = std::stof(param);
        std::cout << "Setting texture scale to " << scale << std::endl;
        Mpcv::Parameters::global().textureScale = scale;
    } else if (arg == "--dsmResolution") {
        int res = std::stoi(param);
        std::cout << "Setting DSM resolution " << res << std::endl;
        Mpcv::Parameters::global().dsmResolution = res;
    } else {
        std::cout << "Unknown parameter '" << arg << "'" << std::endl;
        exit(-1);
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (argv[1] == std::string("-h") || argv[1] == std::string("--help"))) {
        std::cout << "Mesh and Point Cloud Viewer" << std::endl;
        std::cout << "Usage: mpcv [OPTIONS] [FILE]..." << std::endl << std::endl;
        std::cout << "Available parameters:" << std::endl;
        std::cout << "--extents llx,lly:urx,ury     Specifies filtering extents for any loaded point cloud"
                  << std::endl;
        std::cout << "--stride n                    Loads only every n-th point for each point cloud"
                  << std::endl;
        std::cout << "--subset [street,aerial]      Loads only a specific category of points" << std::endl;
        std::cout << "--textureScale f              Resizes the loaded textures by given factor" << std::endl;
        std::cout << "--dsmResolution n             Resolution of the loaded GeoTIFF DSMs" << std::endl;
        return 0;
    }

    QApplication a(argc, argv);
    setlocale(LC_NUMERIC, "C"); // needed for sscanf
    setPalette(a);

    QStringList args = a.arguments();
    std::vector<QString> files;
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args.at(i);
        if (arg.size() > 2 && arg.left(2) == "--") {
            if (i == args.size() - 1) {
                std::cout << "Missing parameter of '" << arg.toStdString() << "'" << std::endl;
                exit(-1);
            }
            parseArgument(arg.toStdString(), args.at(i + 1).toStdString());
            i++;
        } else {
            files.push_back(arg);
        }
    }

    MainWindow w;
#ifdef NDEBUG
    w.setWindowTitle(QString("MPCV build ") + __DATE__ + " " + __TIME__);
#else
    w.setWindowTitle(QString("MPCV DEBUG BUILD ") + __DATE__ + " " + __TIME__);
#endif
    w.showMaximized();

    w.openAll(files);
    return a.exec();
}
