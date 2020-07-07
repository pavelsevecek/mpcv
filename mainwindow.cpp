#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "las.h"
#include "mesh.h"
#include "openglwidget.h"
#include "sunwidget.h"
#include "utils.h"
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProgressDialog>
#include <QShortcut>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace Mpcv;

static bool checkMod = true;

std::string findBasename(const QString& file) {
    std::cout << "Finding basename in file '" << file.toStdString() << "'" << std::endl;
    if (file.isEmpty() || file == ".") {
        return {};
    }
    QFileInfo info(file);
    std::string name = info.baseName().toStdString();
    int p1, p2;
    std::cout << "Checking path '" << name << "' for basename" << std::endl;
    if (sscanf(name.c_str(), "0%d-%d", &p1, &p2) == 2) {
        std::cout << "Detected " << name << " as window basename" << std::endl;
        return name;
    }
    if (!info.isRoot()) {
        return findBasename(info.dir().absolutePath());
    } else {
        return "";
    }
}

std::map<std::string, Coords> parseConfig() {
    QFileInfo info(QDir::homePath() + "/.config/mpcv/extents.csv");
    if (!info.exists()) {
        std::cout << "No extents config at '" << info.filePath().toStdString() << "' found" << std::endl;
        return {};
    }
    std::map<std::string, Coords> extents;
    std::ifstream in(info.filePath().toStdString());
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string basename;
        std::getline(ss, basename, ',');

        std::string value;
        std::getline(ss, value, ',');
        double llx = std::stod(value);
        std::getline(ss, value, ',');
        double lly = std::stod(value);
        std::getline(ss, value, ',');
        double urx = std::stod(value);
        double ury;
        ss >> ury;

        extents[basename] = Coords((llx + urx) / 2, (lly + ury) / 2);
    }

    return extents;
}

static std::map<std::string, Coords> config;

void geolocalize(TexturedMesh& mesh, const QString& file) {
    std::string basename = findBasename(QFileInfo(file).absolutePath());
    if (!basename.empty() && config.find(basename) != config.end()) {
        std::cout << "Setting srs to " << config[basename][0] << "," << config[basename][1] << std::endl;
        mesh.srs = Srs(config[basename]);
    } else {
        std::cout << "No srs found in config" << std::endl;
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow) {
    ui_->setupUi(this);

    viewport_ = findChild<OpenGLWidget*>("Viewport");
    viewport_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    list_ = findChild<QListWidget*>("MeshList");

#ifndef ENABLE_MESH_REPAIR
    QMenu* menu = findChild<QMenu*>("menuMesh");
    QAction* repair = findChild<QAction*>("actionRepair");
    menu->removeAction(repair);
#endif

    QShortcut* showAll = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    QObject::connect(showAll, &QShortcut::activated, this, [this] {
        std::cout << "Showing all" << std::endl;
        checkMod = false;
        for (int i = 0; i < list_->count(); ++i) {
            list_->item(i)->setCheckState(Qt::Checked);
        }
        checkMod = true;
    });
    QShortcut* invert = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_I), this);
    QObject::connect(invert, &QShortcut::activated, this, [this] {
        std::cout << "Inverting selection" << std::endl;
        checkMod = false;
        for (int i = 0; i < list_->count(); ++i) {
            list_->item(i)->setCheckState(
                list_->item(i)->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
        }
        checkMod = true;
    });
    for (int key = 0; key < 10; key++) {
        Qt::Key k = Qt::Key((key == 9) ? Qt::Key_0 : (Qt::Key_1 + key));
        QShortcut* showOnly = new QShortcut(QKeySequence(Qt::CTRL + k), this);
        QObject::connect(showOnly, &QShortcut::activated, this, [this, key] {
            std::cout << "Showing only " << key << std::endl;
            checkMod = false;
            if (list_->count() <= key) {
                return;
            }
            for (int i = 0; i < list_->count(); ++i) {
                list_->item(i)->setCheckState(i == key ? Qt::Checked : Qt::Unchecked);
            }
            checkMod = true;
        });

        QShortcut* showToggle = new QShortcut(QKeySequence(Qt::SHIFT + k), this);
        QObject::connect(showToggle, &QShortcut::activated, this, [this, key] {
            std::cout << "Showing toggle " << key << std::endl;
            checkMod = false;
            if (list_->count() <= key) {
                return;
            }
            Qt::CheckState state = list_->item(key)->checkState();
            list_->item(key)->setCheckState(state == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
            checkMod = true;
        });
    }
    auto firstChecked = [this]() -> QListWidgetItem* {
        for (int i = 0; i < list_->count(); ++i) {
            if (list_->item(i)->checkState() == Qt::Checked) {
                return list_->item(i);
            }
        }
        return nullptr;
    };
    QShortcut* showRight = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this);
    QObject::connect(showRight, &QShortcut::activated, this, [this, firstChecked] {
        checkMod = false;
        // first checked item
        QListWidgetItem* item = firstChecked();
        if (item == nullptr) {
            return;
        }
        int currentId = list_->row(item);
        if (currentId < list_->count() - 1) {
            list_->item(currentId)->setCheckState(Qt::Unchecked);
            list_->item(currentId + 1)->setCheckState(Qt::Checked);
        }
        checkMod = true;
    });
    QShortcut* showLeft = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this);
    QObject::connect(showLeft, &QShortcut::activated, this, [this, firstChecked] {
        checkMod = false;
        QListWidgetItem* item = firstChecked();
        if (item == nullptr) {
            return;
        }
        int currentId = list_->row(item);
        if (currentId > 0) {
            list_->item(currentId)->setCheckState(Qt::Unchecked);
            list_->item(currentId - 1)->setCheckState(Qt::Checked);
        }
        checkMod = true;
    });
    QListWidget* list = this->findChild<QListWidget*>("MeshList");
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(list, &QListWidget::customContextMenuRequested, this, [this, list](const QPoint& pos) {
        QMenu submenu;
        QAction* actReload = submenu.addAction("Reload");
        QAction* actClose = submenu.addAction("Close");

        QPoint item = list_->mapToGlobal(pos);
        QAction* clicked = submenu.exec(item);
        if (clicked == actClose) {
            viewport_->deleteMesh(list_->currentItem());
            list_->takeItem(list_->currentIndex().row());
        } else if (clicked == actReload) {
            QString file = list_->currentItem()->data(Qt::UserRole).toString();
            viewport_->deleteMesh(list_->currentItem());
            list_->takeItem(list_->currentIndex().row());
            open(file);
        }
    });

    QShortcut* reloadAll = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    QObject::connect(reloadAll, &QShortcut::activated, this, [this] {
        std::cout << "Reloading all" << std::endl;
        checkMod = false;
        for (QListWidgetItem* item : list_->selectedItems()) {
            QString file = item->data(Qt::UserRole).toString();
            viewport_->deleteMesh(item);
            list_->takeItem(list_->row(item));
            open(file);
        }
        checkMod = true;
    });

    config = parseConfig();
}

MainWindow::~MainWindow() {
    delete ui_;
}


void MainWindow::open(const QString& file, int index, int total) {
    QCoreApplication::processEvents();
    try {
        QString ext = QFileInfo(file).suffix();
        if (ext != "ply" && ext != "obj" && ext != "las" && ext != "laz") {
            QMessageBox box(QMessageBox::Warning, "Error", "Unknown file format of file '" + file + "'");
            box.exec();
            return;
        }

        QString message = "Loading '" + file + "'";
        if (total > 1) {
            message += " (" + QString::number(index) + " of " + QString::number(total) + ")";
        }
        QProgressDialog dialog(message, "Cancel", 0, 100, viewport_);
        dialog.setWindowModality(Qt::WindowModal);
        auto callback = [&dialog](float prog) {
            dialog.setValue(prog);
            QCoreApplication::processEvents();
            return dialog.wasCanceled();
        };

        // Pvl::Optional<Mesh> mesh;
        TexturedMesh mesh;
        if (ext == "ply") {
            std::ifstream in;
            in.exceptions(std::ifstream::badbit | std::ifstream::failbit);
            in.open(file.toStdString());
            // Pvl::Optional<Mesh> loaded
            mesh = loadPly(in, callback);
            geolocalize(mesh, file);
            /*if (!loaded) {
                return;
            }
            mesh = std::move(loaded.value());*/
        } else if (ext == "obj") {
            mesh = loadObj(file, callback);
            geolocalize(mesh, file);
        } else if (ext == "las" || ext == "laz") {
            mesh = loadLas(file.toStdString(), callback);
        }
        dialog.close();
        if (mesh.vertices.empty()) {
            return;
        }

        QFileInfo info(file);
        QString identifier = info.absoluteDir().dirName() + "/" + info.baseName();
        QListWidgetItem* item = new QListWidgetItem(identifier, list_);
        list_->addItem(item);

        viewport_->view(item, std::move(mesh));
        item->setData(Qt::UserRole, file);
        item->setFlags(
            Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

        /// \todo avoid firing signal

        item->setCheckState(Qt::CheckState::Checked);


    } catch (const std::exception& e) {
        QMessageBox box(QMessageBox::Warning, "Error", "Cannot open file '" + file + "'\n" + e.what());
        box.exec();
    }
}

void MainWindow::on_actionOpenFile_triggered() {
    QDir& initialDir = openFileDialogInitialDir();
    QStringList names = QFileDialog::getOpenFileNames(this,
        tr("Open mesh"),
        initialDir.path(),
        tr("all files (*);;.ply object (*.ply);;LAS point cloud (*.las *.laz)"));
    if (!names.empty()) {
        QFileInfo info(names.first());
        initialDir = info.dir();
        int index = 1;
        for (QString name : names) {
            open(name, index, names.size());
        }
    }
}
void MainWindow::on_MeshList_itemChanged(QListWidgetItem* item) {
    static int reentrant = 0;
    if (reentrant > 0) {
        return;
    }
    reentrant++;
    if (checkMod && QApplication::queryKeyboardModifiers() & Qt::CTRL) {
        for (int i = 0; i < list_->count(); ++i) {
            QListWidgetItem* it = list_->item(i);
            it->setCheckState(it == item ? Qt::Checked : Qt::Unchecked);
            viewport_->toggle(it, it == item);
        }
    } else {
        bool on = item->checkState() == Qt::Checked;
        viewport_->toggle(item, on);
    }
    reentrant--;
}

void MainWindow::on_actionShowWireframe_changed() {
    QAction* act = this->findChild<QAction*>("actionShowWireframe");
    viewport_->wireframe(act->isChecked());
}

void MainWindow::on_actionShowDots_changed() {
    QAction* act = this->findChild<QAction*>("actionShowDots");
    viewport_->dots(act->isChecked());
}

void MainWindow::on_actionScreenshot_triggered() {
    QDir& initialDir = openFileDialogInitialDir();
    QString file = QFileDialog::getSaveFileName(this,
        tr("Save screenshot"),
        initialDir.path(),
        tr("PNG image (*.png);;JPEG image (*.jpg);;Targa image (*.tga)"));
    if (!file.isEmpty()) {
        QFileInfo info(file);
        initialDir = info.dir();
        if (info.suffix().isEmpty()) {
            file += ".png";
        }
        viewport_->screenshot(file);
    }
}

void MainWindow::on_actionLaplacian_smoothing_triggered() {
    viewport_->laplacianSmooth();
}

void MainWindow::on_actionRepair_triggered() {
    viewport_->repair();
}

void MainWindow::on_actionSimplify_triggered() {
    viewport_->simplify();
}

void MainWindow::on_actionQuit_triggered() {
    close();
}

void MainWindow::on_actionSave_triggered() {
    QDir& initialDir = saveFileDialogInitialDir();
    QString file =
        QFileDialog::getSaveFileName(this, tr("Save mesh"), initialDir.path(), tr(".ply object (*.ply)"));
    if (!file.isEmpty()) {
        QFileInfo info(file);
        if (info.suffix().isEmpty()) {
            file += ".ply";
        }
        initialDir = info.dir();
        std::vector<const void*> handles;
        for (int i = 0; i < list_->count(); ++i) {
            if (list_->item(i)->checkState() == Qt::Checked) {
                handles.push_back(list_->item(i));
            }
        }
        viewport_->saveAsMesh(file, handles);
    }
}

void MainWindow::buttonPushed(QAction* pushed) {
    QAction* texture = findChild<QAction*>("actionTexture");
    QAction* flat = findChild<QAction*>("actionFlat");
    QAction* ao = findChild<QAction*>("actionAo");
    texture->setEnabled(texture != pushed);
    texture->setChecked(texture == pushed);
    flat->setEnabled(flat != pushed);
    flat->setChecked(flat == pushed);
    ao->setEnabled(ao != pushed);
    ao->setChecked(ao == pushed);
}

void MainWindow::on_actionAo_triggered() {
    QProgressDialog dialog("Computing A0", "Cancel", 0, 100, viewport_);
    dialog.setWindowModality(Qt::WindowModal);
    QCoreApplication::processEvents();
    auto callback = [&dialog](float prog) {
        dialog.setValue(prog);
        QCoreApplication::processEvents();
        return dialog.wasCanceled();
    };
    viewport_->computeAmbientOcclusion(callback);
    buttonPushed(findChild<QAction*>("actionAo"));
}

void MainWindow::on_actionTexture_triggered() {
    viewport_->enableTextures(true);
    buttonPushed(findChild<QAction*>("actionTexture"));
}

void MainWindow::on_actionFlat_triggered() {
    viewport_->enableTextures(false);
    viewport_->enableAo(false);
    buttonPushed(findChild<QAction*>("actionFlat"));
}


void MainWindow::on_actionGrid_triggered() {
    QAction* act = this->findChild<QAction*>("actionGrid");
    viewport_->grid(act->isChecked());
}

void MainWindow::on_actionResetCamera_triggered() {
    viewport_->resetCamera();
}

void MainWindow::on_actionEstimate_normals_triggered() {
    QProgressDialog dialog("Computing normals", "Cancel", 0, 100, viewport_);
    dialog.setWindowModality(Qt::WindowModal);
    QCoreApplication::processEvents();
    auto callback = [&dialog](float prog) {
        dialog.setValue(prog);
        QCoreApplication::processEvents();
        return dialog.wasCanceled();
    };
    viewport_->estimateNormals(callback);
}

void MainWindow::on_actionRender_view_triggered() {
    viewport_->renderView();
}

void MainWindow::on_actionSun_setup_triggered() {
    static SunWidget* sun = new SunWidget(this);
    sun->setFunc([this](Pvl::Vec3f dir) { viewport_->setSunDir(dir); });
    sun->show();
}

void MainWindow::on_actionControls_triggered() {
    std::cout << "Showing help" << std::endl;
    QString text;
    text += "Ctrl+[1-9]   view only n-th mesh\n";
    text += "Shift+[1-9]  toggle visibility of n-th mesh\n";
    text += "Ctrl+I       invert visibility\n";
    text += "Ctrl+A       show all meshes\n";
    text += "Ctrl+Left    show the mesh above the current\n";
    text += "Ctrl+Right   show the mesh below the current\n";
    text += "Ctrl+Click   show only the selected mesh\n";
    text += "\n";
    text += "Ctrl+Mouse wheel    Change the field of view\n";
    text += "Alt+Mouse wheel     Change the size of points\n";
    text += "Shift+Mouse wheel   Change the point stride\n";
    text += "Double click        Center the camera at target\n";
    QMessageBox box(QMessageBox::Information, "Help", text, QMessageBox::Ok, this);
    QFont font = box.font();
    font.setFamily("Courier New");
    box.setFont(font);
    box.exec();
}
