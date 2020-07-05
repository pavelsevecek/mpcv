#include "sunwidget.h"
#include "./ui_sunwidget.h"
#include "sun-sky/SunSky.h"
#include <QDateTimeEdit>
#include <QDoubleSpinBox>

SunWidget::SunWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::SunWidget) {
    ui_->setupUi(this);
}

SunWidget::~SunWidget() {
    delete ui_;
}

void SunWidget::on_pushButton_clicked() {
    QDoubleSpinBox* latitudeSpin = findChild<QDoubleSpinBox*>("latitude");
    QDoubleSpinBox* longitudeSpin = findChild<QDoubleSpinBox*>("longitude");
    QDateTimeEdit* timeEdit = findChild<QDateTimeEdit*>("time");
    float latitude = latitudeSpin->value();
    float longitude = longitudeSpin->value();
    QTime time = timeEdit->time();
    QDate date = timeEdit->date();
    float localTime = time.hour() + time.minute() / 60.f;
    float timezone = longitude / 15.f; /// \todo
    int julian = date.toJulianDay();
    Vec3f dir = SSLib::SunDirection(localTime, timezone, julian, latitude, longitude);
    hide();
    sunDirFunc_(Pvl::normalize(Pvl::Vec3f(dir[0], dir[1], dir[2])));
}
