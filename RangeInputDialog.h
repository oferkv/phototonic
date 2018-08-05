#ifndef RANGEINPUTDIALOG_H
#define RANGEINPUTDIALOG_H

#include <QDialog>

namespace Ui {
class RangeInputDialog;
}

class RangeInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RangeInputDialog(QWidget *parent = nullptr);
    ~RangeInputDialog();

    qreal minimumValue();
    qreal maximumValue();

private:
    Ui::RangeInputDialog *ui;
};

#endif // RANGEINPUTDIALOG_H
