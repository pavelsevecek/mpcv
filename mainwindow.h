#pragma once

#include <QMainWindow>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

class OpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void open(const QString& file, int index = 1, int total = 1);

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

    void on_actionAo_triggered();

    void on_actionGrid_triggered();

    void on_actionTexture_triggered();

    void on_actionFlat_triggered();

    void on_actionResetCamera_triggered();

    void on_actionEstimate_normals_triggered();

    void on_actionRender_view_triggered();

    void on_actionSun_setup_triggered();

    void on_actionControls_triggered();

private:
    Ui::MainWindow* ui_;
    OpenGLWidget* viewport_;
    QListWidget* list_;
    std::vector<QListWidgetItem*> itemsA, itemsB;

    void buttonPushed(QAction* pushed);
};
