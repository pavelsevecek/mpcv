#pragma once

#include "pvl/UniformGrid.hpp"
#include <QFileDialog>
#include <QMainWindow>
#include <QPainter>
#include <QResizeEvent>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class FrameBuffer;
}
class QProgressBar;
QT_END_NAMESPACE

struct TaskGroup;

using Image = Pvl::UniformGrid<Pvl::Vec3f, 2>;

class View : public QWidget {
public:
    View(QWidget* parent)
        : QWidget(parent) {}

    virtual void paintEvent(QPaintEvent*) override;

    void setTaskGroup(std::shared_ptr<TaskGroup> tg);

    void setImage(Image&& image);

    void setExposure(int exposure);

    void save();

private:
    Image image_;
    std::shared_ptr<TaskGroup> tg_;
    float exposure_ = 1.f;
};


class FrameBufferWidget : public QMainWindow {
    Q_OBJECT

public:
    FrameBufferWidget(QWidget* parent = nullptr);
    ~FrameBufferWidget();

    void setImage(Image&& image) {
        view_->setImage(std::move(image));
    }

    bool cancelled() const {
        return cancelled_;
    }

    void setProgress(int pass, int prog);

    void setNumIters(int numIters);

    void run(const std::function<void()>& func);

private slots:
    void on_actionSave_render_triggered();

    void on_horizontalSlider_valueChanged(int value);

    void on_actionClose_triggered();

private:
    Ui::FrameBuffer* ui_;
    View* view_;
    QProgressBar* progressBar_;
    QProgressBar* iterationBar_;

    int passValue_ = 0;
    int progressValue_ = 0;
    std::shared_ptr<TaskGroup> tg_;
    bool cancelled_ = false;
};
