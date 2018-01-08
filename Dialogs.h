/*
 *  Copyright (C) 2013 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DIALOGS_H
#define DIALOGS_H

#include <QtWidgets>
#include "ThumbsViewer.h"
#include "ImageViewer.h"

int copyMoveFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir);

class CopyMoveDialog : public QDialog {
Q_OBJECT

public slots:

    void abort();

public:
    CopyMoveDialog(QWidget *parent);

    void exec(ThumbsViewer *thumbView, QString &destDir, bool pasteInCurrDir);

    int nFiles;
    int latestRow;

private:
    QLabel *opLabel;
    QPushButton *cancelButton;
    bool abortOp;
};

class SettingsDialog : public QDialog {
Q_OBJECT

public:
    static int const nZoomRadios = 5;

    SettingsDialog(QWidget *parent);

private slots:

    void pickColor();

    void pickThumbsColor();

    void pickThumbsTextColor();

    void pickStartupDir();

    void pickBgImage();

public slots:

    void abort();

    void saveSettings();

private:
    QRadioButton *fitLargeRadios[nZoomRadios];
    QRadioButton *fitSmallRadios[nZoomRadios];
    QToolButton *backgroundColorButton;
    QToolButton *thumbsColorPickerButton;
    QToolButton *thumbsLabelColorButton;
    QSpinBox *thumbPagesSpinBox;
    QSpinBox *saveQualitySpinBox;
    QColor imageViewerBackgroundColor;
    QColor thumbsBackgroundColor;
    QColor thumbsTextColor;
    QCheckBox *exitCliCheckBox;
    QCheckBox *wrapListCheckBox;
    QCheckBox *enableAnimCheckBox;
    QCheckBox *enableExifCheckBox;
    QCheckBox *enableThumbExifCheckBox;
    QCheckBox *imageInfoCheckBox;
    QCheckBox *reverseMouseCheckBox;
    QCheckBox *deleteConfirmCheckBox;
    QSpinBox *slideDelaySpinBox;
    QCheckBox *slideRandomCheckBox;
    QRadioButton *startupDirectoryRadioButtons[3];
    QLineEdit *startupDirLineEdit;
    QLineEdit *thumbsBackgroundImageLineEdit;

    void setButtonBgColor(QColor &color, QToolButton *button);
};

class CropDialog : public QDialog {
Q_OBJECT

public:
    CropDialog(QWidget *parent, ImageViewer *imageViewer);

public slots:

    void ok();

    void reset();

    void applyCrop(int);

private:
    QSpinBox *topSpinBox;
    QSpinBox *bottomSpinBox;
    QSpinBox *leftSpinBox;
    QSpinBox *rightSpinBox;
    ImageViewer *imageViewer;
};

class ResizeDialog : public QDialog {
Q_OBJECT

public:
    ResizeDialog(QWidget *parent, ImageViewer *imageViewer);

public slots:

    void ok();

    void abort();

    void setAspectLock();

    void setUnits();

    void adjustSizes();

private:
    int width;
    int height;
    int lastWidth;
    int lastHeight;
    bool aspectLocked;
    bool pixelUnits;
    int newWidth;
    int newHeight;

    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QRadioButton *pixelsRadioButton;
    QRadioButton *percentRadioButton;
    QLabel *newSizePixelsLabel;
    ImageViewer *imageViewer;
};

class ColorsDialog : public QDialog {
Q_OBJECT

public:
    ColorsDialog(QWidget *parent, ImageViewer *imageViewer);

public slots:

    void ok();

    void reset();

    void enableColorize(int state);

    void redNegative(int state);

    void greenNegative(int state);

    void blueNegative(int state);

    void setRedChannel();

    void setGreenChannel();

    void setBlueChannel();

    void applyColors(int value);

private:
    ImageViewer *imageViewer;
    QSlider *hueSlider;
    QCheckBox *colorizeCheckBox;
    QSlider *saturationSlider;
    QSlider *lightnessSlider;
    QCheckBox *redCheckBox;
    QCheckBox *greenCheckBox;
    QCheckBox *blueCheckBox;
    QSlider *brightSlider;
    QSlider *contrastSlider;
    QSlider *redSlider;
    QSlider *greenSlider;
    QSlider *blueSlider;
    QCheckBox *rNegateCheckBox;
    QCheckBox *gNegateCheckBox;
    QCheckBox *bNegateCheckBox;
};

class AppMgmtDialog : public QDialog {
Q_OBJECT

public:
    AppMgmtDialog(QWidget *parent);

public slots:

    void ok();

private slots:

    void add();

    void remove();

    void entry();

private:
    QTableView *appsTable;
    QStandardItemModel *appsTableModel;

    void addTableModelItem(QStandardItemModel *model, QString &key, QString &val);
};

class CopyMoveToDialog : public QDialog {
Q_OBJECT

public:
    CopyMoveToDialog(QWidget *parent, QString thumbsPath, bool move);

    QString selectedPath;
    bool copyOp;

private slots:

    void copyOrMove();

    void justClose();

    void add();

    void remove();

    void selection(const QItemSelection &, const QItemSelection &);

    void pathDoubleClick(const QModelIndex &idx);

private:
    QTableView *pathsTable;
    QStandardItemModel *pathsTableModel;
    QString currentPath;
    QLabel *destinationLabel;

    void savePaths();
};

class ProgressDialog : public QDialog {
Q_OBJECT

public slots:

    void abort();

public:
    QLabel *opLabel;
    bool abortOp;

    ProgressDialog(QWidget *parent);

private:
    QPushButton *cancelButton;
};

#endif // DIALOGS_H

