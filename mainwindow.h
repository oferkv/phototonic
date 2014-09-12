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

#include <QtWidgets>
#include "thumbview.h"
#include "imageview.h"
#include "dialogs.h"

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

public slots:
	bool event(QEvent *event);
	void unsetBusy();
	void setStatus(QString state);
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);
	void showViewer();
	void loadImagefromThumb(const QModelIndex &idx);
	void updateViewerImageBySelection(const QItemSelection& selection);
	void loadImagefromCli();
	void closeImage();

private slots:
	void about();
	void sortThumbnains();
	void reload();
	void setIncludeSubFolders();
	void refreshThumbs(bool noScroll);
	void showSettings();
	void toggleFullScreen();
	void updateActions(QWidget *old, QWidget *now);
	void changeActionsBySelection(const QItemSelection&, const QItemSelection&);
	void reloadThumbsSlot();
	void renameDir();
	void setThumbviewWindowTitle();
	void rename();
	void openOp();
	void newImage();
	void deleteDir();
	void manageDir();
	void createSubDirectory();
	void checkDirState(const QModelIndex &, int, int);
	void goSelectedDir(const QModelIndex &currDir);
	void goPathBarDir();
	void setThumbsFilter();
	void clearThumbsFilter();
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
	void selectRecentThumb();
	void setViewerKeyEventsEnabled(bool enabled);
	void updateIndexByViewerImage();
	void selectAllThumbs();
	void deleteOp();
	void cutThumbs();
	void copyThumbs();
	void copyMoveImages();
	void pasteThumbs();
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
	void scaleImage();
	void freeRotateLeft();
	void freeRotateRight();
	void showColorsDialog();
	void setMirrorDisabled();
	void setMirrorDual();
	void setMirrorTriple();
	void setMirrorVDual();
	void setMirrorQuad();
	void flipHoriz();
	void moveRight();
	void moveLeft();
	void moveUp();
	void moveDown();
	void setDocksVisibility(bool visible);
	void goTop();
	void goBottom();
	void setClassicThumbs();
	void setCompactThumbs();
	void setSquarishThumbs();
	void showHiddenFiles();
	void showLabels();
	void setToolbarIconSize();
	void chooseExternalApp();
	void updateExternalApps();
	void runExternalApp();
	void cleanupSender();
	void externalAppError();
	void setEditToolBarVisibility();
	void setGoToolBarVisibility();
	void setViewToolBarVisibility();
	void setFsDockVisibility();
	void setIiDockVisibility();
	void setPvDockVisibility();
	void lockDocks();
	void enableInterface();

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
	QMenu *imageFileSubMenu;
	QMenu *MirroringSubMenu;
	QMenu *openWithSubMenu;

	QToolBar *viewToolBar;
	QToolBar *editToolBar;
	QToolBar *goToolBar;

	QAction *exitAction;
	QAction *cutAction;
	QAction *copyAction;
	QAction *copyMoveAction;
	QAction *deleteAction;
	QAction *saveAction;
	QAction *saveAsAction;
	QAction *renameAction;
	QAction *selectAllAction;
	QAction *copyImageAction;
	QAction *pasteImageAction;
	QAction *showClipboardAction;

	QActionGroup *sortTypesGroup;
	QActionGroup *thumbLayoutsGroup;
	QAction *actName;
	QAction *actTime;
	QAction *actSize;
	QAction *actType;
	QAction *actReverse;
	QAction *refreshAction;
	QAction *subFoldersAction;
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
	QAction *resizeAct;
	QAction *freeRotateLeftAct;
	QAction *freeRotateRightAct;
	QAction *colorsAct;
	QActionGroup *mirroringGroup;
	QAction *mirrorSubMenuAct;
	QAction *mirrorDisabledAct;
	QAction *mirrorDualAct;
	QAction *mirrorTripleAct;
	QAction *mirrorVDualAct;
	QAction *mirrorQuadAct;
	QAction *moveLeftAct;
	QAction *moveRightAct;
	QAction *moveUpAct;
	QAction *moveDownAct;
	
	QAction *actClassic;
	QAction *actCompact;
	QAction *actSquarish;
	QAction *aboutAction;
	QAction *actShowHidden;
	QAction *actShowLabels;
	QAction *actSmallIcons;
	QAction *actLockDocks;
	
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
	QAction *invertSelectionAct;
	
	QLineEdit *pathBar;
	QLineEdit *filterBar;
	QCompleter *pathComplete;
	QLabel *stateLabel;
	QDockWidget *fsDock;
	QDockWidget *pvDock;
	QFileSystemModel *fsModel;
	FSTree *fsTree;
	QVBoxLayout *mainLayout;
	QDockWidget *iiDock;
	ThumbView *thumbView;
	ImageView *imageView;
	QList<QString> pathHistory;
	QTimer *SlideShowTimer;
	CopyMoveToDialog *copyMoveToDialog;
	QWidget *fsDockOrigWidget;
	QWidget *iiDockOrigWidget;
	QWidget *pvDockOrigWidget;
	QWidget *fsDockEmptyWidget;
	QWidget *iiDockEmptyWidget;
	QWidget *pvDockEmptyWidget;
	QVBoxLayout *imageViewContainer;
	bool interfaceDisabled;

	enum CentralWidgets
	{
		thumbViewIdx = 0,
		imageViewIdx
	};

	int currentHistoryIdx;
	bool needHistoryRecord;
	bool initComplete;
	bool needThumbsRefresh;
	bool cliImageLoaded;
	bool thumbViewBusy;
	bool shouldMaximize;

	bool editToolBarVisible;
	bool goToolBarVisible;
	bool viewToolBarVisible;

	void loadShortcuts();
	void setupDocks();
	void deleteViewerImage();
	void updateCurrentImage(int currentRow);
	void selectCurrentViewDir();
	void handleStartupArgs();
	void addMenuSeparator(QWidget *widget);
	void createImageView();
	void createThumbView();
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void setfsModelFlags();
	void createFSTree();
	void writeSettings();
	void readSettings();
	void recordHistory(QString dir);
	bool isValidPath(QString &path);
	bool isBusy()
	{
		return thumbViewBusy;
	}
	QString getSelectedPath();
	void setCopyCutActions(bool setEnabled);
	void wheelEvent(QWheelEvent *event);	
	void copyOrCutThumbs(bool copy);
	void showNewImageWarning(QWidget *parent);
	bool removeDirOp(QString dirToDelete);
	void closeImageCleanup();
	void selectThumbByRow(int row);
	void setInterfaceEnabled(bool enable);
};

#endif // MAINWINDOW_H

