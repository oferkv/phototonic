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

#include <QtGui>
#include "thumbview.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class MainWindow;

class CpMvDialog : public QDialog
{
    Q_OBJECT

public slots:
	void abort();

public:
    CpMvDialog(QWidget *parent);
	void exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir);
	int nfiles;

private:
	QLabel *opLabel;
	QPushButton *cancelButton;
	QFileInfo *dirInfo;
	bool abortOp;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
	static int const nZoomRadios = 5;
    SettingsDialog(QWidget *parent);

private slots:
	void pickColor();
	void pickThumbsColor();
	void pickThumbsTextColor();

public slots:
	void abort();
	void saveSettings();

private:
	QRadioButton *fitLargeRadios[nZoomRadios];
	QRadioButton *fitSmallRadios[nZoomRadios];
	QCheckBox *compactLayoutCb;
    QPushButton *backgroundColorButton;
    QPushButton *colThumbButton;
    QPushButton *colThumbTextButton;
	QSpinBox *thumbSpacingSpin;
	QColor bgColor;
	QColor thumbBgColor;
	QColor thumbTextColor;
	QCheckBox *exitCliCb;
};

#endif // DIALOGS_H

