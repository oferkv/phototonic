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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeView>
#include <QSplitter>
#include <QCompleter>
#include <QSettings>
#include <QStackedWidget>
#include "thumbview.h"
#include "imageview.h"

class QAction;
class QActionGroup;
class QLabel;

class Phototonic : public QMainWindow
{
	Q_OBJECT

public:
	Phototonic(QWidget *parent = 0);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	int copyCutCount;

protected:
	void closeEvent(QCloseEvent *event);

signals:
	void abortThumbLoading();

public slots:
	void unsetBusy();
	void updateState(QString state);
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);
	void loadImageFile(QString imageFileName);
	void loadImagefromThumb(const QModelIndex &idx);
	void loadImagefromCli();
	void closeImage();

private slots:
	void about();
	void setSortFlags();
	void refreshThumbs();
	void refreshThumbs(bool noScroll);
	void setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag);
	void showSettings();
	void toggleFullScreen();
	void updateActions(QWidget *old, QWidget *now);
	void changeActionsBySelection(const QItemSelection&, const QItemSelection&);
	void reloadThumbsSlot();
	void renameDir();
	void rename();
	void deleteDir();
	void manageDir();
	void createSubDirectory();
	void checkDirState(const QModelIndex &, int, int);
	void goSelectedDir(const QModelIndex &currDir);
	void goPathBarDir();
	void goBack();
	void goTo(QString path);
	void goForward();
	void goUp();
	void goHome();
	void loadNextImage();
	void loadPrevImage();
	void selectAllThumbs();
	void deleteOp();
	void cutImages();
	void copyImages();
	void pasteImages();
	void thumbsZoomIn();
	void thumbsZoomOut();
	void setThumbViewWidgetsVisible(bool visible);
	void goTop();
	void goBottom();
	void setClassicThumbs();
	void setCompactThumbs();
	void setSquarishThumbs();

private:
	void restoreCurrentIdx();
	bool handleArgs();
	void createImageView();
	void createThumbView();
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void createFSTree();
	void writeSettings();
	void loadDefaultSettings();
	void recordHistory(QString dir);
	bool isValidPath(QString &path);
	bool isBusy()
	{
		return thumbViewBusy;
	}
	void abortThumbsLoad();
	QString getSelectedPath();
	void createCopyCutFileList();
	void setCopyCutActions(bool setEnabled);

	QString cliFileName;
	bool cliImageLoaded;
	bool thumbViewBusy;
	QMenuBar *thumbsMenuBar;
	QMenu *fileMenu;
	QMenu *editMenu;
	QMenu *goMenu;
	QMenu *sortMenu;
	QMenu *viewMenu;
	QMenu *helpMenu;

	QToolBar *viewToolBar;
	QToolBar *editToolBar;
	QToolBar *goToolBar;

	QAction *exitAction;

	QAction *cutAction;
	QAction *copyAction;
	QAction *deleteAction;
	QAction *copyToAction;
	QAction *moveToAction;
	QAction *renameAction;
	QAction *selectAllAction;

	QActionGroup *sortTypesGroup;
	QActionGroup *thumbLayoutsGroup;
	QAction *actName;
	QAction *actTime;
	QAction *actSize;
	QAction *actType;
	QAction *actReverse;
	QAction *refreshAction;
	QAction *fullScreenAct;
	QAction *thumbsGoTopAct;
	QAction *thumbsGoBottomAct;
	QAction *closeImageAct;
	QAction *settingsAction;
	QAction *thumbsZoomInAct;
	QAction *thumbsZoomOutAct;

	QAction *actClassic;
	QAction *actCompact;
	QAction *actSquarish;

	QAction *aboutAction;
	QAction *aboutQtAction;

	QAction *pasteAction;
	QAction *createDirAction;
	QAction *manageDirAction;

	QAction *goBackAction;
	QAction *goFrwdAction;
	QAction *goUpAction;
	QAction *goHomeAction;

	QAction *nextImageAction;
	QAction *prevImageAction;

	QLineEdit *pathBar;
	QCompleter *pathComplete;
	QLabel *stateLabel;
	QFileSystemModel *fsModel;
	FSTree *fsTree;
	QSplitter *splitter;
	QGridLayout *imageLayout;
	QStackedWidget *stackedWidget;
	ThumbView *thumbView;
	ImageView *imageView;
	bool initComplete;
	bool settingsChangedNeedRefresh;

	QList<QString> pathHistory;
	int currentHistoryIdx;
	bool needHistoryRecord;

	enum CentralWidgets
	{
		thumbViewIdx = 0,
		imageViewIdx
	};

	bool viewToolBarWasVisible;
	bool editToolBarWasVisible;
	bool goToolBarWasVisible;
};

#endif // MAINWINDOW_H

