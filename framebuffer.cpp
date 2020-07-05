#include "framebuffer.h"
#include "./ui_framebuffer.h"
#include <QProgressBar>
#include <QTimer>
#include <tbb/tbb.h>

struct TaskGroup {
    tbb::task_group group;
    tbb::mutex mutex;
};

void View::paintEvent(QPaintEvent*) {
    QImage image;
    {
        tbb::mutex::scoped_lock lock(tg_->mutex);
        Pvl::Vec2i dims = fb_.dimension();
        image = QImage(dims[0], dims[1], QImage::Format_RGB888);
        image.fill(QColor(0, 0, 0));
        for (int y = 0; y < dims[1]; ++y) {
            for (int x = 0; x < dims[0]; ++x) {
                Pvl::Vec2i pix(x, y);
                Color color = fb_(pix).get(exposure_);
                image.setPixelColor(x, y, QColor(color[0], color[1], color[2]));
            }
        }
    }
    // image.save("render-" + QString::number(pass) + ".png");
    QRect targetRect = rect();
    QRect sourceRect = image.rect();
    float targetAspect = float(targetRect.width()) / targetRect.height();
    float sourceAspect = float(sourceRect.width()) / sourceRect.height();
    if (targetAspect > sourceAspect) {
        int newWidth = sourceRect.width() * float(targetRect.height()) / sourceRect.height();
        int newX = (targetRect.width() - newWidth) / 2;
        targetRect.setX(newX);
        targetRect.setWidth(newWidth);
    } else {
        int newHeight = sourceRect.height() * float(targetRect.width()) / sourceRect.width();
        int newY = (targetRect.height() - newHeight) / 2;
        targetRect.setY(newY);
        targetRect.setHeight(newHeight);
    }
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0));
    painter.drawImage(targetRect, image, sourceRect);
    painter.end();
}

void View::setTaskGroup(std::shared_ptr<TaskGroup> tg) {
    tg_ = tg;
}

void View::setImage(FrameBuffer&& image) {
    {
        tbb::mutex::scoped_lock lock(tg_->mutex);
        fb_ = std::move(image);
    }
    update();
}

void View::setExposure(int exposure) {
    exposure_ = std::pow(2.f, float(exposure - 50.f) / 10.f);
    update();
}

void View::save() {
    static QDir initialDir(".");
    QString file = QFileDialog::getSaveFileName(this,
        tr("Save render"),
        initialDir.path(),
        tr("PNG image (*.png);;JPEG image (*.jpg);;Targa image (*.tga)"));
    if (!file.isEmpty()) {
        QFileInfo info(file);
        initialDir = info.dir();
        if (info.suffix().isEmpty()) {
            file += ".png";
        }
        /// \todo deduplicate
        QImage image;
        {
            tbb::mutex::scoped_lock lock(tg_->mutex);
            Pvl::Vec2i dims = fb_.dimension();
            image = QImage(dims[0], dims[1], QImage::Format_RGB888);
            image.fill(QColor(0, 0, 0));
            for (int y = 0; y < dims[1]; ++y) {
                for (int x = 0; x < dims[0]; ++x) {
                    Pvl::Vec2i pix(x, y);
                    Color color = fb_(pix).get(exposure_);
                    image.setPixelColor(x, y, QColor(color[0], color[1], color[2]));
                }
            }
        }
        image.save(file);
    }
}

FrameBufferWidget::FrameBufferWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::FrameBuffer) {
    ui_->setupUi(this);

    view_ = findChild<View*>("view");
    progressBar_ = findChild<QProgressBar*>("progress");
    /*QWidget* layout = findChild<QWidget*>("horizontalLayoutWidget");
    view_ = new View(layout);
    view_->setGeometry(10, 10, width() - 20, height() - 20);
    view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->adjustSize();*/

    tg_ = std::make_shared<TaskGroup>();
    view_->setTaskGroup(tg_);

    QTimer* timer = new QTimer(this);
    timer->start(200);
    timer->callOnTimeout([this] { progressBar_->setValue(progressValue_); });
}

FrameBufferWidget::~FrameBufferWidget() {
    delete ui_;
}

void FrameBufferWidget::setProgress(const float prog) {
    progressValue_ = prog;
}
void FrameBufferWidget::run(const std::function<void()>& func) {
    tg_->group.run(func);
}

void FrameBufferWidget::on_actionSave_render_triggered() {
    view_->save();
}

void FrameBufferWidget::on_horizontalSlider_valueChanged(int value) {
    view_->setExposure(value);
}
