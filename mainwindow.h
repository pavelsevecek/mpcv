#pragma once

#include <QMainWindow>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QListWidgetItem;
QT_END_NAMESPACE

class OpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    /*virtual void resizeEvent(QResizeEvent* evt) override {
        QWidget* viewport = this->findChild<QWidget*>("Viewport");
        viewport->resize(evt->size() - QSize(10, 10));
    }*/

    void open(const QString& file);

private slots:
    void on_MeshList_itemChanged(QListWidgetItem* item);

    void on_actionOpenFile_triggered();

    void on_actionShowWireframe_changed();

    void on_actionShowDots_changed();

    void on_actionScreenshot_triggered();

    void on_actionLaplacian_smoothing_triggered();

    void on_actionRepair_triggered();

    void on_actionSimplify_triggered();

    void on_actionQuit_triggered();

    void on_actionSave_triggered();

private:
    Ui::MainWindow* ui_;
    OpenGLWidget* viewport_;
    std::vector<QListWidgetItem*> itemsA, itemsB;
};
