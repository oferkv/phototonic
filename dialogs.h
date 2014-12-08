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
#include "thumbview.h"
#include "imageview.h"

int cpMvFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir);

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

class ShortcutsTableView : public QTableView
{
	Q_OBJECT

public:
	ShortcutsTableView();
	void addRow(QString action, QString shortcut);

public slots:
	void showShortcutsTableMenu(QPoint pt);
	void clearShortcut();

protected:
	void keyPressEvent(QKeyEvent *e);

private:
	QStandardItemModel *keysModel;
	QModelIndex selectedEntry;
	QMenu *shortcutsMenu;
	QAction *clearAction;
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
	void pickStartupDir();
	void pickBgImage();

public slots:
	void abort();
	void saveSettings();

private:
	QRadioButton *fitLargeRadios[nZoomRadios];
	QRadioButton *fitSmallRadios[nZoomRadios];
	QCheckBox *compactLayoutCb;
	QToolButton *backgroundColorButton;
    QToolButton *colThumbButton;
    QToolButton *colThumbTextButton;
	QSpinBox *thumbSpacingSpin;
	QSpinBox *thumbPagesSpin;
	QSpinBox *saveQualitySpin;
	QColor bgColor;
	QColor thumbBgColor;
	QColor thumbTextColor;
	QCheckBox *exitCliCb;
	QCheckBox *wrapListCb;
	QCheckBox *enableAnimCb;
	QCheckBox *enableExifCb;
	QCheckBox *enableThumbExifCb;
	QCheckBox *imageInfoCb;
	QCheckBox *noSmallThumbCb;
	QCheckBox *reverseMouseCb;
	QSpinBox *slideDelaySpin;
	QCheckBox *slideRandomCb;
	QRadioButton *startupDirRadios[3];
	QLineEdit *startupDirEdit;
	QLineEdit *thumbsBackImageEdit;

	void setButtonBgColor(QColor &color, QToolButton *button);
};

class CropDialog : public QDialog
{
    Q_OBJECT

public:
    CropDialog(QWidget *parent, ImageView *imageView);

public slots:
	void ok();
	void reset();
	void applyCrop(int);

private:
	QSpinBox *topSpin;
	QSpinBox *bottomSpin;
	QSpinBox *leftSpin;
	QSpinBox *rightSpin;
	ImageView *imageView;
};

class ResizeDialog : public QDialog
{
    Q_OBJECT

public:
    ResizeDialog(QWidget *parent, ImageView *imageView);

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
	
	QSpinBox *widthSpin;
	QSpinBox *heightSpin;
	QRadioButton *pixelsRadio;
	QRadioButton *percentRadio;
	QLabel *newSizePixelsLab;
	ImageView *imageView;
};

class ColorsDialog : public QDialog
{
    Q_OBJECT

public:
    ColorsDialog(QWidget *parent, ImageView *imageView);

public slots:
	void ok();
	void reset();
	void enableColorize(int state);
	void setRedChannel();
	void setGreenChannel();
	void setBlueChannel();
	void applyColors(int value);

private:
	ImageView *imageView;
	QSlider *hueSlide;
	QCheckBox *colorizeCb;
	QSlider *saturationSlide;
	QSlider *lightnessSlide;
	QCheckBox *redB;
	QCheckBox *greenB;
	QCheckBox *blueB;
	QSlider *brightSlide;
	QSlider *contrastSlide;
	QSlider *redSlide;
	QSlider *greenSlide;
	QSlider *blueSlide;
};

class AppMgmtDialog : public QDialog
{
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

class CopyMoveToDialog : public QDialog
{
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
	void selection(const QItemSelection&, const QItemSelection&);
	void pathDoubleClick(const QModelIndex &idx);

private:
	QTableView *pathsTable;
	QStandardItemModel *pathsTableModel;
	QString currentPath;
	QLabel *destinationLab;

	void savePaths();
};

#endif // DIALOGS_H

