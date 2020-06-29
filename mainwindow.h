#pragma once

#include <QMainWindow>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QListWidgetItem;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    /*virtual void resizeEvent(QResizeEvent* evt) override {
        QWidget* viewport = this->findChild<QWidget*>("Viewport");
        viewport->resize(evt->size() - QSize(10, 10));
    }*/

    void open(const QString& file) const;

private slots:
    void on_actionOpen_triggered();

    void on_MeshList_itemChanged(QListWidgetItem* item);

private:
    Ui::MainWindow* ui;
};
