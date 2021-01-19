#include "sunwidget.h"
#include "./ui_sunwidget.h"
#include "renderer.h"
#include "sun-sky/SunSky.h"
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>

SunWidget::SunWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::SunWidget) {
    ui_->setupUi(this);
#ifndef HAS_OIDN
    QCheckBox* checkBox = findChild<QCheckBox*>("denoiseBox");
    checkBox->setEnabled(false);
#endif
}

SunWidget::~SunWidget() {
    delete ui_;
}

void SunWidget::on_pushButton_clicked() {
    Mpcv::RenderSettings settings;
    QSpinBox* widthSpin = findChild<QSpinBox*>("widthSpin");
    QSpinBox* heightSpin = findChild<QSpinBox*>("heightSpin");
    QSpinBox* itersSpin = findChild<QSpinBox*>("itersSpin");
    QCheckBox* checkBox = findChild<QCheckBox*>("denoiseBox");
    settings.resolution = Pvl::Vec2i(widthSpin->value(), heightSpin->value());
    settings.numIters = itersSpin->value();
    settings.denoise = checkBox->checkState() == Qt::Checked;

    QDoubleSpinBox* latitudeSpin = findChild<QDoubleSpinBox*>("latitude");
    QDoubleSpinBox* longitudeSpin = findChild<QDoubleSpinBox*>("longitude");
    QDateTimeEdit* timeEdit = findChild<QDateTimeEdit*>("time");
    float latitude = latitudeSpin->value();
    float longitude = longitudeSpin->value();
    QTime time = timeEdit->time();
    QDate date = timeEdit->date();
    float localTime = time.hour() + time.minute() / 60.f;
    if (timeEdit->dateTime().isDaylightTime()) {
        localTime += 1.f;
    }
    float timezone = longitude / 15.f; /// \todo
    int day = date.dayOfYear();
    std::cout << "localTime = " << localTime << std::endl;
    std::cout << "julianDay = " << day << std::endl;
    Vec3f dir =
        SSLib::SunDirection(localTime, timezone, day, latitude, longitude);
    hide();
    settings.dirToSun = Pvl::normalize(Pvl::Vec3f(dir[0], dir[1], dir[2]));
    sunDirFunc_(settings);
}
