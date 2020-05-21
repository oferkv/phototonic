#include "RangeInputDialog.h"
#include "ui_RangeInputDialog.h"

RangeInputDialog::RangeInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RangeInputDialog)
{
    ui->setupUi(this);
}

RangeInputDialog::~RangeInputDialog()
{
    delete ui;
}

qreal RangeInputDialog::minimumValue()
{
    return qreal(ui->lowerSpinBox->value());
}

qreal RangeInputDialog::maximumValue()
{
    return qreal(ui->upperSpinBox->value());
}
