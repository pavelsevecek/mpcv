#include "framebuffer.h"
#include "./ui_framebuffer.h"


FrameBufferWidget::FrameBufferWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::FrameBuffer) {
    ui_->setupUi(this);

    view_ = findChild<View*>("view");
    /*QWidget* layout = findChild<QWidget*>("horizontalLayoutWidget");
    view_ = new View(layout);
    view_->setGeometry(10, 10, width() - 20, height() - 20);
    view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->adjustSize();*/
}

FrameBufferWidget::~FrameBufferWidget() {
    delete ui_;
}

void FrameBufferWidget::on_actionSave_render_triggered() {
    view_->save();
}
