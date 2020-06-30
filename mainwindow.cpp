#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "openglwidget.h"
#include "pvl/PlyReader.hpp"
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
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
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::open(const QString& file) const {
    QListWidget* list = this->findChild<QListWidget*>("MeshList");
    QFileInfo info(file);
    QListWidgetItem* item = new QListWidgetItem(info.dir().dirName() + "/" + info.baseName(), list);
    item->setCheckState(Qt::CheckState::Checked);
    list->addItem(item);

    std::ifstream in(file.toStdString());
    Pvl::PlyReader reader(in);
    Pvl::TriangleMesh<Pvl::Vec3f> mesh = reader.readMesh();
    OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
    viewport->view(item, std::move(mesh));
}

void MainWindow::on_actionOpen_triggered() {
    QString name = QFileDialog::getOpenFileName(
        this, tr("Open mesh"), "/home/pavel/projects/pvl/data/", tr(".ply object (*.ply)"));
    std::cout << "Opened name = " << name.toStdString() << std::endl;
    if (!name.isEmpty()) {
        open(name);
    }
}

void MainWindow::on_MeshList_itemChanged(QListWidgetItem* item) {
    std::cout << "Item check state = " << item->checkState() << std::endl;
    bool on = item->checkState() == Qt::Checked;
    OpenGLWidget* viewport = this->findChild<OpenGLWidget*>("Viewport");
    viewport->toggle(item, on);
}
