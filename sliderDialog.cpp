#include "sliderDialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

sliderDialog::sliderDialog(std::tuple<int, int, int>& scaling_default, QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    // Настройка минимума и максимума слайдеров

    auto [Nx_sc, Ux_sc, sig_sc] = scaling_default;
    ui.horizontalSlider->setMinimum(1);
    ui.horizontalSlider->setMaximum(150);
    ui.horizontalSlider->setValue(Nx_sc);

    ui.horizontalSlider_2->setMinimum(1);
    ui.horizontalSlider_2->setMaximum(150);
    ui.horizontalSlider_2->setValue(Ux_sc);

    ui.horizontalSlider_3->setMinimum(1);
    ui.horizontalSlider_3->setMaximum(150);
    ui.horizontalSlider_3->setValue(sig_sc);

    // Создание QLabel для отображения значений слайдеров
    labelNxValue = new QLabel(QString::number(Nx_sc), this);
    labelNxValue->setMinimumWidth(40);
    labelNxValue->setStyleSheet("color: blue; font-weight: bold;");
    labelNxValue->setGeometry(360, 50, 25, 21);

    labelUxValue = new QLabel(QString::number(Ux_sc), this);
    labelUxValue->setMinimumWidth(40);
    labelUxValue->setStyleSheet("color: blue; font-weight: bold;");
    labelUxValue->setGeometry(360, 100, 25, 21);

    labelSigmaValue = new QLabel(QString::number(sig_sc), this);
    labelSigmaValue->setMinimumWidth(40);
    labelSigmaValue->setStyleSheet("color: blue; font-weight: bold;");
    labelSigmaValue->setGeometry(360, 150, 25, 21);

    // Подключение сигналов слайдеров к слотам
    connect(ui.horizontalSlider, QOverload<int>::of(&QSlider::valueChanged),
        this, &sliderDialog::onSlider1ValueChanged);

    connect(ui.horizontalSlider_2, QOverload<int>::of(&QSlider::valueChanged),
        this, &sliderDialog::onSlider2ValueChanged);

    connect(ui.horizontalSlider_3, QOverload<int>::of(&QSlider::valueChanged),
        this, &sliderDialog::onSlider3ValueChanged);

    // Подключение кнопки подтверждения
    connect(ui.pushButton_scale, &QPushButton::clicked,
        this, &sliderDialog::onConfirmButtonClicked);
}

sliderDialog::~sliderDialog()
{
}

void sliderDialog::onSlider1ValueChanged(int value)
{
    labelNxValue->setText(QString::number(value));
}

void sliderDialog::onSlider2ValueChanged(int value)
{
    labelUxValue->setText(QString::number(value));
}

void sliderDialog::onSlider3ValueChanged(int value)
{
    labelSigmaValue->setText(QString::number(value));
}

void sliderDialog::onConfirmButtonClicked()
{
    int nxVal = ui.horizontalSlider->value();
    int uxVal = ui.horizontalSlider_2->value();
    int sigmaVal = ui.horizontalSlider_3->value();

    emit scalesConfirmed(nxVal, uxVal, sigmaVal);
    hide();
}

int sliderDialog::getNxValue() const
{
    return ui.horizontalSlider->value();
}

int sliderDialog::getUxValue() const
{
    return ui.horizontalSlider_2->value();
}

int sliderDialog::getSigmaValue() const
{
    return ui.horizontalSlider_3->value();
}
