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

#include <QtGui>
#include "thumbview.h"
#include "imageview.h"

class Phototonic : public QMainWindow
{
	Q_OBJECT

public:
	Phototonic(QWidget *parent = 0);
	int copyCutCount;

protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
	void closeEvent(QCloseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

signals:
	void abortThumbLoading();

public slots:
	void unsetBusy();
	void updateState(QString state, QString info = "");
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);
	void loadImageFile(QString imageFileName);
	void loadImagefromThumb(const QModelIndex &idx);
	void loadImagefromCli();
	void loadImagefromAction();
	void closeImage();

private slots:
	void about();
	void sortThumbnains();
	void reload();
	void refreshThumbs(bool noScroll);
	void setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag);
	void showSettings();
	void toggleFullScreen();
	void updateActions(QWidget *old, QWidget *now);
	void changeActionsBySelection(const QItemSelection&, const QItemSelection&);
	void reloadThumbsSlot();
	void renameDir();
	void rename();
	void openOp();
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
	void slideShow();
	void slideShowHandler();
	void loadNextImage();
	void loadPrevImage();
	void loadFirstImage();
	void loadLastImage();
	void loadRandomImage();
	void selectAllThumbs();
	void deleteOp();
	void cutImages();
	void copyImages();
	void pasteImages();
	void thumbsZoomIn();
	void thumbsZoomOut();
	void zoomIn();
	void zoomOut();
	void resetZoom();
	void origZoom();
	void keepZoom();
	void keepTransformClicked();
	void rotateLeft();
	void rotateRight();
	void flipVert();
	void cropImage();
	void setMirrorDisabled();
	void setMirrorDual();
	void setMirrorTriple();
	void setMirrorVDual();
	void setMirrorQuad();
	void flipHoriz();
	void setThumbViewWidgetsVisible(bool visible);
	void goTop();
	void goBottom();
	void setClassicThumbs();
	void setCompactThumbs();
	void setSquarishThumbs();
	void chooseExternalApp();
	void updateExternalApps();
	void runExternalApp();

private:
	QString cliFileName;
	QMenuBar *thumbsMenuBar;
	QMenu *fileMenu;
	QMenu *editMenu;
	QMenu *goMenu;
	QMenu *sortMenu;
	QMenu *viewMenu;
	QMenu *helpMenu;
	QMenu *zoomSubMenu;
	QMenu *transformSubMenu;
	QMenu *MirroringSubMenu;
	QMenu *openWithSubMenu;

	QToolBar *viewToolBar;
	QToolBar *editToolBar;
	QToolBar *goToolBar;

	QAction *exitAction;
	QAction *cutAction;
	QAction *copyAction;
	QAction *deleteAction;
	QAction *saveAction;
	QAction *saveAsAction;
	QAction *renameAction;
	QAction *selectAllAction;
	QAction *copyImageAction;

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
	QAction *zoomSubMenuAct;
	QAction *zoomInAct;
	QAction *zoomOutAct;
	QAction *resetZoomAct;
	QAction *origZoomAct;
	QAction *keepZoomAct;
	QAction *keepTransformAct;
	QAction *transformSubMenuAct;
	QAction *rotateLeftAct;
	QAction *rotateRightAct;
	QAction *flipHAct;
	QAction *flipVAct;
	QAction *cropAct;
	QActionGroup *mirroringGroup;
	QAction *mirrorSubMenuAct;
	QAction *mirrorDisabledAct;
	QAction *mirrorDualAct;
	QAction *mirrorTripleAct;
	QAction *mirrorVDualAct;
	QAction *mirrorQuadAct;
	
	QAction *actClassic;
	QAction *actCompact;
	QAction *actSquarish;
	QAction *aboutAction;

	QAction *pasteAction;
	QAction *createDirAction;
	QAction *manageDirAction;

	QAction *goBackAction;
	QAction *goFrwdAction;
	QAction *goUpAction;
	QAction *goHomeAction;

	QAction *slideShowAction;
	QAction *nextImageAction;
	QAction *prevImageAction;
	QAction *firstImageAction;
	QAction *lastImageAction;
	QAction *randomImageAction;
	QAction *openAction;

	QAction *openWithMenuAct;
	QAction *chooseAppAct;
	QAction *openWithExteralApp;
	
	QLineEdit *pathBar;
	QCompleter *pathComplete;
	QLabel *stateLabel;
	QLabel *infoLabel;
	QFileSystemModel *fsModel;
	FSTree *fsTree;
	QSplitter *splitter;
	QStackedWidget *stackedWidget;
	ThumbView *thumbView;
	ImageView *imageView;
	QList<QString> pathHistory;
	QProcess externalProcess;
	QString externalAppPath;

	enum CentralWidgets
	{
		thumbViewIdx = 0,
		imageViewIdx
	};

	bool viewToolBarWasVisible;
	bool editToolBarWasVisible;
	bool goToolBarWasVisible;
	int currentHistoryIdx;
	bool needHistoryRecord;
	bool initComplete;
	bool newSettingsRefreshThumbs;
	bool cliImageLoaded;
	bool thumbViewBusy;
	bool shouldMaximize;

	void loadShortcuts();
	void deleteSingleImage();
	void restoreCurrentIdx();
	bool handleArgs();
	void addMenuSeparator(QWidget *widget);
	void createImageView();
	void createThumbView();
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void createFSTree();
	void writeSettings();
	void readSettings();
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
	void wheelEvent(QWheelEvent *event);	
};

#endif // MAINWINDOW_H

