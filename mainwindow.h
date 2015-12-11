/*
 *  Copyright (C) 2013-2015 Ofer Kashayov <oferkv@live.com>
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

#define VERSION "Phototonic v1.7.13"

class Phototonic : public QMainWindow
{
	Q_OBJECT

public:
	Phototonic(QString fileOrDirectory, QWidget *parent = 0);
	QMenu *createPopupMenu();
	
	int copyCutCount;

protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
	void closeEvent(QCloseEvent *event);
	void mousePressEvent(QMouseEvent *event);

public slots:
	bool event(QEvent *event);
	void setStatus(QString state);
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);
	void showViewer();
	void loadImagefromThumb(const QModelIndex &idx);
	void updateViewerImageBySelection(const QItemSelection& selection);
	void loadImagefromCli();
	void hideViewer();
	void showBusyStatus(bool busy);

private slots:
	void about();
	void sortThumbnains();
	void reload();
	void setIncludeSubFolders();
	void showSettings();
	void toggleFullScreen();
	void updateActions();
	void reloadThumbsSlot();
	void renameDir();
	void setThumbviewWindowTitle();
	void rename();
	void openOp();
	void newImage();
	void addNewBookmark();
	void deleteDir();
	void createSubDirectory();
	void checkDirState(const QModelIndex &, int, int);
	void goSelectedDir(const QModelIndex &currDir);
	void bookmarkClicked(QTreeWidgetItem *item, int col);
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
	void updateIndexByViewerImage();
	void selectAllThumbs();
	void deleteOp();
	void cutThumbs();
	void copyThumbs();
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
	void findDuplicateImages();
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
	void toggleImageToolbar();
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
	void setImageToolBarVisibility();
	void setFsDockVisibility();
	void setBmDockVisibility();
	void setTagsDockVisibility();
	void setIiDockVisibility();
	void setPvDockVisibility();
	void lockDocks();
	void cleanupCropDialog();
	void cleanupScaleDialog();
	void cleanupColorsDialog();
	void filterImagesFocus();
	void setPathFocus();
	void copyImagesTo();
	void moveImagesTo();


private:
	QString cliFileName;
	QMenuBar *thumbsMenuBar;
	QMenu *fileMenu;
	QMenu *editMenu;
	QMenu *goMenu;
	QMenu *sortMenu;
	QMenu *viewMenu;
	QMenu *toolsMenu;
	QMenu *helpMenu;
	QMenu *zoomSubMenu;
	QMenu *transformSubMenu;
	QMenu *viewSubMenu;
	QMenu *imageFileSubMenu;
	QMenu *MirroringSubMenu;
	QMenu *openWithSubMenu;

	QToolBar *viewToolBar;
	QToolBar *editToolBar;
	QToolBar *goToolBar;
	QToolBar *imageToolBar;

	QAction *exitAction;
	QAction *cutAction;
	QAction *copyAction;
	QAction *copyToAction;
	QAction *moveToAction;
	QAction *deleteAction;
	QAction *saveAction;
	QAction *saveAsAction;
	QAction *renameAction;
	QAction *selectAllAction;
	QAction *copyImageAction;
	QAction *pasteImageAction;
	QAction *showClipboardAction;
	QAction *addBookmarkAction;
	QAction *removeBookmarkAction;

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
	QAction *viewSubMenuAct;
	QAction *rotateLeftAct;
	QAction *rotateRightAct;
	QAction *flipHAct;
	QAction *flipVAct;
	QAction *cropAct;
	QAction *cropToSelectionAct;
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
	QAction *actShowViewerToolbars;
	
	QAction *pasteAction;
	QAction *createDirAction;

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
	QAction *findDupesAction;
	QAction *filterImagesFocusAct;
	QAction *setPathFocusAct;

	QAction *openWithMenuAct;
	QAction *chooseAppAct;
	QAction *invertSelectionAct;
	
	QLineEdit *pathBar;
	QLineEdit *filterBar;
	QLabel *stateLabel;
	QDockWidget *fsDock;
	QDockWidget *bmDock;
	QDockWidget *tagsDock;
	QDockWidget *pvDock;
	FSTree *fsTree;
	BookMarks *bookmarks;
	QHBoxLayout *mainLayout;
	QDockWidget *iiDock;
	ThumbView *thumbView;
	ImageView *imageView;
	QList<QString> pathHistory;
	QTimer *SlideShowTimer;
	CopyMoveToDialog *copyMoveToDialog;
	QWidget *fsDockOrigWidget;
	QWidget *bmDockOrigWidget;
	QWidget *tagsDockOrigWidget;
	QWidget *iiDockOrigWidget;
	QWidget *pvDockOrigWidget;
	QWidget *fsDockEmptyWidget;
	QWidget *bmDockEmptyWidget;
	QWidget *tagsDockEmptyWidget;
	QWidget *iiDockEmptyWidget;
	QWidget *pvDockEmptyWidget;
	QVBoxLayout *imageViewLayout;
	bool interfaceDisabled;
	MetadataCache *mdCache;

	enum CentralWidgets	{
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
	bool imageToolBarVisible;

	QMovie *busyMovie;
	QLabel *busyLabel;
	ResizeDialog *resizeDialog;
	ColorsDialog *colorsDialog;
	CropDialog *cropDialog;

	void refreshThumbs(bool noScroll);
	void loadShortcuts();
	void setupDocks();
	void deleteViewerImage();
	void loadCurrentImage(int currentRow);
	void selectCurrentViewDir();
	void handleStartupArgs(QString fileOrDirectory);
	void addMenuSeparator(QWidget *widget);
	void createImageView();
	void createThumbView();
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void setfsModelFlags();
	void createFSTree();
	void createBookmarks();
	void createImageTags();
	void writeSettings();
	void readSettings();
	void recordHistory(QString dir);
	bool isValidPath(QString &path);
	QString getSelectedPath();
	void setCopyCutActions(bool setEnabled);
	void setDeleteAction(bool setEnabled);
	void wheelEvent(QWheelEvent *event);	
	void copyOrCutThumbs(bool copy);
	void showNewImageWarning(QWidget *parent);
	bool removeDirOp(QString dirToDelete);
	void setInterfaceEnabled(bool enable);
	void addBookmark(QString path);
	void copyMoveImages(bool move);
	void setViewerKeyEventsEnabled(bool enabled);
};

#endif // MAINWINDOW_H

