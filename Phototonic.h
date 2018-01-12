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

#ifndef PHOTOTONIC_H
#define PHOTOTONIC_H

#include <QtWidgets>
#include "ThumbsViewer.h"
#include "ImageViewer.h"
#include "SettingsDialog.h"
#include "CopyMoveToDialog.h"
#include "CropDialog.h"
#include "ColorsDialog.h"
#include "ResizeDialog.h"

#define VERSION "Phototonic v2.0.1"

class Phototonic : public QMainWindow {
Q_OBJECT

public:

    int copyCutCount;

    Phototonic(QString fileOrDirectory, QWidget *parent = 0);

    QMenu *createPopupMenu();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);

    void closeEvent(QCloseEvent *event);

    void mousePressEvent(QMouseEvent *event);

public slots:

    bool event(QEvent *event);

    void setStatus(QString state);

    void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString copyMoveDirPath);

    void showViewer();

    void loadImagefromThumb(const QModelIndex &idx);

    void loadImagefromCli();

    void hideViewer();

    void showBusyStatus(bool busy);

private slots:

    void about();

    void sortThumbnails();

    void reload();

    void setIncludeSubFolders();

    void showSettings();

    void toggleFullScreen();

    void updateActions();

    void reloadThumbsSlot();

    void renameDir();

    void setThumbsViewerWindowTitle();

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

    void toggleSlideShow();

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

    void flipVertical();

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

    void flipHorizontal();

    void moveRight();

    void moveLeft();

    void moveUp();

    void moveDown();

    void setDocksVisibility(bool visible);

    void goTop();

    void goBottom();

    void showHiddenFiles();

    void toggleImageViewerToolbar();

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
    QAction *actionSortByName;
    QAction *actionSortByTime;
    QAction *actionSortBySize;
    QAction *actionSortByType;
    QAction *actionSortReverse;
    QAction *actionRefresh;
    QAction *actionIncludeSubDirectories;
    QAction *actionFullScreen;
    QAction *actionThumbsGoToTop;
    QAction *actionThumbsGoToBottom;
    QAction *actionCloseImage;
    QAction *actionSettings;
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

    QAction *actionAbout;
    QAction *actionShowHiddenFiles;
    QAction *actionSmallToolbarIcons;
    QAction *actionLockDocks;
    QAction *actionShowViewerToolbar;

    QAction *actionPaste;
    QAction *actionCreateDirectory;

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
    QAction *filterImagesFocusAct;
    QAction *setPathFocusAct;

    QAction *openWithMenuAct;
    QAction *chooseAppAct;
    QAction *invertSelectionAct;

    QLineEdit *pathLineEdit;
    QLineEdit *filterLineEdit;
    QLabel *statusLabel;
    QDockWidget *fileSystemDock;
    QDockWidget *bookmarksDock;
    QDockWidget *tagsDock;
    FileSystemTree *fileSystemTree;
    BookMarks *bookmarks;
    QDockWidget *imageInfoDock;
    ThumbsViewer *thumbsViewer;
    ImageViewer *imageViewer;
    QList<QString> pathHistoryList;
    QTimer *SlideShowTimer;
    CopyMoveToDialog *copyMoveToDialog;
    QWidget *fileSystemDockOrigWidget;
    QWidget *bookmarksDockOrigWidget;
    QWidget *tagsDockOrigWidget;
    QWidget *imageInfoDockOrigWidget;
    QWidget *fileSystemDockEmptyWidget;
    QWidget *bookmarksDockEmptyWidget;
    QWidget *tagsDockEmptyWidget;
    QWidget *imageInfoDockEmptyWidget;
    QVBoxLayout *imageViewLayout;
    bool interfaceDisabled;
    MetadataCache *metadataCache;

    enum CentralWidgets {
        ThumbViewWidget = 0,
        ImageViewWidget
    };

    int currentHistoryIdx;
    bool needHistoryRecord;
    bool initComplete;
    bool needThumbsRefresh;
    bool cliImageLoaded;
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

    void createImageViewer();

    void createThumbsViewer();

    void createActions();

    void createMenus();

    void createToolBars();

    void createStatusBar();

    void createFileSystemTree();

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

#endif // PHOTOTONIC_H

