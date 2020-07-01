#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "mesh.h"
#include "openglwidget.h"
//#include "pvl/PlyReader.hpp"
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProgressDialog>
#include <QShortcut>
#include <fstream>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    QWidget* viewport = this->findChild<QWidget*>("Viewport");
    viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    /*if (viewport) {
        std::cout << "viewport found" << std::endl;
        timer.setInterval(100);
        // timer.setSingleShot(false);
        timer.callOnTimeout([viewport] {
            viewport->update();
            // std::cout << "Timer timeout" << std::endl;
        });
        timer.start(100);
    } else {
        std::cout << "NO VIEWPORT" << std::endl;
    }*/

    /*QShortcut* flipNormals = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
    QObject::connect(flipNormals, &QShortcut::activated, this, [this] {
        OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
        static bool outward = true;
        viewport->orientation(outward);
        outward = !outward;
    });*/

    for (int key = 0; key < 10; key++) {
        Qt::Key k = Qt::Key((key == 9) ? Qt::Key_0 : (Qt::Key_1 + key));
        QShortcut* showOnly = new QShortcut(QKeySequence(Qt::CTRL + k), this);
        QObject::connect(showOnly, &QShortcut::activated, this, [this, key] {
            std::cout << "Showing only " << key << std::endl;
            QListWidget* list = this->findChild<QListWidget*>("MeshList");
            if (list->count() <= key) {
                return;
            }
            for (int i = 0; i < list->count(); ++i) {
                list->item(i)->setCheckState(i == key ? Qt::Checked : Qt::Unchecked);
            }
        });

        QShortcut* showToggle = new QShortcut(QKeySequence(Qt::SHIFT + k), this);
        QObject::connect(showToggle, &QShortcut::activated, this, [this, key] {
            std::cout << "Showing toggle " << key << std::endl;
            QListWidget* list = this->findChild<QListWidget*>("MeshList");
            if (list->count() <= key) {
                return;
            }
            Qt::CheckState state = list->item(key)->checkState();
            list->item(key)->setCheckState(state == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
        });
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::open(const QString& file) {
    QCoreApplication::processEvents();

    try {
        std::ifstream in;
        in.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        /*if (!in) {
            throw std::runtime_error("Cannot open file " + file.toStdString());
        }*/
        in.open(file.toStdString());
        // Pvl::PlyReader reader(in);
        QProgressDialog dialog("Loading '" + file + "'", "Cancel", 0, 100);
        dialog.setWindowModality(Qt::WindowModal);
        Pvl::Optional<Mesh> mesh = loadPly(in, [&dialog](float prog) {
            dialog.setValue(prog);
            return dialog.wasCanceled();
        });
        dialog.close();
        if (mesh) {
            QListWidget* list = this->findChild<QListWidget*>("MeshList");
            QFileInfo info(file);
            QString identifier = info.absoluteDir().dirName() + "/" + info.baseName();
            QListWidgetItem* item = new QListWidgetItem(identifier, list);
            list->addItem(item);

            OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
            viewport->view(item, std::move(mesh.value()));

            /// \todo avoid firing signal
            item->setCheckState(Qt::CheckState::Checked);
        }
    } catch (const std::exception& e) {
        QMessageBox box(QMessageBox::Warning, "Error", "Cannot open file '" + file + "'\n" + e.what());
        box.exec();
    }
}

void MainWindow::on_actionOpenFile_triggered() {
    QStringList names = QFileDialog::getOpenFileNames(this, tr("Open mesh"), ".", tr(".ply object (*.ply)"));
    if (!names.empty()) {
        for (QString name : names) {
            open(name);
        }
    }
}
void MainWindow::on_MeshList_itemChanged(QListWidgetItem* item) {
    static int reentrant = 0;
    if (reentrant > 0) {
        return;
    }
    reentrant++;
    OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
    if (QApplication::queryKeyboardModifiers() & Qt::CTRL) {
        QListWidget* list = item->listWidget();
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem* it = list->item(i);
            it->setCheckState(it == item ? Qt::Checked : Qt::Unchecked);
            viewport->toggle(it, it == item);
        }
    } else {
        bool on = item->checkState() == Qt::Checked;
        viewport->toggle(item, on);
    }
    reentrant--;
}

void MainWindow::on_actionShowWireframe_changed() {
    QAction* act = this->findChild<QAction*>("actionShowWireframe");
    OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
    viewport->wireframe(act->isChecked());
}

void MainWindow::on_actionShowDots_changed() {
    QAction* act = this->findChild<QAction*>("actionShowDots");
    OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
    viewport->dots(act->isChecked());
}
