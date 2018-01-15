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

    int copyCutThumbsCount;

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

    void loadSelectedThumbImage(const QModelIndex &idx);

    void loadImageBromCliArguments();

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

    void removeMetadata();

    void viewImage();

    void newImage();

    void addNewBookmark();

    void deleteDirectory();

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

    void loadPreviousImage();

    void loadFirstImage();

    void loadLastImage();

    void loadRandomImage();

    void updateIndexByViewerImage();

    void selectAllThumbs();

    void deleteOperation();

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

    void setFileSystemDockVisibility();

    void setBookmarksDockVisibility();

    void setImagePreviewDockVisibility();

    void setTagsDockVisibility();

    void setImageInfoDockVisibility();

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
    QAction *removeMetadataAction;
    QAction *selectAllAction;
    QAction *copyImageAction;
    QAction *pasteImageAction;
    QAction *showClipboardAction;
    QAction *addBookmarkAction;
    QAction *removeBookmarkAction;

    QActionGroup *sortTypesGroup;
    QAction *sortByNameAction;
    QAction *sortByTimeAction;
    QAction *sortBySizeAction;
    QAction *sortByTypeAction;
    QAction *sortReverseAction;
    QAction *refreshAction;
    QAction *includeSubDirectoriesAction;
    QAction *fullScreenAction;
    QAction *thumbsGoToTopAction;
    QAction *thumbsGoToBottomAction;
    QAction *CloseImageAction;
    QAction *settingsAction;
    QAction *thumbsZoomInAction;
    QAction *thumbsZoomOutAction;
    QAction *zoomSubMenuAction;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *resetZoomAction;
    QAction *origZoomAction;
    QAction *keepZoomAction;
    QAction *keepTransformAction;
    QAction *transformSubMenuAction;
    QAction *viewSubMenuAction;
    QAction *rotateLeftAction;
    QAction *rotateRightAction;
    QAction *flipHorizontalAction;
    QAction *flipVerticalAction;
    QAction *cropAction;
    QAction *cropToSelectionAction;
    QAction *resizeAction;
    QAction *freeRotateLeftAction;
    QAction *freeRotateRightAction;
    QAction *colorsAction;
    QActionGroup *mirroringActionGroup;
    QAction *mirrorSubMenuAction;
    QAction *mirrorDisabledAction;
    QAction *mirrorDualAction;
    QAction *mirrorTripleAction;
    QAction *mirrorDualVerticalAction;
    QAction *mirrorQuadAction;
    QAction *moveLeftAction;
    QAction *moveRightAction;
    QAction *moveUpAction;
    QAction *moveDownAction;

    QAction *aboutAction;
    QAction *showHiddenFilesAction;
    QAction *smallToolbarIconsAction;
    QAction *lockDocksAction;
    QAction *showViewerToolbarAction;

    QAction *pasteAction;
    QAction *createDirectoryAction;

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
    QAction *viewImageAction;
    QAction *filterImagesFocusAction;
    QAction *setPathFocusAction;

    QAction *openWithMenuAction;
    QAction *chooseAppAction;
    QAction *invertSelectionAction;

    QLineEdit *pathLineEdit;
    QLineEdit *filterLineEdit;
    QLabel *statusLabel;
    QDockWidget *fileSystemDock;
    QDockWidget *bookmarksDock;
    QDockWidget *imagePreviewDock;
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
    QWidget *imagePreviewDockOrigWidget;
    QWidget *tagsDockOrigWidget;
    QWidget *imageInfoDockOrigWidget;
    QWidget *fileSystemDockEmptyWidget;
    QWidget *bookmarksDockEmptyWidget;
    QWidget *imagePreviewDockEmptyWidget;
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

    void viewerDeleteImage();

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

    void createImagePreview();

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

    bool removeDirectoryOperation(QString dirToDelete);

    void setInterfaceEnabled(bool enable);

    void addBookmark(QString path);

    void copyOrMoveImages(bool move);

    void setViewerKeyEventsEnabled(bool enabled);
};

#endif // PHOTOTONIC_H

