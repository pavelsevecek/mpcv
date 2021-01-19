#pragma once

#include "pvl/Vector.hpp"
#include <QFileDialog>
#include <QMainWindow>
#include <QResizeEvent>
#include <functional>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui {
class SunWidget;
}
QT_END_NAMESPACE

namespace Mpcv {
struct RenderSettings;
}

class SunWidget : public QMainWindow {
    Q_OBJECT

public:
    SunWidget(QWidget* parent = nullptr);
    ~SunWidget();

    void setFunc(std::function<void(Mpcv::RenderSettings)> func) {
        sunDirFunc_ = func;
    }

private slots:
    void on_pushButton_clicked();

private:
    Ui::SunWidget* ui_;
    std::function<void(Mpcv::RenderSettings)> sunDirFunc_;
};
