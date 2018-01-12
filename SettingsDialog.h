/*
 *  Copyright (C) 2013-2018 Ofer Kashayov <oferkv@live.com>
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

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QtWidgets/QtWidgets>
#include "Settings.h"
#include "ShortcutsTable.h"

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

#endif // SETTINGS_DIALOG_H

