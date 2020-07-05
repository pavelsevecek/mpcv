#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QPainter>
#include <QResizeEvent>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui {
class FrameBuffer;
}
QT_END_NAMESPACE

class View : public QWidget {
public:
    View(QWidget* parent)
        : QWidget(parent) {
        image_ = QImage(width() - 20, height() - 20, QImage::Format_RGB888);
        image_.fill(QColor(0, 0, 0));
    }

    virtual void paintEvent(QPaintEvent*) override {
        std::cout << "Rendering image to the view" << std::endl;
        QRect targetRect = geometry();
        QRect sourceRect = image_.rect();
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
        painter.drawImage(targetRect, image_, sourceRect);
        painter.end();
    }

    void setImage(QImage&& image) {
        image_ = std::move(image);
        update();
    }

    void save() {
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
            image_.save(file);
        }
    }

private:
    QImage image_;
};

class FrameBufferWidget : public QMainWindow {
    Q_OBJECT

public:
    FrameBufferWidget(QWidget* parent = nullptr);
    ~FrameBufferWidget();

    void setImage(QImage&& image) {
        view_->setImage(std::move(image));
    }

private slots:


    void on_actionSave_render_triggered();

private:
    Ui::FrameBuffer* ui_;
    View* view_;
};
