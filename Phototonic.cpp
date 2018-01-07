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

#include "DirCompleter.h"
#include "Phototonic.h"
#include "Settings.h"

#define THUMB_SIZE_MIN    50
#define THUMB_SIZE_MAX    500

Phototonic::Phototonic(QString fileOrDirectory, QWidget *parent) : QMainWindow(parent) {
    Settings::appSettings = new QSettings("phototonic", "phototonic");
    setDockOptions(QMainWindow::AllowNestedDocks);
    readSettings();
    createThumbsViewer();
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createFileSystemTree();
    createBookmarks();
    createImageTags();
    createImageViewer();
    updateExternalApps();
    loadShortcuts();
    setupDocks();

    connect(qApp, SIGNAL(focusChanged(QWidget * , QWidget * )), this, SLOT(updateActions()));

    restoreGeometry(Settings::appSettings->value("Geometry").toByteArray());
    restoreState(Settings::appSettings->value("WindowState").toByteArray());
    setWindowIcon(QIcon(":/images/phototonic.png"));
    setCentralWidget(thumbsViewer);

    handleStartupArgs(fileOrDirectory);

    copyMoveToDialog = 0;
    colorsDialog = 0;
    cropDialog = 0;
    initComplete = true;
    thumbsViewer->busy = false;
    currentHistoryIdx = -1;
    needHistoryRecord = true;
    interfaceDisabled = false;

    refreshThumbs(true);
    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->setFocus(Qt::OtherFocusReason);
    }

    if (cliImageLoaded) {
        QTimer::singleShot(100, this, SLOT(updateIndexByViewerImage()));
    }
}

void Phototonic::handleStartupArgs(QString fileOrDirectory) {
    cliImageLoaded = false;
    if (fileOrDirectory.size()) {
        QFileInfo cliFileOrDir(fileOrDirectory);
        if (cliFileOrDir.isDir()) {
            Settings::currentViewDir = fileOrDirectory;
            cliImageLoaded = false;
        } else {
            Settings::currentViewDir = cliFileOrDir.absolutePath();
            cliFileName = Settings::currentViewDir + QDir::separator() + cliFileOrDir.fileName();
            cliImageLoaded = true;
            loadImagefromCli();
        }
    } else {
        if (Settings::startupDir == Settings::SpecifiedDir) {
            Settings::currentViewDir = Settings::specifiedStartDir;
        } else if (Settings::startupDir == Settings::RememberLastDir) {
            Settings::currentViewDir = Settings::appSettings->value("lastDir").toString();
        }
    }
    selectCurrentViewDir();
}

bool Phototonic::event(QEvent *event) {
    if (event->type() == QEvent::ActivationChange ||
        (Settings::layoutMode == ThumbViewWidget && event->type() == QEvent::MouseButtonRelease)) {
        thumbsViewer->loadVisibleThumbs();
    }

    return QMainWindow::event(event);
}

void Phototonic::createThumbsViewer() {
    metadataCache = new MetadataCache;
    thumbsViewer = new ThumbsViewer(this, metadataCache);
    thumbsViewer->thumbsSortFlags = (QDir::SortFlags) Settings::appSettings->value(
            Settings::optionThumbsSortFlags).toInt();
    thumbsViewer->thumbsSortFlags |= QDir::IgnoreCase;

    connect(thumbsViewer, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
    connect(thumbsViewer, SIGNAL(showBusy(bool)), this, SLOT(showBusyStatus(bool)));
    connect(thumbsViewer->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(updateActions()));

    imageInfoDock = new QDockWidget(tr("Image Info"), this);
    imageInfoDock->setObjectName("Image Info");
    imageInfoDock->setWidget(thumbsViewer->infoView);
    connect(imageInfoDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setIiDockVisibility()));
    connect(imageInfoDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setIiDockVisibility()));
}

void Phototonic::addMenuSeparator(QWidget *widget) {
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    widget->addAction(separator);
}

void Phototonic::createImageViewer() {
    imageViewer = new ImageViewer(this, metadataCache);
    connect(saveAction, SIGNAL(triggered()), imageViewer, SLOT(saveImage()));
    connect(saveAsAction, SIGNAL(triggered()), imageViewer, SLOT(saveImageAs()));
    connect(copyImageAction, SIGNAL(triggered()), imageViewer, SLOT(copyImage()));
    connect(pasteImageAction, SIGNAL(triggered()), imageViewer, SLOT(pasteImage()));
    connect(cropToSelectionAct, SIGNAL(triggered()), imageViewer, SLOT(cropToSelection()));
    imageViewer->ImagePopUpMenu = new QMenu();

    // Widget actions
    imageViewer->addAction(slideShowAction);
    imageViewer->addAction(nextImageAction);
    imageViewer->addAction(prevImageAction);
    imageViewer->addAction(firstImageAction);
    imageViewer->addAction(lastImageAction);
    imageViewer->addAction(randomImageAction);
    imageViewer->addAction(zoomInAct);
    imageViewer->addAction(zoomOutAct);
    imageViewer->addAction(origZoomAct);
    imageViewer->addAction(resetZoomAct);
    imageViewer->addAction(rotateRightAct);
    imageViewer->addAction(rotateLeftAct);
    imageViewer->addAction(freeRotateRightAct);
    imageViewer->addAction(freeRotateLeftAct);
    imageViewer->addAction(flipHAct);
    imageViewer->addAction(flipVAct);
    imageViewer->addAction(cropAct);
    imageViewer->addAction(cropToSelectionAct);
    imageViewer->addAction(resizeAct);
    imageViewer->addAction(saveAction);
    imageViewer->addAction(saveAsAction);
    imageViewer->addAction(copyImageAction);
    imageViewer->addAction(pasteImageAction);
    imageViewer->addAction(deleteAction);
    imageViewer->addAction(renameAction);
    imageViewer->addAction(actionCloseImage);
    imageViewer->addAction(actionFullScreen);
    imageViewer->addAction(actionSettings);
    imageViewer->addAction(mirrorDisabledAct);
    imageViewer->addAction(mirrorDualAct);
    imageViewer->addAction(mirrorTripleAct);
    imageViewer->addAction(mirrorVDualAct);
    imageViewer->addAction(mirrorQuadAct);
    imageViewer->addAction(keepTransformAct);
    imageViewer->addAction(keepZoomAct);
    imageViewer->addAction(actionRefresh);
    imageViewer->addAction(colorsAct);
    imageViewer->addAction(moveRightAct);
    imageViewer->addAction(moveLeftAct);
    imageViewer->addAction(moveUpAct);
    imageViewer->addAction(moveDownAct);
    imageViewer->addAction(showClipboardAction);
    imageViewer->addAction(copyToAction);
    imageViewer->addAction(moveToAction);
    imageViewer->addAction(resizeAct);
    imageViewer->addAction(openAction);
    imageViewer->addAction(exitAction);

    // Actions
    addMenuSeparator(imageViewer->ImagePopUpMenu);
    imageViewer->ImagePopUpMenu->addAction(nextImageAction);
    imageViewer->ImagePopUpMenu->addAction(prevImageAction);
    imageViewer->ImagePopUpMenu->addAction(firstImageAction);
    imageViewer->ImagePopUpMenu->addAction(lastImageAction);
    imageViewer->ImagePopUpMenu->addAction(randomImageAction);
    imageViewer->ImagePopUpMenu->addAction(slideShowAction);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    zoomSubMenu = new QMenu(tr("Zoom"));
    zoomSubMenuAct = new QAction(tr("Zoom"), this);
    zoomSubMenuAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
    zoomSubMenuAct->setMenu(zoomSubMenu);
    imageViewer->ImagePopUpMenu->addAction(zoomSubMenuAct);
    zoomSubMenu->addAction(zoomInAct);
    zoomSubMenu->addAction(zoomOutAct);
    zoomSubMenu->addAction(origZoomAct);
    zoomSubMenu->addAction(resetZoomAct);
    addMenuSeparator(zoomSubMenu);
    zoomSubMenu->addAction(keepZoomAct);

    MirroringSubMenu = new QMenu(tr("Mirroring"));
    mirrorSubMenuAct = new QAction(tr("Mirroring"), this);
    mirrorSubMenuAct->setMenu(MirroringSubMenu);
    mirroringGroup = new QActionGroup(this);
    mirroringGroup->addAction(mirrorDisabledAct);
    mirroringGroup->addAction(mirrorDualAct);
    mirroringGroup->addAction(mirrorTripleAct);
    mirroringGroup->addAction(mirrorVDualAct);
    mirroringGroup->addAction(mirrorQuadAct);
    MirroringSubMenu->addActions(mirroringGroup->actions());

    transformSubMenu = new QMenu(tr("Transform"));
    transformSubMenuAct = new QAction(tr("Transform"), this);
    transformSubMenuAct->setMenu(transformSubMenu);
    imageViewer->ImagePopUpMenu->addAction(resizeAct);
    imageViewer->ImagePopUpMenu->addAction(cropToSelectionAct);
    imageViewer->ImagePopUpMenu->addAction(transformSubMenuAct);
    transformSubMenu->addAction(colorsAct);
    transformSubMenu->addAction(rotateRightAct);
    transformSubMenu->addAction(rotateLeftAct);
    transformSubMenu->addAction(freeRotateRightAct);
    transformSubMenu->addAction(freeRotateLeftAct);
    transformSubMenu->addAction(flipHAct);
    transformSubMenu->addAction(flipVAct);
    transformSubMenu->addAction(cropAct);

    addMenuSeparator(transformSubMenu);
    transformSubMenu->addAction(keepTransformAct);
    imageViewer->ImagePopUpMenu->addAction(mirrorSubMenuAct);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    imageViewer->ImagePopUpMenu->addAction(copyToAction);
    imageViewer->ImagePopUpMenu->addAction(moveToAction);
    imageViewer->ImagePopUpMenu->addAction(saveAction);
    imageViewer->ImagePopUpMenu->addAction(saveAsAction);
    imageViewer->ImagePopUpMenu->addAction(renameAction);
    imageViewer->ImagePopUpMenu->addAction(deleteAction);
    imageViewer->ImagePopUpMenu->addAction(openWithMenuAct);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    viewSubMenu = new QMenu(tr("View"));
    viewSubMenuAct = new QAction(tr("View"), this);
    viewSubMenuAct->setMenu(viewSubMenu);
    imageViewer->ImagePopUpMenu->addAction(viewSubMenuAct);
    viewSubMenu->addAction(actionFullScreen);
    viewSubMenu->addAction(showClipboardAction);
    viewSubMenu->addAction(actionShowViewerToolbar);
    viewSubMenu->addAction(actionRefresh);
    imageViewer->ImagePopUpMenu->addAction(copyImageAction);
    imageViewer->ImagePopUpMenu->addAction(pasteImageAction);
    imageViewer->ImagePopUpMenu->addAction(actionCloseImage);
    imageViewer->ImagePopUpMenu->addAction(exitAction);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    imageViewer->ImagePopUpMenu->addAction(actionSettings);

    imageViewer->setContextMenuPolicy(Qt::DefaultContextMenu);
    Settings::isFullScreen = Settings::appSettings->value(Settings::optionFullScreenMode).toBool();
    actionFullScreen->setChecked(Settings::isFullScreen);
    thumbsViewer->setImageView(imageViewer);
}

void Phototonic::createActions() {
    actionThumbsGoToTop = new QAction(tr("Top"), this);
    actionThumbsGoToTop->setObjectName("thumbsGoTop");
    actionThumbsGoToTop->setIcon(QIcon::fromTheme("go-top", QIcon(":/images/top.png")));
    connect(actionThumbsGoToTop, SIGNAL(triggered()), this, SLOT(goTop()));

    actionThumbsGoToBottom = new QAction(tr("Bottom"), this);
    actionThumbsGoToBottom->setObjectName("thumbsGoBottom");
    actionThumbsGoToBottom->setIcon(QIcon::fromTheme("go-bottom", QIcon(":/images/bottom.png")));
    connect(actionThumbsGoToBottom, SIGNAL(triggered()), this, SLOT(goBottom()));

    actionCloseImage = new QAction(tr("Close Image"), this);
    actionCloseImage->setObjectName("closeImage");
    connect(actionCloseImage, SIGNAL(triggered()), this, SLOT(hideViewer()));

    actionFullScreen = new QAction(tr("Full Screen"), this);
    actionFullScreen->setObjectName("fullScreen");
    actionFullScreen->setCheckable(true);
    connect(actionFullScreen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

    actionSettings = new QAction(tr("Preferences"), this);
    actionSettings->setObjectName("settings");
    actionSettings->setIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/settings.png")));
    connect(actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));

    exitAction = new QAction(tr("Exit"), this);
    exitAction->setObjectName("exit");
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    thumbsZoomInAct = new QAction(tr("Enlarge Thumbnails"), this);
    thumbsZoomInAct->setObjectName("thumbsZoomIn");
    connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
    thumbsZoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
    if (thumbsViewer->thumbSize == THUMB_SIZE_MAX) {
        thumbsZoomInAct->setEnabled(false);
    }

    thumbsZoomOutAct = new QAction(tr("Shrink Thumbnails"), this);
    thumbsZoomOutAct->setObjectName("thumbsZoomOut");
    connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
    thumbsZoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
    if (thumbsViewer->thumbSize == THUMB_SIZE_MIN) {
        thumbsZoomOutAct->setEnabled(false);
    }

    cutAction = new QAction(tr("Cut"), this);
    cutAction->setObjectName("cut");
    cutAction->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png")));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cutThumbs()));
    cutAction->setEnabled(false);

    copyAction = new QAction(tr("Copy"), this);
    copyAction->setObjectName("copy");
    copyAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png")));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copyThumbs()));
    copyAction->setEnabled(false);

    copyToAction = new QAction(tr("Copy to..."), this);
    copyToAction->setObjectName("copyTo");
    connect(copyToAction, SIGNAL(triggered()), this, SLOT(copyImagesTo()));

    moveToAction = new QAction(tr("Move to..."), this);
    moveToAction->setObjectName("moveTo");
    connect(moveToAction, SIGNAL(triggered()), this, SLOT(moveImagesTo()));

    deleteAction = new QAction(tr("Delete"), this);
    deleteAction->setObjectName("delete");
    deleteAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

    saveAction = new QAction(tr("Save"), this);
    saveAction->setObjectName("save");
    saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

    saveAsAction = new QAction(tr("Save As"), this);
    saveAsAction->setObjectName("saveAs");
    saveAsAction->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/images/save_as.png")));

    copyImageAction = new QAction(tr("Copy Image Data"), this);
    copyImageAction->setObjectName("copyImage");
    pasteImageAction = new QAction(tr("Paste Image Data"), this);
    pasteImageAction->setObjectName("pasteImage");

    renameAction = new QAction(tr("Rename"), this);
    renameAction->setObjectName("rename");
    connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

    selectAllAction = new QAction(tr("Select All"), this);
    selectAllAction->setObjectName("selectAll");
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

    actionAbout = new QAction(tr("About"), this);
    actionAbout->setObjectName("about");
    actionAbout->setIcon(QIcon::fromTheme("help-about", QIcon(":/images/about.png")));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));

    // Sort actions
    actionSortByName = new QAction(tr("Name"), this);
    actionSortByName->setObjectName("name");
    actionSortByTime = new QAction(tr("Time"), this);
    actionSortByTime->setObjectName("time");
    actionSortBySize = new QAction(tr("Size"), this);
    actionSortBySize->setObjectName("size");
    actionSortByType = new QAction(tr("Type"), this);
    actionSortByType->setObjectName("type");
    actionSortReverse = new QAction(tr("Reverse"), this);
    actionSortReverse->setObjectName("reverse");
    actionSortByName->setCheckable(true);
    actionSortByTime->setCheckable(true);
    actionSortBySize->setCheckable(true);
    actionSortByType->setCheckable(true);
    actionSortReverse->setCheckable(true);
    connect(actionSortByName, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(actionSortByTime, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(actionSortBySize, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(actionSortByType, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(actionSortReverse, SIGNAL(triggered()), this, SLOT(sortThumbnails()));

    if (thumbsViewer->thumbsSortFlags & QDir::Time) {
        actionSortByTime->setChecked(true);
    } else if (thumbsViewer->thumbsSortFlags & QDir::Size) {
        actionSortBySize->setChecked(true);
    } else if (thumbsViewer->thumbsSortFlags & QDir::Type) {
        actionSortByType->setChecked(true);
    } else {
        actionSortByName->setChecked(true);
    }
    actionSortReverse->setChecked(thumbsViewer->thumbsSortFlags & QDir::Reversed);

    actionShowHiddenFiles = new QAction(tr("Show Hidden Files"), this);
    actionShowHiddenFiles->setObjectName("showHidden");
    actionShowHiddenFiles->setCheckable(true);
    actionShowHiddenFiles->setChecked(Settings::showHiddenFiles);
    connect(actionShowHiddenFiles, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

    actionSmallToolbarIcons = new QAction(tr("Small Toolbar Icons"), this);
    actionSmallToolbarIcons->setObjectName("smallToolbarIcons");
    actionSmallToolbarIcons->setCheckable(true);
    actionSmallToolbarIcons->setChecked(Settings::smallToolbarIcons);
    connect(actionSmallToolbarIcons, SIGNAL(triggered()), this, SLOT(setToolbarIconSize()));

    actionLockDocks = new QAction(tr("Hide Dock Title Bars"), this);
    actionLockDocks->setObjectName("lockDocks");
    actionLockDocks->setCheckable(true);
    actionLockDocks->setChecked(Settings::hideDockTitlebars);
    connect(actionLockDocks, SIGNAL(triggered()), this, SLOT(lockDocks()));

    actionShowViewerToolbar = new QAction(tr("Show Toolbar"), this);
    actionShowViewerToolbar->setObjectName("showViewerToolbars");
    actionShowViewerToolbar->setCheckable(true);
    actionShowViewerToolbar->setChecked(Settings::showViewerToolbar);
    connect(actionShowViewerToolbar, SIGNAL(triggered()), this, SLOT(toggleImageViewerToolbar()));

    actionRefresh = new QAction(tr("Reload"), this);
    actionRefresh->setObjectName("refresh");
    actionRefresh->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
    connect(actionRefresh, SIGNAL(triggered()), this, SLOT(reload()));

    actionIncludeSubDirectories = new QAction(tr("Include Sub-directories"), this);
    actionIncludeSubDirectories->setObjectName("subFolders");
    actionIncludeSubDirectories->setIcon(QIcon(":/images/tree.png"));
    actionIncludeSubDirectories->setCheckable(true);
    connect(actionIncludeSubDirectories, SIGNAL(triggered()), this, SLOT(setIncludeSubFolders()));

    actionPaste = new QAction(tr("Paste Here"), this);
    actionPaste->setObjectName("paste");
    actionPaste->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
    connect(actionPaste, SIGNAL(triggered()), this, SLOT(pasteThumbs()));
    actionPaste->setEnabled(false);

    actionCreateDirectory = new QAction(tr("New Directory"), this);
    actionCreateDirectory->setObjectName("createDir");
    connect(actionCreateDirectory, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
    actionCreateDirectory->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));

    goBackAction = new QAction(tr("Back"), this);
    goBackAction->setObjectName("goBack");
    goBackAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
    connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
    goBackAction->setEnabled(false);

    goFrwdAction = new QAction(tr("Forward"), this);
    goFrwdAction->setObjectName("goFrwd");
    goFrwdAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
    connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
    goFrwdAction->setEnabled(false);

    goUpAction = new QAction(tr("Go Up"), this);
    goUpAction->setObjectName("up");
    goUpAction->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/up.png")));
    connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

    goHomeAction = new QAction(tr("Home"), this);
    goHomeAction->setObjectName("home");
    connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));
    goHomeAction->setIcon(QIcon::fromTheme("go-home", QIcon(":/images/home.png")));

    slideShowAction = new QAction(tr("Slide Show"), this);
    slideShowAction->setObjectName("toggleSlideShow");
    connect(slideShowAction, SIGNAL(triggered()), this, SLOT(toggleSlideShow()));
    slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));

    nextImageAction = new QAction(tr("Next"), this);
    nextImageAction->setObjectName("nextImage");
    nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
    connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));

    prevImageAction = new QAction(tr("Previous"), this);
    prevImageAction->setObjectName("prevImage");
    prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
    connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

    firstImageAction = new QAction(tr("First"), this);
    firstImageAction->setObjectName("firstImage");
    firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
    connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

    lastImageAction = new QAction(tr("Last"), this);
    lastImageAction->setObjectName("lastImage");
    lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
    connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

    randomImageAction = new QAction(tr("Random"), this);
    randomImageAction->setObjectName("randomImage");
    connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

    openAction = new QAction(tr("Open"), this);
    openAction->setObjectName("open");
    openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    connect(openAction, SIGNAL(triggered()), this, SLOT(openOp()));

    showClipboardAction = new QAction(tr("Load Clipboard"), this);
    showClipboardAction->setObjectName("showClipboard");
    showClipboardAction->setIcon(QIcon::fromTheme("insert-image", QIcon(":/images/new.png")));
    connect(showClipboardAction, SIGNAL(triggered()), this, SLOT(newImage()));

    openWithSubMenu = new QMenu(tr("Open With..."));
    openWithMenuAct = new QAction(tr("Open With..."), this);
    openWithMenuAct->setObjectName("openWithMenu");
    openWithMenuAct->setMenu(openWithSubMenu);
    chooseAppAct = new QAction(tr("Manage External Applications"), this);
    chooseAppAct->setObjectName("chooseApp");
    connect(chooseAppAct, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));

    addBookmarkAction = new QAction(tr("Add Bookmark"), this);
    addBookmarkAction->setObjectName("addBookmark");
    addBookmarkAction->setIcon(QIcon(":/images/new_bookmark.png"));
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(addNewBookmark()));

    removeBookmarkAction = new QAction(tr("Remove Bookmark"), this);
    removeBookmarkAction->setObjectName("removeBookmark");
    removeBookmarkAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

    zoomOutAct = new QAction(tr("Zoom Out"), this);
    zoomOutAct->setObjectName("zoomOut");
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
    zoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

    zoomInAct = new QAction(tr("Zoom In"), this);
    zoomInAct->setObjectName("zoomIn");
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
    zoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

    resetZoomAct = new QAction(tr("Reset Zoom"), this);
    resetZoomAct->setObjectName("resetZoom");
    resetZoomAct->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom.png")));
    connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

    origZoomAct = new QAction(tr("Original Size"), this);
    origZoomAct->setObjectName("origZoom");
    origZoomAct->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
    connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

    keepZoomAct = new QAction(tr("Keep Zoom"), this);
    keepZoomAct->setObjectName("keepZoom");
    keepZoomAct->setCheckable(true);
    connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

    rotateLeftAct = new QAction(tr("Rotate 90 degree CCW"), this);
    rotateLeftAct->setObjectName("rotateLeft");
    rotateLeftAct->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
    connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

    rotateRightAct = new QAction(tr("Rotate 90 degree CW"), this);
    rotateRightAct->setObjectName("rotateRight");
    rotateRightAct->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
    connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

    flipHAct = new QAction(tr("Flip Horizontally"), this);
    flipHAct->setObjectName("flipH");
    flipHAct->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
    connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));

    flipVAct = new QAction(tr("Flip Vertically"), this);
    flipVAct->setObjectName("flipV");
    flipVAct->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
    connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));

    cropAct = new QAction(tr("Cropping"), this);
    cropAct->setObjectName("crop");
    cropAct->setIcon(QIcon(":/images/crop.png"));
    connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));

    cropToSelectionAct = new QAction(tr("Crop to Selection"), this);
    cropToSelectionAct->setObjectName("cropToSelection");
    cropToSelectionAct->setIcon(QIcon(":/images/crop.png"));

    resizeAct = new QAction(tr("Scale Image"), this);
    resizeAct->setObjectName("resize");
    resizeAct->setIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/scale.png")));
    connect(resizeAct, SIGNAL(triggered()), this, SLOT(scaleImage()));

    freeRotateLeftAct = new QAction(tr("Rotate 1 degree CCW"), this);
    freeRotateLeftAct->setObjectName("freeRotateLeft");
    connect(freeRotateLeftAct, SIGNAL(triggered()), this, SLOT(freeRotateLeft()));

    freeRotateRightAct = new QAction(tr("Rotate 1 degree CW"), this);
    freeRotateRightAct->setObjectName("freeRotateRight");
    connect(freeRotateRightAct, SIGNAL(triggered()), this, SLOT(freeRotateRight()));

    colorsAct = new QAction(tr("Colors"), this);
    colorsAct->setObjectName("colors");
    connect(colorsAct, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
    colorsAct->setIcon(QIcon(":/images/colors.png"));

    findDupesAction = new QAction(tr("Find Duplicate Images"), this);
    findDupesAction->setObjectName("findDupes");
    findDupesAction->setIcon(QIcon(":/images/duplicates.png"));
    findDupesAction->setCheckable(true);
    connect(findDupesAction, SIGNAL(triggered()), this, SLOT(findDuplicateImages()));

    mirrorDisabledAct = new QAction(tr("Disable"), this);
    mirrorDisabledAct->setObjectName("mirrorDisabled");
    mirrorDualAct = new QAction(tr("Dual"), this);
    mirrorDualAct->setObjectName("mirrorDual");
    mirrorTripleAct = new QAction(tr("Triple"), this);
    mirrorTripleAct->setObjectName("mirrorTriple");
    mirrorVDualAct = new QAction(tr("Dual Vertical"), this);
    mirrorVDualAct->setObjectName("mirrorVDual");
    mirrorQuadAct = new QAction(tr("Quad"), this);
    mirrorQuadAct->setObjectName("mirrorQuad");

    mirrorDisabledAct->setCheckable(true);
    mirrorDualAct->setCheckable(true);
    mirrorTripleAct->setCheckable(true);
    mirrorVDualAct->setCheckable(true);
    mirrorQuadAct->setCheckable(true);
    connect(mirrorDisabledAct, SIGNAL(triggered()), this, SLOT(setMirrorDisabled()));
    connect(mirrorDualAct, SIGNAL(triggered()), this, SLOT(setMirrorDual()));
    connect(mirrorTripleAct, SIGNAL(triggered()), this, SLOT(setMirrorTriple()));
    connect(mirrorVDualAct, SIGNAL(triggered()), this, SLOT(setMirrorVDual()));
    connect(mirrorQuadAct, SIGNAL(triggered()), this, SLOT(setMirrorQuad()));
    mirrorDisabledAct->setChecked(true);

    keepTransformAct = new QAction(tr("Lock Transformations"), this);
    keepTransformAct->setObjectName("keepTransform");
    keepTransformAct->setCheckable(true);
    connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

    moveLeftAct = new QAction(tr("Move Left"), this);
    moveLeftAct->setObjectName("moveLeft");
    connect(moveLeftAct, SIGNAL(triggered()), this, SLOT(moveLeft()));
    moveRightAct = new QAction(tr("Move Right"), this);
    moveRightAct->setObjectName("moveRight");
    connect(moveRightAct, SIGNAL(triggered()), this, SLOT(moveRight()));
    moveUpAct = new QAction(tr("Move Up"), this);
    moveUpAct->setObjectName("moveUp");
    connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveUp()));
    moveDownAct = new QAction(tr("Move Down"), this);
    moveDownAct->setObjectName("moveDown");
    connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveDown()));

    invertSelectionAct = new QAction(tr("Invert Selection"), this);
    invertSelectionAct->setObjectName("invertSelection");
    connect(invertSelectionAct, SIGNAL(triggered()), thumbsViewer, SLOT(invertSelection()));

    filterImagesFocusAct = new QAction(tr("Filter by Name"), this);
    filterImagesFocusAct->setObjectName("filterImagesFocus");
    connect(filterImagesFocusAct, SIGNAL(triggered()), this, SLOT(filterImagesFocus()));
    setPathFocusAct = new QAction(tr("Set Path"), this);
    setPathFocusAct->setObjectName("setPathFocus");
    connect(setPathFocusAct, SIGNAL(triggered()), this, SLOT(setPathFocus()));
}

void Phototonic::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actionIncludeSubDirectories);
    fileMenu->addAction(actionCreateDirectory);
    fileMenu->addAction(showClipboardAction);
    fileMenu->addAction(addBookmarkAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(copyToAction);
    editMenu->addAction(moveToAction);
    editMenu->addAction(actionPaste);
    editMenu->addAction(renameAction);
    editMenu->addAction(deleteAction);
    editMenu->addSeparator();
    editMenu->addAction(selectAllAction);
    editMenu->addAction(invertSelectionAct);
    addAction(filterImagesFocusAct);
    addAction(setPathFocusAct);
    editMenu->addSeparator();
    editMenu->addAction(actionSettings);

    goMenu = menuBar()->addMenu(tr("&Go"));
    goMenu->addAction(goBackAction);
    goMenu->addAction(goFrwdAction);
    goMenu->addAction(goUpAction);
    goMenu->addAction(goHomeAction);
    goMenu->addSeparator();
    goMenu->addAction(actionThumbsGoToTop);
    goMenu->addAction(actionThumbsGoToBottom);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(slideShowAction);
    viewMenu->addSeparator();

    viewMenu->addAction(thumbsZoomInAct);
    viewMenu->addAction(thumbsZoomOutAct);
    sortMenu = viewMenu->addMenu(tr("Sort By"));
    sortTypesGroup = new QActionGroup(this);
    sortTypesGroup->addAction(actionSortByName);
    sortTypesGroup->addAction(actionSortByTime);
    sortTypesGroup->addAction(actionSortBySize);
    sortTypesGroup->addAction(actionSortByType);
    sortMenu->addActions(sortTypesGroup->actions());
    sortMenu->addSeparator();
    sortMenu->addAction(actionSortReverse);
    viewMenu->addSeparator();

    viewMenu->addAction(actionShowHiddenFiles);
    viewMenu->addSeparator();
    viewMenu->addAction(actionRefresh);

    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(findDupesAction);

    // thumbview context menu
    thumbsViewer->addAction(openAction);
    thumbsViewer->addAction(openWithMenuAct);
    thumbsViewer->addAction(cutAction);
    thumbsViewer->addAction(copyAction);
    thumbsViewer->addAction(actionPaste);
    addMenuSeparator(thumbsViewer);
    thumbsViewer->addAction(copyToAction);
    thumbsViewer->addAction(moveToAction);
    thumbsViewer->addAction(renameAction);
    thumbsViewer->addAction(deleteAction);
    addMenuSeparator(thumbsViewer);
    thumbsViewer->addAction(selectAllAction);
    thumbsViewer->addAction(invertSelectionAct);
    thumbsViewer->setContextMenuPolicy(Qt::ActionsContextMenu);
    menuBar()->setVisible(true);
}

void Phototonic::createToolBars() {
    /* Edit */
    editToolBar = addToolBar(tr("Edit Toolbar"));
    editToolBar->setObjectName("Edit");
    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(actionPaste);
    editToolBar->addAction(deleteAction);
    editToolBar->addAction(showClipboardAction);
    connect(editToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setEditToolBarVisibility()));

    /* Navigation */
    goToolBar = addToolBar(tr("Navigation Toolbar"));
    goToolBar->setObjectName("Navigation");
    goToolBar->addAction(goBackAction);
    goToolBar->addAction(goFrwdAction);
    goToolBar->addAction(goUpAction);
    goToolBar->addAction(goHomeAction);
    goToolBar->addAction(actionRefresh);

    /* path bar */
    pathLineEdit = new QLineEdit;
    pathLineEdit->setCompleter(new DirCompleter(pathLineEdit));
    pathLineEdit->setMinimumWidth(200);
    pathLineEdit->setMaximumWidth(300);
    connect(pathLineEdit, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
    goToolBar->addWidget(pathLineEdit);
    goToolBar->addAction(actionIncludeSubDirectories);
    goToolBar->addAction(findDupesAction);
    connect(goToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setGoToolBarVisibility()));

    /* View */
    viewToolBar = addToolBar(tr("View Toolbar"));
    viewToolBar->setObjectName("View");
    viewToolBar->addAction(slideShowAction);
    viewToolBar->addAction(thumbsZoomInAct);
    viewToolBar->addAction(thumbsZoomOutAct);

    /* filter bar */
    QAction *filterAct = new QAction(tr("Filter"), this);
    filterAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
    connect(filterAct, SIGNAL(triggered()), this, SLOT(setThumbsFilter()));
    filterLineEdit = new QLineEdit;
    filterLineEdit->setMinimumWidth(100);
    filterLineEdit->setMaximumWidth(200);
    connect(filterLineEdit, SIGNAL(returnPressed()), this, SLOT(setThumbsFilter()));
    connect(filterLineEdit, SIGNAL(textChanged(
                                           const QString&)), this, SLOT(clearThumbsFilter()));
    filterLineEdit->setClearButtonEnabled(true);
    filterLineEdit->addAction(filterAct, QLineEdit::LeadingPosition);

    viewToolBar->addSeparator();
    viewToolBar->addWidget(filterLineEdit);
    viewToolBar->addAction(actionSettings);
    connect(viewToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setViewToolBarVisibility()));

    /* image */
    imageToolBar = new QToolBar(tr("Image Toolbar"));
    imageToolBar->setObjectName("Image");
    imageToolBar->addAction(prevImageAction);
    imageToolBar->addAction(nextImageAction);
    imageToolBar->addAction(firstImageAction);
    imageToolBar->addAction(lastImageAction);
    imageToolBar->addAction(slideShowAction);
    imageToolBar->addSeparator();
    imageToolBar->addAction(saveAction);
    imageToolBar->addAction(saveAsAction);
    imageToolBar->addAction(deleteAction);
    imageToolBar->addSeparator();
    imageToolBar->addAction(zoomInAct);
    imageToolBar->addAction(zoomOutAct);
    imageToolBar->addAction(resetZoomAct);
    imageToolBar->addAction(origZoomAct);
    imageToolBar->addSeparator();
    imageToolBar->addAction(resizeAct);
    imageToolBar->addAction(rotateRightAct);
    imageToolBar->addAction(rotateLeftAct);
    imageToolBar->addAction(flipHAct);
    imageToolBar->addAction(flipVAct);
    imageToolBar->addAction(cropAct);
    imageToolBar->addAction(colorsAct);
    imageToolBar->setVisible(false);
    connect(imageToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setImageToolBarVisibility()));

    setToolbarIconSize();
}

void Phototonic::setToolbarIconSize() {
    if (initComplete) {
        Settings::smallToolbarIcons = actionSmallToolbarIcons->isChecked();
    }
    int iconSize = Settings::smallToolbarIcons ? 16 : 24;
    QSize iconQSize(iconSize, iconSize);

    editToolBar->setIconSize(iconQSize);
    goToolBar->setIconSize(iconQSize);
    viewToolBar->setIconSize(iconQSize);
    imageToolBar->setIconSize(iconQSize);
}

void Phototonic::createStatusBar() {
    statusLabel = new QLabel(tr("Initializing..."));
    statusBar()->addWidget(statusLabel);

    busyMovie = new QMovie(":/images/busy.gif");
    busyLabel = new QLabel(this);
    busyLabel->setMovie(busyMovie);
    statusBar()->addWidget(busyLabel);
    busyLabel->setVisible(false);

    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
}

void Phototonic::createFileSystemTree() {
    fileSystemDock = new QDockWidget(tr("File System"), this);
    fileSystemDock->setObjectName("File System");
    fileSystemTree = new FileSystemTree(fileSystemDock);
    fileSystemDock->setWidget(fileSystemTree);
    connect(fileSystemDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setFsDockVisibility()));
    connect(fileSystemDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setFsDockVisibility()));
    addDockWidget(Qt::LeftDockWidgetArea, fileSystemDock);

    // Context menu
    fileSystemTree->addAction(openAction);
    fileSystemTree->addAction(actionCreateDirectory);
    fileSystemTree->addAction(renameAction);
    fileSystemTree->addAction(deleteAction);
    addMenuSeparator(fileSystemTree);
    fileSystemTree->addAction(actionPaste);
    addMenuSeparator(fileSystemTree);
    fileSystemTree->addAction(openWithMenuAct);
    fileSystemTree->addAction(addBookmarkAction);
    fileSystemTree->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(fileSystemTree, SIGNAL(clicked(
                                           const QModelIndex&)), this, SLOT(goSelectedDir(
                                                                                    const QModelIndex &)));

    connect(fileSystemTree->fsModel, SIGNAL(rowsRemoved(
                                                    const QModelIndex &, int, int)),
            this, SLOT(checkDirState(
                               const QModelIndex &, int, int)));

    connect(fileSystemTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
            this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

    fileSystemTree->setCurrentIndex(fileSystemTree->fsModel->index(QDir::currentPath()));

    connect(fileSystemTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(updateActions()));
}

void Phototonic::createBookmarks() {
    bookmarksDock = new QDockWidget(tr("Bookmarks"), this);
    bookmarksDock->setObjectName("Bookmarks");
    bookmarks = new BookMarks(bookmarksDock);
    bookmarksDock->setWidget(bookmarks);

    connect(bookmarksDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setBmDockVisibility()));
    connect(bookmarksDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setBmDockVisibility()));
    connect(bookmarks, SIGNAL(itemClicked(QTreeWidgetItem * , int)),
            this, SLOT(bookmarkClicked(QTreeWidgetItem * , int)));
    connect(removeBookmarkAction, SIGNAL(triggered()), bookmarks, SLOT(removeBookmark()));
    connect(bookmarks, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
            this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));
    connect(bookmarks, SIGNAL(itemSelectionChanged()),
            this, SLOT(updateActions()));
    addDockWidget(Qt::LeftDockWidgetArea, bookmarksDock);

    bookmarks->addAction(actionPaste);
    bookmarks->addAction(removeBookmarkAction);
    bookmarks->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::createImageTags() {
    tagsDock = new QDockWidget(tr("Tags"), this);
    tagsDock->setObjectName("Tags");
    thumbsViewer->imageTags = new ImageTags(tagsDock, thumbsViewer, metadataCache);
    tagsDock->setWidget(thumbsViewer->imageTags);

    connect(tagsDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setTagsDockVisibility()));
    connect(tagsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setTagsDockVisibility()));
    connect(thumbsViewer->imageTags, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
    connect(thumbsViewer->imageTags, SIGNAL(reloadThumbs()), this, SLOT(reloadThumbsSlot()));
    connect(thumbsViewer->imageTags->removeTagAction, SIGNAL(triggered()), this, SLOT(deleteOp()));
}

void Phototonic::sortThumbnails() {
    thumbsViewer->thumbsSortFlags = QDir::IgnoreCase;

    if (actionSortByName->isChecked()) {
        thumbsViewer->thumbsSortFlags |= QDir::Name;
    } else if (actionSortByTime->isChecked()) {
        thumbsViewer->thumbsSortFlags |= QDir::Time;
    } else if (actionSortBySize->isChecked()) {
        thumbsViewer->thumbsSortFlags |= QDir::Size;
    } else if (actionSortByType->isChecked()) {
        thumbsViewer->thumbsSortFlags |= QDir::Type;
    }

    if (actionSortReverse->isChecked()) {
        thumbsViewer->thumbsSortFlags |= QDir::Reversed;
    }
    refreshThumbs(false);
}

void Phototonic::reload() {
    findDupesAction->setChecked(false);
    if (Settings::layoutMode == ThumbViewWidget) {
        refreshThumbs(false);
    } else {
        imageViewer->reload();
    }
}

void Phototonic::setIncludeSubFolders() {
    findDupesAction->setChecked(false);
    Settings::includeSubDirectories = actionIncludeSubDirectories->isChecked();
    refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop) {
    thumbsViewer->setNeedScroll(scrollToTop);
    QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
}

void Phototonic::showHiddenFiles() {
    Settings::showHiddenFiles = actionShowHiddenFiles->isChecked();
    fileSystemTree->setModelFlags();
    refreshThumbs(false);
}

void Phototonic::toggleImageViewerToolbar() {
    imageToolBar->setVisible(actionShowViewerToolbar->isChecked());
    addToolBar(imageToolBar);
    Settings::showViewerToolbar = actionShowViewerToolbar->isChecked();
}

void Phototonic::about() {
    QString aboutString = "<h2>" + QString(VERSION) + "</h2>"
                          + tr("<p>Image viewer and organizer</p>")
                          + "Qt v" + QT_VERSION_STR
                          + "<p><a href=\"http://oferkv.github.io/phototonic/\">" + tr("Home page") + "</a></p>"
                          + "<p><a href=\"https://github.com/oferkv/phototonic/issues\">" + tr("Bug reports") +
                          "</a></p>"
                          + "<p></p>"
                          + "<table><tr><td>Code:</td><td>Ofer Kashayov</td><td>(oferkv@gmail.com)</td></tr>"
                          + "<tr><td></td><td>Christopher Roy Bratusek</td><td>(nano@jpberlin.de)</td></tr>"
                          + "<tr><td></td><td>Krzysztof Pyrkosz</td><td>(pyrkosz@o2.pl)</td></tr>"
                          + QString::fromUtf8(
            "<tr><td></td><td>Thomas L\u00FCbking</td><td>(thomas.luebking@gmail.com)</td></tr>")
                          + "<tr><td></td><td>Tung Le</td><td>(https://github.com/everbot)</td></tr>"
                          + "<tr><td></td><td>Peter Mattern</td><td>(https://github.com/pmattern)</td></tr>"
                          + QString::fromUtf8(
            "<tr><td>Bosnian:</td><td>Dino Duratovi\u0107</td><td>(dinomol@mail.com)</td></tr>")
                          + QString::fromUtf8(
            "<tr><td>Croatian:</td><td>Dino Duratovi\u0107</td><td>(dinomol@mail.com)</td></tr>")
                          + "<tr><td>Czech:</td><td>Pavel Fric</td><td>(pavelfric@seznam.cz)</td></tr>"
                          +
                          "<tr><td>French:</td><td>Adrien Daugabel</td><td>(adrien.d@mageialinux-online.org)</td></tr>"
                          + "<tr><td></td><td>David Geiger</td><td>(david.david@mageialinux-online.org)</td></tr>"
                          + QString::fromUtf8(
            "<tr><td></td><td>R\u00E9mi Verschelde</td><td>(akien@mageia.org)</td></tr>")
                          +
                          "<tr><td>German:</td><td>Jonathan Hooverman</td><td>(jonathan.hooverman@gmail.com)</td></tr>"
                          + QString::fromUtf8(
            "<tr><td>Polish:</td><td>Robert Wojew\u00F3dzki</td><td>(robwoj44@poczta.onet.pl)</td></tr>")
                          + QString::fromUtf8("<tr><td></td><td>Krzysztof Pyrkosz</td><td>(pyrkosz@o2.pl)</td></tr>")
                          + "<tr><td>Russian:</td><td>Ilya Alexandrovich</td><td>(yast4ik@gmail.com)</td></tr>"
                          + QString::fromUtf8(
            "<tr><td>Serbian:</td><td>Dino Duratovi\u0107</td><td>(dinomol@mail.com)</td></tr></table>")
                          + "<p>Phototonic is licensed under the GNU General Public License version 3</p>"
                          + "<p>Copyright &copy; 2013-2015 Ofer Kashayov</p>";

    QMessageBox::about(this, tr("About") + " Phototonic", aboutString);
}

void Phototonic::filterImagesFocus() {
    if (Settings::layoutMode == ThumbViewWidget) {
        if (!viewToolBar->isVisible())
            viewToolBar->setVisible(true);
        setViewToolBarVisibility();
        filterLineEdit->setFocus(Qt::OtherFocusReason);
        filterLineEdit->selectAll();
    }
}

void Phototonic::setPathFocus() {
    if (Settings::layoutMode == ThumbViewWidget) {
        if (!goToolBar->isVisible())
            goToolBar->setVisible(true);
        setGoToolBarVisibility();
        pathLineEdit->setFocus(Qt::OtherFocusReason);
        pathLineEdit->selectAll();
    }
}

void Phototonic::cleanupSender() {
    delete QObject::sender();
}

void Phototonic::externalAppError() {
    QMessageBox msgBox;
    msgBox.critical(this, tr("Error"), tr("Failed to start external application."));
}

void Phototonic::runExternalApp() {
    QString execCommand;
    QString selectedFileNames("");
    execCommand = Settings::externalApps[((QAction *) sender())->text()];

    if (Settings::layoutMode == ImageViewWidget) {
        if (imageViewer->isNewImage()) {
            showNewImageWarning(this);
            return;
        }

        execCommand += " \"" + imageViewer->currentImageFullPath + "\"";
    } else {
        if (QApplication::focusWidget() == fileSystemTree) {
            selectedFileNames += " \"" + getSelectedPath() + "\"";
        } else {

            QModelIndexList selectedIdxList = thumbsViewer->selectionModel()->selectedIndexes();
            if (selectedIdxList.size() < 1) {
                setStatus(tr("Invalid selection."));
                return;
            }

            selectedFileNames += " ";
            for (int tn = selectedIdxList.size() - 1; tn >= 0; --tn) {
                selectedFileNames += "\"" +
                                     thumbsViewer->thumbsViewerModel->item(selectedIdxList[tn].row())->data(
                                             thumbsViewer->FileNameRole).toString();
                if (tn)
                    selectedFileNames += "\" ";
            }
        }

        execCommand += selectedFileNames;
    }

    QProcess *externalProcess = new QProcess();
    connect(externalProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(cleanupSender()));
    connect(externalProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(externalAppError()));
    externalProcess->start(execCommand);
}

void Phototonic::updateExternalApps() {
    int actionNum = 0;
    QMapIterator<QString, QString> eaIter(Settings::externalApps);

    QList<QAction *> actionList = openWithSubMenu->actions();
    if (!actionList.empty()) {

        for (int i = 0; i < actionList.size(); ++i) {
            QAction *action = actionList.at(i);
            if (action->isSeparator()) {
                break;
            }

            openWithSubMenu->removeAction(action);
            imageViewer->removeAction(action);
            delete action;
        }

        openWithSubMenu->clear();
    }

    while (eaIter.hasNext()) {
        ++actionNum;
        eaIter.next();
        QAction *extAppAct = new QAction(eaIter.key(), this);
        if (actionNum < 10) {
            extAppAct->setShortcut(QKeySequence("Alt+" + QString::number(actionNum)));
        }
        extAppAct->setIcon(QIcon::fromTheme(eaIter.key()));
        connect(extAppAct, SIGNAL(triggered()), this, SLOT(runExternalApp()));
        openWithSubMenu->addAction(extAppAct);
        imageViewer->addAction(extAppAct);
    }

    openWithSubMenu->addSeparator();
    openWithSubMenu->addAction(chooseAppAct);
}

void Phototonic::chooseExternalApp() {
    AppMgmtDialog *dialog = new AppMgmtDialog(this);

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    dialog->exec();
    updateExternalApps();
    delete (dialog);

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::showSettings() {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    imageViewer->setCursorHiding(false);

    SettingsDialog *dialog = new SettingsDialog(this);
    if (dialog->exec()) {
        imageViewer->setBgColor();
        thumbsViewer->setThumbColors();
        Settings::imageZoomFactor = 1.0;
        imageViewer->infoLabel->setVisible(Settings::enableImageInfoFS);

        if (Settings::layoutMode == ImageViewWidget) {
            imageViewer->reload();
            needThumbsRefresh = true;
        } else {
            refreshThumbs(false);
        }
    }

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
    delete dialog;
}

void Phototonic::toggleFullScreen() {
    if (actionFullScreen->isChecked()) {
        shouldMaximize = isMaximized();
        showFullScreen();
        Settings::isFullScreen = true;
        imageViewer->setCursorHiding(true);
    } else {
        showNormal();
        if (shouldMaximize)
            showMaximized();
        imageViewer->setCursorHiding(false);
        Settings::isFullScreen = false;
    }
}

void Phototonic::selectAllThumbs() {
    thumbsViewer->selectAll();
}

void Phototonic::copyOrCutThumbs(bool copy) {
    Settings::copyCutIdxList = thumbsViewer->selectionModel()->selectedIndexes();
    copyCutCount = Settings::copyCutIdxList.size();

    Settings::copyCutFileList.clear();
    for (int tn = 0; tn < copyCutCount; ++tn) {
        Settings::copyCutFileList.append(thumbsViewer->thumbsViewerModel->item(Settings::copyCutIdxList[tn].
                row())->data(thumbsViewer->FileNameRole).toString());
    }

    Settings::copyOp = copy;
    actionPaste->setEnabled(true);
}

void Phototonic::cutThumbs() {
    copyOrCutThumbs(false);
}

void Phototonic::copyThumbs() {
    copyOrCutThumbs(true);
}

void Phototonic::copyImagesTo() {
    copyMoveImages(false);
}

void Phototonic::moveImagesTo() {
    copyMoveImages(true);
}

void Phototonic::copyMoveImages(bool move) {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    copyMoveToDialog = new CopyMoveToDialog(this, getSelectedPath(), move);
    if (copyMoveToDialog->exec()) {
        if (Settings::layoutMode == ThumbViewWidget) {
            if (copyMoveToDialog->copyOp) {
                copyThumbs();
            } else {
                cutThumbs();
            }

            pasteThumbs();
        } else {
            if (imageViewer->isNewImage()) {
                showNewImageWarning(this);
                if (isFullScreen()) {
                    imageViewer->setCursorHiding(true);
                }

                return;
            }

            QFileInfo fileInfo = QFileInfo(imageViewer->currentImageFullPath);
            QString fileName = fileInfo.fileName();
            QString destFile = copyMoveToDialog->selectedPath + QDir::separator() + fileInfo.fileName();

            int res = copyMoveFile(copyMoveToDialog->copyOp, fileName, imageViewer->currentImageFullPath,
                                   destFile, copyMoveToDialog->selectedPath);

            if (!res) {
                QMessageBox msgBox;
                msgBox.critical(this, tr("Error"), tr("Failed to copy or move image."));
            } else {
                if (!copyMoveToDialog->copyOp) {
                    int currentRow = thumbsViewer->getCurrentRow();
                    thumbsViewer->thumbsViewerModel->removeRow(currentRow);
                    loadCurrentImage(currentRow);
                }
            }
        }
    }

    bookmarks->reloadBookmarks();
    delete (copyMoveToDialog);
    copyMoveToDialog = 0;

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::thumbsZoomIn() {
    if (thumbsViewer->thumbSize < THUMB_SIZE_MAX) {
        thumbsViewer->thumbSize += 50;
        thumbsZoomOutAct->setEnabled(true);
        if (thumbsViewer->thumbSize == THUMB_SIZE_MAX)
            thumbsZoomInAct->setEnabled(false);
        refreshThumbs(false);
    }
}

void Phototonic::thumbsZoomOut() {
    if (thumbsViewer->thumbSize > THUMB_SIZE_MIN) {
        thumbsViewer->thumbSize -= 50;
        thumbsZoomInAct->setEnabled(true);
        if (thumbsViewer->thumbSize == THUMB_SIZE_MIN)
            thumbsZoomOutAct->setEnabled(false);
        refreshThumbs(false);
    }
}

void Phototonic::zoomOut() {
    if (Settings::imageZoomFactor <= 4.0 && Settings::imageZoomFactor > 0.25) {
        Settings::imageZoomFactor -= 0.25;
    } else if (Settings::imageZoomFactor <= 8.0 && Settings::imageZoomFactor >= 4.0) {
        Settings::imageZoomFactor -= 0.50;
    } else if (Settings::imageZoomFactor <= 16.0 && Settings::imageZoomFactor >= 8.0) {
        Settings::imageZoomFactor -= 1.0;
    } else {
        imageViewer->setFeedback(tr("Minimum zoom"));
        return;
    }

    imageViewer->tempDisableResize = false;
    imageViewer->resizeImage();
    imageViewer->setFeedback(tr("Zoom %1%").arg(QString::number(Settings::imageZoomFactor * 100)));
}

void Phototonic::zoomIn() {
    if (Settings::imageZoomFactor < 4.0 && Settings::imageZoomFactor >= 0.25) {
        Settings::imageZoomFactor += 0.25;
    } else if (Settings::imageZoomFactor < 8.0 && Settings::imageZoomFactor >= 4.0) {
        Settings::imageZoomFactor += 0.50;
    } else if (Settings::imageZoomFactor < 16.0 && Settings::imageZoomFactor >= 8.0) {
        Settings::imageZoomFactor += 1.00;
    } else {
        imageViewer->setFeedback(tr("Maximum zoom"));
        return;
    }

    imageViewer->tempDisableResize = false;
    imageViewer->resizeImage();
    imageViewer->setFeedback(tr("Zoom %1%").arg(QString::number(Settings::imageZoomFactor * 100)));
}

void Phototonic::resetZoom() {
    Settings::imageZoomFactor = 1.0;
    imageViewer->tempDisableResize = false;
    imageViewer->resizeImage();
    imageViewer->setFeedback(tr("Zoom Reset"));
}

void Phototonic::origZoom() {
    Settings::imageZoomFactor = 1.0;
    imageViewer->tempDisableResize = true;
    imageViewer->resizeImage();
    imageViewer->setFeedback(tr("Original Size"));
}

void Phototonic::keepZoom() {
    Settings::keepZoomFactor = keepZoomAct->isChecked();
    if (Settings::keepZoomFactor) {
        imageViewer->setFeedback(tr("Zoom Locked"));
    } else {
        imageViewer->setFeedback(tr("Zoom Unlocked"));
    }
}

void Phototonic::keepTransformClicked() {
    Settings::keepTransform = keepTransformAct->isChecked();

    if (Settings::keepTransform) {
        imageViewer->setFeedback(tr("Transformations Locked"));
        if (cropDialog)
            cropDialog->applyCrop(0);
    } else {
        Settings::cropLeftPercent = Settings::cropTopPercent = Settings::cropWidthPercent = Settings::cropHeightPercent = 0;
        imageViewer->setFeedback(tr("Transformations Unlocked"));
    }

    imageViewer->refresh();
}

void Phototonic::rotateLeft() {
    Settings::rotation -= 90;
    if (Settings::rotation < 0)
        Settings::rotation = 270;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Rotation %1°").arg(QString::number(Settings::rotation)));
}

void Phototonic::rotateRight() {
    Settings::rotation += 90;
    if (Settings::rotation > 270)
        Settings::rotation = 0;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Rotation %1°").arg(QString::number(Settings::rotation)));
}

void Phototonic::flipVert() {
    Settings::flipV = !Settings::flipV;
    imageViewer->refresh();
    imageViewer->setFeedback(Settings::flipV ? tr("Flipped Vertically") : tr("Unflipped Vertically"));
}

void Phototonic::flipHoriz() {
    Settings::flipH = !Settings::flipH;
    imageViewer->refresh();
    imageViewer->setFeedback(Settings::flipH ? tr("Flipped Horizontally") : tr("Unflipped Horizontally"));
}

void Phototonic::cropImage() {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    if (!cropDialog) {
        cropDialog = new CropDialog(this, imageViewer);
        connect(cropDialog, SIGNAL(accepted()), this, SLOT(cleanupCropDialog()));
        connect(cropDialog, SIGNAL(rejected()), this, SLOT(cleanupCropDialog()));
    }

    cropDialog->show();
    setInterfaceEnabled(false);
    cropDialog->applyCrop(0);
}

void Phototonic::scaleImage() {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    if (Settings::layoutMode == ThumbViewWidget && thumbsViewer->selectionModel()->selectedIndexes().size() < 1) {
        setStatus(tr("No selection"));
        return;
    }

    resizeDialog = new ResizeDialog(this, imageViewer);
    connect(resizeDialog, SIGNAL(accepted()), this, SLOT(cleanupScaleDialog()));
    connect(resizeDialog, SIGNAL(rejected()), this, SLOT(cleanupScaleDialog()));

    resizeDialog->show();
    setInterfaceEnabled(false);
}

void Phototonic::freeRotateLeft() {
    --Settings::rotation;
    if (Settings::rotation < 0)
        Settings::rotation = 359;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Rotation %1°").arg(QString::number(Settings::rotation)));
}

void Phototonic::freeRotateRight() {
    ++Settings::rotation;
    if (Settings::rotation > 360)
        Settings::rotation = 1;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Rotation %1°").arg(QString::number(Settings::rotation)));
}

void Phototonic::showColorsDialog() {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    if (!colorsDialog) {
        colorsDialog = new ColorsDialog(this, imageViewer);
        connect(colorsDialog, SIGNAL(accepted()), this, SLOT(cleanupColorsDialog()));
        connect(colorsDialog, SIGNAL(rejected()), this, SLOT(cleanupColorsDialog()));
    }

    Settings::colorsActive = true;
    colorsDialog->show();
    colorsDialog->applyColors(0);
    setInterfaceEnabled(false);
}

void Phototonic::moveRight() {
    imageViewer->keyMoveEvent(ImageViewer::MoveRight);
}

void Phototonic::moveLeft() {
    imageViewer->keyMoveEvent(ImageViewer::MoveLeft);
}

void Phototonic::moveUp() {
    imageViewer->keyMoveEvent(ImageViewer::MoveUp);
}

void Phototonic::moveDown() {
    imageViewer->keyMoveEvent(ImageViewer::MoveDown);
}

void Phototonic::setMirrorDisabled() {
    imageViewer->mirrorLayout = ImageViewer::LayNone;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Mirroring Disabled"));
}

void Phototonic::setMirrorDual() {
    imageViewer->mirrorLayout = ImageViewer::LayDual;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Mirroring: Dual"));
}

void Phototonic::setMirrorTriple() {
    imageViewer->mirrorLayout = ImageViewer::LayTriple;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Mirroring: Triple"));
}

void Phototonic::setMirrorVDual() {
    imageViewer->mirrorLayout = ImageViewer::LayVDual;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Mirroring: Dual Vertical"));
}

void Phototonic::setMirrorQuad() {
    imageViewer->mirrorLayout = ImageViewer::LayQuad;
    imageViewer->refresh();
    imageViewer->setFeedback(tr("Mirroring: Quad"));
}

bool Phototonic::isValidPath(QString &path) {
    QDir checkPath(path);
    if (!checkPath.exists() || !checkPath.isReadable()) {
        return false;
    }
    return true;
}

void Phototonic::pasteThumbs() {
    if (!copyCutCount)
        return;

    QString destDir;
    if (copyMoveToDialog)
        destDir = copyMoveToDialog->selectedPath;
    else {
        if (QApplication::focusWidget() == bookmarks) {
            if (bookmarks->currentItem()) {
                destDir = bookmarks->currentItem()->toolTip(0);
            }
        } else {
            destDir = getSelectedPath();
        }
    }

    if (!isValidPath(destDir)) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Can not copy or move to ") + destDir);
        selectCurrentViewDir();
        return;
    }

    bool pasteInCurrDir = (Settings::currentViewDir == destDir);

    QFileInfo fileInfo;
    if (!Settings::copyOp && pasteInCurrDir) {
        for (int tn = 0; tn < Settings::copyCutFileList.size(); ++tn) {
            fileInfo = QFileInfo(Settings::copyCutFileList[tn]);
            if (fileInfo.absolutePath() == destDir) {
                QMessageBox msgBox;
                msgBox.critical(this, tr("Error"), tr("Can not copy or move to the same folder"));
                return;
            }
        }
    }

    CopyMoveDialog *dialog = new CopyMoveDialog(this);
    dialog->exec(thumbsViewer, destDir, pasteInCurrDir);
    if (pasteInCurrDir) {
        for (int tn = 0; tn < Settings::copyCutFileList.size(); ++tn) {
            thumbsViewer->addThumb(Settings::copyCutFileList[tn]);
        }
    } else {
        int row = dialog->latestRow;
        if (thumbsViewer->thumbsViewerModel->rowCount()) {
            if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
                row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
            }

            thumbsViewer->setCurrentRow(row);
            thumbsViewer->selectThumbByRow(row);
        }
    }

    QString state = QString((Settings::copyOp ? tr("Copied") : tr("Moved")) + " " +
                            tr("%n image(s)", "", dialog->nFiles));
    setStatus(state);
    delete (dialog);
    selectCurrentViewDir();

    copyCutCount = 0;
    Settings::copyCutIdxList.clear();
    Settings::copyCutFileList.clear();
    actionPaste->setEnabled(false);

    thumbsViewer->loadVisibleThumbs();
}

void Phototonic::loadCurrentImage(int currentRow) {
    bool wrapImageListTmp = Settings::wrapImageList;
    Settings::wrapImageList = false;

    if (currentRow == thumbsViewer->thumbsViewerModel->rowCount()) {
        thumbsViewer->setCurrentRow(currentRow - 1);
    }

    if (thumbsViewer->getNextRow() < 0 && currentRow > 0) {
        imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(currentRow - 1)->
                data(thumbsViewer->FileNameRole).toString());
    } else {
        if (thumbsViewer->thumbsViewerModel->rowCount() == 0) {
            hideViewer();
            refreshThumbs(true);
            return;
        }

        if (currentRow > (thumbsViewer->thumbsViewerModel->rowCount() - 1))
            currentRow = thumbsViewer->thumbsViewerModel->rowCount() - 1;

        imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(currentRow)->
                data(thumbsViewer->FileNameRole).toString());
    }

    Settings::wrapImageList = wrapImageListTmp;
    thumbsViewer->setImageViewerWindowTitle();
}

void Phototonic::deleteViewerImage() {
    if (imageViewer->isNewImage()) {
        showNewImageWarning(this);
        return;
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    bool ok;
    QFileInfo fileInfo = QFileInfo(imageViewer->currentImageFullPath);
    QString fileName = fileInfo.fileName();

    bool deleteConfirmed = true;
    if (Settings::deleteConfirm) {
        QMessageBox msgBox;
        msgBox.setText(tr("Permanently delete") + " " + fileName);
        msgBox.setWindowTitle(tr("Delete image"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));

        if (msgBox.exec() != QMessageBox::Yes) {
            deleteConfirmed = false;
        }
    }

    if (deleteConfirmed) {
        int currentRow = thumbsViewer->getCurrentRow();

        ok = QFile::remove(imageViewer->currentImageFullPath);
        if (ok) {
            thumbsViewer->thumbsViewerModel->removeRow(currentRow);
            imageViewer->setFeedback(tr("Deleted ") + fileName);
        } else {
            QMessageBox msgBox;
            msgBox.critical(this, tr("Error"), tr("Failed to delete image"));
            if (isFullScreen()) {
                imageViewer->setCursorHiding(true);
            }
            return;
        }

        loadCurrentImage(currentRow);
    }
    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::deleteOp() {
    if (QApplication::focusWidget() == thumbsViewer->imageTags->tagsTree) {
        thumbsViewer->imageTags->removeTag();
        return;
    }

    if (QApplication::focusWidget() == bookmarks) {
        bookmarks->removeBookmark();
        return;
    }

    if (QApplication::focusWidget() == fileSystemTree) {
        deleteDir();
        return;
    }

    if (Settings::layoutMode == ImageViewWidget) {
        deleteViewerImage();
        return;
    }

    if (thumbsViewer->selectionModel()->selectedIndexes().size() < 1) {
        setStatus(tr("No selection"));
        return;
    }

    // deleting from thumbnail viewer
    bool deleteConfirmed = true;
    if (Settings::deleteConfirm) {
        QMessageBox msgBox;
        msgBox.setText(tr("Permanently delete selected images?"));
        msgBox.setWindowTitle(tr("Delete images"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));

        if (msgBox.exec() != QMessageBox::Yes) {
            deleteConfirmed = false;
        }
    }

    if (deleteConfirmed) {
        QModelIndexList indexesList;
        int nfiles = 0;
        bool ok;
        QList<int> rows;
        int row;

        ProgressDialog *dialog = new ProgressDialog(this);
        dialog->show();

        while ((indexesList = thumbsViewer->selectionModel()->selectedIndexes()).size()) {
            QString fileName = thumbsViewer->thumbsViewerModel->item(
                    indexesList.first().row())->data(thumbsViewer->FileNameRole).toString();
            dialog->opLabel->setText("Deleting " + fileName);
            ok = QFile::remove(fileName);

            ++nfiles;
            if (ok) {
                row = indexesList.first().row();
                rows << row;
                thumbsViewer->thumbsViewerModel->removeRow(row);
            } else {
                QMessageBox msgBox;
                msgBox.critical(this, tr("Error"), tr("Failed to delete image."));
                break;
            }
            if (thumbsViewer->thumbsViewerModel->rowCount() > 0) {
                thumbsViewer->setRowHidden(0, false);
            }

            if (dialog->abortOp) {
                break;
            }
        }

        if (thumbsViewer->thumbsViewerModel->rowCount()) {

            qSort(rows.begin(), rows.end());
            row = rows.at(0);

            if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
                row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
            }

            thumbsViewer->setCurrentRow(row);
            thumbsViewer->selectThumbByRow(row);
        }

        dialog->close();
        delete (dialog);

        QString state = QString(tr("Deleted") + " " + tr("%n image(s)", "", nfiles));
        setStatus(state);
    }
}

void Phototonic::goTo(QString path) {
    findDupesAction->setChecked(false);
    thumbsViewer->setNeedScroll(true);
    fileSystemTree->setCurrentIndex(fileSystemTree->fsModel->index(path));
    Settings::currentViewDir = path;
    refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex &idx) {
    findDupesAction->setChecked(false);
    thumbsViewer->setNeedScroll(true);
    Settings::currentViewDir = getSelectedPath();
    refreshThumbs(true);
    fileSystemTree->expand(idx);
}

void Phototonic::goPathBarDir() {
    findDupesAction->setChecked(false);
    thumbsViewer->setNeedScroll(true);

    QDir checkPath(pathLineEdit->text());
    if (!checkPath.exists() || !checkPath.isReadable()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Invalid Path:") + " " + pathLineEdit->text());
        pathLineEdit->setText(Settings::currentViewDir);
        return;
    }

    Settings::currentViewDir = pathLineEdit->text();
    refreshThumbs(true);
    selectCurrentViewDir();
}

void Phototonic::bookmarkClicked(QTreeWidgetItem *item, int col) {
    goTo(item->toolTip(col));
}

void Phototonic::setThumbsFilter() {
    thumbsViewer->filterStr = filterLineEdit->text();
    refreshThumbs(true);
}

void Phototonic::clearThumbsFilter() {
    if (filterLineEdit->text() == "") {
        thumbsViewer->filterStr = filterLineEdit->text();
        refreshThumbs(true);
    }
}

void Phototonic::goBack() {
    if (currentHistoryIdx > 0) {
        needHistoryRecord = false;
        goTo(pathHistoryList.at(--currentHistoryIdx));
        goFrwdAction->setEnabled(true);
        if (currentHistoryIdx == 0)
            goBackAction->setEnabled(false);
    }
}

void Phototonic::goForward() {

    if (currentHistoryIdx < pathHistoryList.size() - 1) {
        needHistoryRecord = false;
        goTo(pathHistoryList.at(++currentHistoryIdx));
        if (currentHistoryIdx == (pathHistoryList.size() - 1))
            goFrwdAction->setEnabled(false);
    }
}

void Phototonic::goUp() {
    QFileInfo fileInfo = QFileInfo(Settings::currentViewDir);
    goTo(fileInfo.dir().absolutePath());
}

void Phototonic::goHome() {
    goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled) {
    cutAction->setEnabled(setEnabled);
    copyAction->setEnabled(setEnabled);
}

void Phototonic::setDeleteAction(bool setEnabled) {
    deleteAction->setEnabled(setEnabled);
}

void Phototonic::updateActions() {
    if (QApplication::focusWidget() == thumbsViewer) {
        setCopyCutActions(thumbsViewer->selectionModel()->selectedIndexes().size());
        setDeleteAction(thumbsViewer->selectionModel()->selectedIndexes().size());
    } else if (QApplication::focusWidget() == bookmarks) {
        setCopyCutActions(false);
        setDeleteAction(bookmarks->selectionModel()->selectedIndexes().size());
    } else if (QApplication::focusWidget() == fileSystemTree) {
        setCopyCutActions(false);
        setDeleteAction(fileSystemTree->selectionModel()->selectedIndexes().size());
    } else if (Settings::layoutMode == ImageViewWidget || QApplication::focusWidget() == imageViewer->scrlArea) {
        setCopyCutActions(false);
        setDeleteAction(true);
    } else {
        setCopyCutActions(false);
        setDeleteAction(false);
    }

    if (Settings::layoutMode == ImageViewWidget && !interfaceDisabled) {
        setViewerKeyEventsEnabled(true);
        actionFullScreen->setEnabled(true);
        actionCloseImage->setEnabled(true);
    } else {
        if (QApplication::focusWidget() == imageViewer->scrlArea) {
            setViewerKeyEventsEnabled(true);
            actionFullScreen->setEnabled(false);
            actionCloseImage->setEnabled(false);
        } else {
            setViewerKeyEventsEnabled(false);
            actionFullScreen->setEnabled(false);
            actionCloseImage->setEnabled(false);
        }
    }
}

void Phototonic::writeSettings() {
    if (Settings::layoutMode == ThumbViewWidget) {
        Settings::appSettings->setValue("Geometry", saveGeometry());
        Settings::appSettings->setValue("WindowState", saveState());
    }

    Settings::appSettings->setValue(Settings::optionThumbsSortFlags, (int) thumbsViewer->thumbsSortFlags);
    Settings::appSettings->setValue(Settings::optionThumbsZoomLevel, (int) thumbsViewer->thumbSize);
    Settings::appSettings->setValue(Settings::optionFullScreenMode, (bool) Settings::isFullScreen);
    Settings::appSettings->setValue("backgroundColor", Settings::backgroundColor);
    Settings::appSettings->setValue("backgroundThumbColor", Settings::thumbsBackgroundColor);
    Settings::appSettings->setValue("textThumbColor", Settings::thumbsTextColor);
    Settings::appSettings->setValue("thumbPagesReadahead", (int) Settings::thumbPagesReadahead);
    Settings::appSettings->setValue("exitInsteadOfClose", (int) Settings::exitInsteadOfClose);
    Settings::appSettings->setValue("enableAnimations", (bool) Settings::enableAnimations);
    Settings::appSettings->setValue("exifRotationEnabled", (bool) Settings::exifRotationEnabled);
    Settings::appSettings->setValue("exifThumbRotationEnabled", (bool) Settings::exifThumbRotationEnabled);
    Settings::appSettings->setValue("reverseMouseBehavior", (bool) Settings::reverseMouseBehavior);
    Settings::appSettings->setValue("deleteConfirm", (bool) Settings::deleteConfirm);
    Settings::appSettings->setValue("showHiddenFiles", (bool) Settings::showHiddenFiles);
    Settings::appSettings->setValue("wrapImageList", (bool) Settings::wrapImageList);
    Settings::appSettings->setValue("imageZoomFactor", (float) Settings::imageZoomFactor);
    Settings::appSettings->setValue("shouldMaximize", (bool) isMaximized());
    Settings::appSettings->setValue("defaultSaveQuality", (int) Settings::defaultSaveQuality);
    Settings::appSettings->setValue("slideShowDelay", (int) Settings::slideShowDelay);
    Settings::appSettings->setValue("slideShowRandom", (bool) Settings::slideShowRandom);
    Settings::appSettings->setValue("editToolBarVisible", (bool) editToolBarVisible);
    Settings::appSettings->setValue("goToolBarVisible", (bool) goToolBarVisible);
    Settings::appSettings->setValue("viewToolBarVisible", (bool) viewToolBarVisible);
    Settings::appSettings->setValue("imageToolBarVisible", (bool) imageToolBarVisible);
    Settings::appSettings->setValue("fileSystemDockVisible", (bool) Settings::fileSystemDockVisible);
    Settings::appSettings->setValue("imageInfoDockVisible", (bool) Settings::imageInfoDockVisible);
    Settings::appSettings->setValue("bookmarksDockVisible", (bool) Settings::bookmarksDockVisible);
    Settings::appSettings->setValue("startupDir", (int) Settings::startupDir);
    Settings::appSettings->setValue("specifiedStartDir", Settings::specifiedStartDir);
    Settings::appSettings->setValue("thumbsBackImage", Settings::thumbsBackImage);
    Settings::appSettings->setValue("lastDir",
                                    Settings::startupDir == Settings::RememberLastDir ? Settings::currentViewDir : "");
    Settings::appSettings->setValue("enableImageInfoFS", (bool) Settings::enableImageInfoFS);
    Settings::appSettings->setValue("smallToolbarIcons", (bool) Settings::smallToolbarIcons);
    Settings::appSettings->setValue("hideDockTitlebars", (bool) Settings::hideDockTitlebars);
    Settings::appSettings->setValue("showViewerToolbar", (bool) Settings::showViewerToolbar);

    /* Action shortcuts */
    Settings::appSettings->beginGroup("Shortcuts");
    QMapIterator<QString, QAction *> scIter(Settings::actionKeys);
    while (scIter.hasNext()) {
        scIter.next();
        Settings::appSettings->setValue(scIter.key(), scIter.value()->shortcut().toString());
    }
    Settings::appSettings->endGroup();

    /* External apps */
    Settings::appSettings->beginGroup("ExternalApps");
    Settings::appSettings->remove("");
    QMapIterator<QString, QString> eaIter(Settings::externalApps);
    while (eaIter.hasNext()) {
        eaIter.next();
        Settings::appSettings->setValue(eaIter.key(), eaIter.value());
    }
    Settings::appSettings->endGroup();

    /* save bookmarks */
    int idx = 0;
    Settings::appSettings->beginGroup("CopyMoveToPaths");
    Settings::appSettings->remove("");
    QSetIterator<QString> pathsIter(Settings::bookmarkPaths);
    while (pathsIter.hasNext()) {
        Settings::appSettings->setValue("path" + QString::number(++idx), pathsIter.next());
    }
    Settings::appSettings->endGroup();

    /* save known Tags */
    idx = 0;
    Settings::appSettings->beginGroup("KnownTags");
    Settings::appSettings->remove("");
    QSetIterator<QString> tagsIter(Settings::knownTags);
    while (tagsIter.hasNext()) {
        Settings::appSettings->setValue("tag" + QString::number(++idx), tagsIter.next());
    }
    Settings::appSettings->endGroup();
}

void Phototonic::readSettings() {
    initComplete = false;
    needThumbsRefresh = false;

    if (!Settings::appSettings->contains(Settings::optionThumbsZoomLevel)) {
        resize(800, 600);
        Settings::appSettings->setValue(Settings::optionThumbsSortFlags, (int) 0);
        Settings::appSettings->setValue(Settings::optionThumbsZoomLevel, (int) 200);
        Settings::appSettings->setValue(Settings::optionFullScreenMode, (bool) false);
        Settings::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
        Settings::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
        Settings::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
        Settings::appSettings->setValue("thumbPagesReadahead", (int) 2);
        Settings::appSettings->setValue("zoomOutFlags", (int) 1);
        Settings::appSettings->setValue("zoomInFlags", (int) 0);
        Settings::appSettings->setValue("wrapImageList", (bool) false);
        Settings::appSettings->setValue("exitInsteadOfClose", (int) 0);
        Settings::appSettings->setValue("imageZoomFactor", (float) 1.0);
        Settings::appSettings->setValue("defaultSaveQuality", (int) 90);
        Settings::appSettings->setValue("noEnlargeSmallThumb", (bool) true);
        Settings::appSettings->setValue("enableAnimations", (bool) true);
        Settings::appSettings->setValue("exifRotationEnabled", (bool) true);
        Settings::appSettings->setValue("exifThumbRotationEnabled", (bool) false);
        Settings::appSettings->setValue("reverseMouseBehavior", (bool) false);
        Settings::appSettings->setValue("deleteConfirm", (bool) true);
        Settings::appSettings->setValue("showHiddenFiles", (bool) false);
        Settings::appSettings->setValue("slideShowDelay", (int) 5);
        Settings::appSettings->setValue("slideShowRandom", (bool) false);
        Settings::appSettings->setValue("editToolBarVisible", (bool) true);
        Settings::appSettings->setValue("goToolBarVisible", (bool) true);
        Settings::appSettings->setValue("viewToolBarVisible", (bool) true);
        Settings::appSettings->setValue("imageToolBarVisible", (bool) false);
        Settings::appSettings->setValue("fileSystemDockVisible", (bool) true);
        Settings::appSettings->setValue("bookmarksDockVisible", (bool) true);
        Settings::appSettings->setValue("imageInfoDockVisible", (bool) true);
        Settings::appSettings->setValue("enableImageInfoFS", (bool) true);
        Settings::appSettings->setValue("smallToolbarIcons", (bool) false);
        Settings::appSettings->setValue("hideDockTitlebars", (bool) false);
        Settings::appSettings->setValue("showViewerToolbar", (bool) false);
        Settings::appSettings->setValue("smallToolbarIcons", (bool) true);
        Settings::bookmarkPaths.insert(QDir::homePath());
    }

    Settings::backgroundColor = Settings::appSettings->value("backgroundColor").value<QColor>();
    Settings::exitInsteadOfClose = Settings::appSettings->value("exitInsteadOfClose").toBool();
    Settings::enableAnimations = Settings::appSettings->value("enableAnimations").toBool();
    Settings::exifRotationEnabled = Settings::appSettings->value("exifRotationEnabled").toBool();
    Settings::exifThumbRotationEnabled = Settings::appSettings->value("exifThumbRotationEnabled").toBool();
    Settings::reverseMouseBehavior = Settings::appSettings->value("reverseMouseBehavior").toBool();
    Settings::deleteConfirm = Settings::appSettings->value("deleteConfirm").toBool();
    Settings::showHiddenFiles = Settings::appSettings->value("showHiddenFiles").toBool();
    Settings::wrapImageList = Settings::appSettings->value("wrapImageList").toBool();
    Settings::imageZoomFactor = Settings::appSettings->value("imageZoomFactor").toFloat();
    Settings::zoomOutFlags = Settings::appSettings->value("zoomOutFlags").toInt();
    Settings::zoomInFlags = Settings::appSettings->value("zoomInFlags").toInt();
    Settings::rotation = 0;
    Settings::keepTransform = false;
    shouldMaximize = Settings::appSettings->value("shouldMaximize").toBool();
    Settings::flipH = false;
    Settings::flipV = false;
    Settings::defaultSaveQuality = Settings::appSettings->value("defaultSaveQuality").toInt();
    Settings::slideShowDelay = Settings::appSettings->value("slideShowDelay").toInt();
    Settings::slideShowRandom = Settings::appSettings->value("slideShowRandom").toBool();
    Settings::slideShowActive = false;
    editToolBarVisible = Settings::appSettings->value("editToolBarVisible").toBool();
    goToolBarVisible = Settings::appSettings->value("goToolBarVisible").toBool();
    viewToolBarVisible = Settings::appSettings->value("viewToolBarVisible").toBool();
    imageToolBarVisible = Settings::appSettings->value("imageToolBarVisible").toBool();
    Settings::fileSystemDockVisible = Settings::appSettings->value("fileSystemDockVisible").toBool();
    Settings::bookmarksDockVisible = Settings::appSettings->value("bookmarksDockVisible").toBool();
    Settings::imageInfoDockVisible = Settings::appSettings->value("imageInfoDockVisible").toBool();
    Settings::startupDir = (Settings::StartupDir) Settings::appSettings->value("startupDir").toInt();
    Settings::specifiedStartDir = Settings::appSettings->value("specifiedStartDir").toString();
    Settings::thumbsBackImage = Settings::appSettings->value("thumbsBackImage").toString();
    Settings::enableImageInfoFS = Settings::appSettings->value("enableImageInfoFS").toBool();
    Settings::smallToolbarIcons = Settings::appSettings->value("smallToolbarIcons").toBool();
    Settings::hideDockTitlebars = Settings::appSettings->value("hideDockTitlebars").toBool();
    Settings::showViewerToolbar = Settings::appSettings->value("showViewerToolbar").toBool();

    /* read external apps */
    Settings::appSettings->beginGroup("ExternalApps");
    QStringList extApps = Settings::appSettings->childKeys();
    for (int i = 0; i < extApps.size(); ++i) {
        Settings::externalApps[extApps.at(i)] = Settings::appSettings->value(extApps.at(i)).toString();
    }
    Settings::appSettings->endGroup();

    /* read bookmarks */
    Settings::appSettings->beginGroup("CopyMoveToPaths");
    QStringList paths = Settings::appSettings->childKeys();
    for (int i = 0; i < paths.size(); ++i) {
        Settings::bookmarkPaths.insert(Settings::appSettings->value(paths.at(i)).toString());
    }
    Settings::appSettings->endGroup();

    /* read known tags */
    Settings::appSettings->beginGroup("KnownTags");
    QStringList tags = Settings::appSettings->childKeys();
    for (int i = 0; i < tags.size(); ++i) {
        Settings::knownTags.insert(Settings::appSettings->value(tags.at(i)).toString());
    }
    Settings::appSettings->endGroup();
}

void Phototonic::setupDocks() {
    imageViewLayout = new QVBoxLayout;
    imageViewLayout->setContentsMargins(0, 0, 0, 0);
    imageViewLayout->addWidget(imageViewer);
    QWidget *imageViewContainerWidget = new QWidget;
    imageViewContainerWidget->setLayout(imageViewLayout);

    addDockWidget(Qt::RightDockWidgetArea, imageInfoDock);
    addDockWidget(Qt::RightDockWidgetArea, tagsDock);

    menuBar()->addMenu(createPopupMenu())->setText(tr("Window"));

    menuBar()->addSeparator();
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(actionAbout);

    fileSystemDockOrigWidget = fileSystemDock->titleBarWidget();
    bookmarksDockOrigWidget = bookmarksDock->titleBarWidget();
    tagsDockOrigWidget = tagsDock->titleBarWidget();
    imageInfoDockOrigWidget = imageInfoDock->titleBarWidget();
    fileSystemDockEmptyWidget = new QWidget;
    bookmarksDockEmptyWidget = new QWidget;
    tagsDockEmptyWidget = new QWidget;
    imageInfoDockEmptyWidget = new QWidget;
    lockDocks();
}

void Phototonic::lockDocks() {
    if (initComplete)
        Settings::hideDockTitlebars = actionLockDocks->isChecked();

    if (Settings::hideDockTitlebars) {
        fileSystemDock->setTitleBarWidget(fileSystemDockEmptyWidget);
        bookmarksDock->setTitleBarWidget(bookmarksDockEmptyWidget);
        tagsDock->setTitleBarWidget(tagsDockEmptyWidget);
        imageInfoDock->setTitleBarWidget(imageInfoDockEmptyWidget);
    } else {
        fileSystemDock->setTitleBarWidget(fileSystemDockOrigWidget);
        bookmarksDock->setTitleBarWidget(bookmarksDockOrigWidget);
        tagsDock->setTitleBarWidget(tagsDockOrigWidget);
        imageInfoDock->setTitleBarWidget(imageInfoDockOrigWidget);
    }
}

QMenu *Phototonic::createPopupMenu() {
    QMenu *extraActsMenu = QMainWindow::createPopupMenu();
    extraActsMenu->addSeparator();
    extraActsMenu->addAction(actionSmallToolbarIcons);
    extraActsMenu->addAction(actionLockDocks);
    return extraActsMenu;
}

void Phototonic::loadShortcuts() {
    // Add customizable key shortcut actions
    Settings::actionKeys[actionThumbsGoToTop->objectName()] = actionThumbsGoToTop;
    Settings::actionKeys[actionThumbsGoToBottom->objectName()] = actionThumbsGoToBottom;
    Settings::actionKeys[actionCloseImage->objectName()] = actionCloseImage;
    Settings::actionKeys[actionFullScreen->objectName()] = actionFullScreen;
    Settings::actionKeys[actionSettings->objectName()] = actionSettings;
    Settings::actionKeys[exitAction->objectName()] = exitAction;
    Settings::actionKeys[thumbsZoomInAct->objectName()] = thumbsZoomInAct;
    Settings::actionKeys[thumbsZoomOutAct->objectName()] = thumbsZoomOutAct;
    Settings::actionKeys[cutAction->objectName()] = cutAction;
    Settings::actionKeys[copyAction->objectName()] = copyAction;
    Settings::actionKeys[nextImageAction->objectName()] = nextImageAction;
    Settings::actionKeys[prevImageAction->objectName()] = prevImageAction;
    Settings::actionKeys[deleteAction->objectName()] = deleteAction;
    Settings::actionKeys[saveAction->objectName()] = saveAction;
    Settings::actionKeys[saveAsAction->objectName()] = saveAsAction;
    Settings::actionKeys[keepTransformAct->objectName()] = keepTransformAct;
    Settings::actionKeys[keepZoomAct->objectName()] = keepZoomAct;
    Settings::actionKeys[showClipboardAction->objectName()] = showClipboardAction;
    Settings::actionKeys[copyImageAction->objectName()] = copyImageAction;
    Settings::actionKeys[pasteImageAction->objectName()] = pasteImageAction;
    Settings::actionKeys[renameAction->objectName()] = renameAction;
    Settings::actionKeys[actionRefresh->objectName()] = actionRefresh;
    Settings::actionKeys[actionPaste->objectName()] = actionPaste;
    Settings::actionKeys[goBackAction->objectName()] = goBackAction;
    Settings::actionKeys[goFrwdAction->objectName()] = goFrwdAction;
    Settings::actionKeys[slideShowAction->objectName()] = slideShowAction;
    Settings::actionKeys[firstImageAction->objectName()] = firstImageAction;
    Settings::actionKeys[lastImageAction->objectName()] = lastImageAction;
    Settings::actionKeys[randomImageAction->objectName()] = randomImageAction;
    Settings::actionKeys[openAction->objectName()] = openAction;
    Settings::actionKeys[zoomOutAct->objectName()] = zoomOutAct;
    Settings::actionKeys[zoomInAct->objectName()] = zoomInAct;
    Settings::actionKeys[resetZoomAct->objectName()] = resetZoomAct;
    Settings::actionKeys[origZoomAct->objectName()] = origZoomAct;
    Settings::actionKeys[rotateLeftAct->objectName()] = rotateLeftAct;
    Settings::actionKeys[rotateRightAct->objectName()] = rotateRightAct;
    Settings::actionKeys[freeRotateLeftAct->objectName()] = freeRotateLeftAct;
    Settings::actionKeys[freeRotateRightAct->objectName()] = freeRotateRightAct;
    Settings::actionKeys[flipHAct->objectName()] = flipHAct;
    Settings::actionKeys[flipVAct->objectName()] = flipVAct;
    Settings::actionKeys[cropAct->objectName()] = cropAct;
    Settings::actionKeys[cropToSelectionAct->objectName()] = cropToSelectionAct;
    Settings::actionKeys[colorsAct->objectName()] = colorsAct;
    Settings::actionKeys[mirrorDisabledAct->objectName()] = mirrorDisabledAct;
    Settings::actionKeys[mirrorDualAct->objectName()] = mirrorDualAct;
    Settings::actionKeys[mirrorTripleAct->objectName()] = mirrorTripleAct;
    Settings::actionKeys[mirrorVDualAct->objectName()] = mirrorVDualAct;
    Settings::actionKeys[mirrorQuadAct->objectName()] = mirrorQuadAct;
    Settings::actionKeys[moveDownAct->objectName()] = moveDownAct;
    Settings::actionKeys[moveUpAct->objectName()] = moveUpAct;
    Settings::actionKeys[moveRightAct->objectName()] = moveRightAct;
    Settings::actionKeys[moveLeftAct->objectName()] = moveLeftAct;
    Settings::actionKeys[copyToAction->objectName()] = copyToAction;
    Settings::actionKeys[moveToAction->objectName()] = moveToAction;
    Settings::actionKeys[goUpAction->objectName()] = goUpAction;
    Settings::actionKeys[resizeAct->objectName()] = resizeAct;
    Settings::actionKeys[filterImagesFocusAct->objectName()] = filterImagesFocusAct;
    Settings::actionKeys[setPathFocusAct->objectName()] = setPathFocusAct;
    Settings::actionKeys[keepTransformAct->objectName()] = keepTransformAct;

    Settings::appSettings->beginGroup("Shortcuts");
    QStringList groupKeys = Settings::appSettings->childKeys();

    if (groupKeys.size()) {
        if (groupKeys.contains(actionThumbsGoToTop->text())) {
            QMapIterator<QString, QAction *> key(Settings::actionKeys);
            while (key.hasNext()) {
                key.next();
                if (groupKeys.contains(key.value()->text())) {
                    key.value()->setShortcut(Settings::appSettings->value(key.value()->text()).toString());
                    Settings::appSettings->remove(key.value()->text());
                    Settings::appSettings->setValue(key.key(), key.value()->shortcut().toString());
                }
            }
        } else {
            for (int i = 0; i < groupKeys.size(); ++i) {
                if (Settings::actionKeys.value(groupKeys.at(i)))
                    Settings::actionKeys.value(groupKeys.at(i))->setShortcut
                            (Settings::appSettings->value(groupKeys.at(i)).toString());
            }
        }
    } else {
        actionThumbsGoToTop->setShortcut(QKeySequence("Ctrl+Home"));
        actionThumbsGoToBottom->setShortcut(QKeySequence("Ctrl+End"));
        actionCloseImage->setShortcut(Qt::Key_Escape);
        actionFullScreen->setShortcut(QKeySequence("Alt+Return"));
        actionSettings->setShortcut(QKeySequence("Ctrl+P"));
        exitAction->setShortcut(QKeySequence("Ctrl+Q"));
        cutAction->setShortcut(QKeySequence("Ctrl+X"));
        copyAction->setShortcut(QKeySequence("Ctrl+C"));
        deleteAction->setShortcut(QKeySequence("Del"));
        saveAction->setShortcut(QKeySequence("Ctrl+S"));
        copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
        pasteImageAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
        renameAction->setShortcut(QKeySequence("F2"));
        actionRefresh->setShortcut(QKeySequence("F5"));
        actionPaste->setShortcut(QKeySequence("Ctrl+V"));
        goBackAction->setShortcut(QKeySequence("Alt+Left"));
        goFrwdAction->setShortcut(QKeySequence("Alt+Right"));
        goUpAction->setShortcut(QKeySequence("Alt+Up"));
        slideShowAction->setShortcut(QKeySequence("Ctrl+W"));
        nextImageAction->setShortcut(QKeySequence("PgDown"));
        prevImageAction->setShortcut(QKeySequence("PgUp"));
        firstImageAction->setShortcut(QKeySequence("Home"));
        lastImageAction->setShortcut(QKeySequence("End"));
        randomImageAction->setShortcut(QKeySequence("Ctrl+D"));
        openAction->setShortcut(QKeySequence("Return"));
        zoomOutAct->setShortcut(QKeySequence("-"));
        zoomInAct->setShortcut(QKeySequence("+"));
        resetZoomAct->setShortcut(QKeySequence("*"));
        origZoomAct->setShortcut(QKeySequence("/"));
        rotateLeftAct->setShortcut(QKeySequence("Ctrl+Left"));
        rotateRightAct->setShortcut(QKeySequence("Ctrl+Right"));
        freeRotateLeftAct->setShortcut(QKeySequence("Ctrl+Shift+Left"));
        freeRotateRightAct->setShortcut(QKeySequence("Ctrl+Shift+Right"));
        flipHAct->setShortcut(QKeySequence("Ctrl+Down"));
        flipVAct->setShortcut(QKeySequence("Ctrl+Up"));
        cropAct->setShortcut(QKeySequence("Ctrl+G"));
        cropToSelectionAct->setShortcut(QKeySequence("Ctrl+R"));
        colorsAct->setShortcut(QKeySequence("Ctrl+O"));
        mirrorDisabledAct->setShortcut(QKeySequence("Ctrl+1"));
        mirrorDualAct->setShortcut(QKeySequence("Ctrl+2"));
        mirrorTripleAct->setShortcut(QKeySequence("Ctrl+3"));
        mirrorVDualAct->setShortcut(QKeySequence("Ctrl+4"));
        mirrorQuadAct->setShortcut(QKeySequence("Ctrl+5"));
        moveDownAct->setShortcut(QKeySequence("Down"));
        moveUpAct->setShortcut(QKeySequence("Up"));
        moveLeftAct->setShortcut(QKeySequence("Left"));
        moveRightAct->setShortcut(QKeySequence("Right"));
        copyToAction->setShortcut(QKeySequence("Ctrl+Y"));
        moveToAction->setShortcut(QKeySequence("Ctrl+M"));
        resizeAct->setShortcut(QKeySequence("Ctrl+I"));
        filterImagesFocusAct->setShortcut(QKeySequence("Ctrl+F"));
        setPathFocusAct->setShortcut(QKeySequence("Ctrl+L"));
        keepTransformAct->setShortcut(QKeySequence("Ctrl+K"));
    }

    Settings::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event) {
    thumbsViewer->abort();
    writeSettings();
    hide();
    qInfo() << "Phototonic: releasing clipboard...";
    if (!QApplication::clipboard()->image().isNull()) {
        QApplication::clipboard()->clear();
    }
    event->accept();
}

void Phototonic::setStatus(QString state) {
    statusLabel->setText("    " + state + "    ");
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event) {
    if (interfaceDisabled)
        return;

    if (event->button() == Qt::LeftButton) {
        if (Settings::layoutMode == ImageViewWidget) {
            if (Settings::reverseMouseBehavior) {
                actionFullScreen->setChecked(!(actionFullScreen->isChecked()));
                toggleFullScreen();
                event->accept();
            } else if (actionCloseImage->isEnabled()) {
                hideViewer();
                event->accept();
            }
        } else {
            if (QApplication::focusWidget() == imageViewer->scrlArea) {
                openOp();
            }
        }
    }
}

void Phototonic::mousePressEvent(QMouseEvent *event) {
    if (interfaceDisabled)
        return;

    if (Settings::layoutMode == ImageViewWidget) {
        if (event->button() == Qt::MiddleButton) {

            if (event->modifiers() == Qt::ShiftModifier) {
                origZoom();
                event->accept();
                return;
            }
            if (event->modifiers() == Qt::ControlModifier) {
                resetZoom();
                event->accept();
                return;
            }

            if (Settings::reverseMouseBehavior && actionCloseImage->isEnabled()) {
                hideViewer();
                event->accept();
            } else {
                actionFullScreen->setChecked(!(actionFullScreen->isChecked()));
                toggleFullScreen();
                event->accept();
            }
        }
    } else {
        if (QApplication::focusWidget() == imageViewer->scrlArea) {
            if (event->button() == Qt::MiddleButton && Settings::reverseMouseBehavior) {
                openOp();
            }
        }
    }
}

void Phototonic::newImage() {
    if (Settings::layoutMode == ThumbViewWidget)
        showViewer();

    imageViewer->loadImage("");
}

void Phototonic::setDocksVisibility(bool visible) {
    fileSystemDock->setVisible(visible ? Settings::fileSystemDockVisible : false);
    bookmarksDock->setVisible(visible ? Settings::bookmarksDockVisible : false);
    tagsDock->setVisible(visible ? Settings::tagsDockVisible : false);
    imageInfoDock->setVisible(visible ? Settings::imageInfoDockVisible : false);

    menuBar()->setVisible(visible);
    menuBar()->setDisabled(!visible);
    statusBar()->setVisible(visible);

    editToolBar->setVisible(visible ? editToolBarVisible : false);
    goToolBar->setVisible(visible ? goToolBarVisible : false);
    viewToolBar->setVisible(visible ? viewToolBarVisible : false);
    imageToolBar->setVisible(visible ? imageToolBarVisible : Settings::showViewerToolbar);
    addToolBar(imageToolBar);

    setContextMenuPolicy(Qt::PreventContextMenu);
}

void Phototonic::openOp() {
    if (Settings::layoutMode == ImageViewWidget) {
        hideViewer();
        return;
    }

    if (QApplication::focusWidget() == fileSystemTree) {
        goSelectedDir(fileSystemTree->getCurrentIndex());
        return;
    } else if (QApplication::focusWidget() == thumbsViewer
               || QApplication::focusWidget() == imageViewer->scrlArea) {
        QModelIndex idx;
        QModelIndexList indexesList = thumbsViewer->selectionModel()->selectedIndexes();
        if (indexesList.size() > 0) {
            idx = indexesList.first();
        } else {
            if (thumbsViewer->thumbsViewerModel->rowCount() == 0) {
                setStatus(tr("No images"));
                return;
            }

            idx = thumbsViewer->thumbsViewerModel->indexFromItem(thumbsViewer->thumbsViewerModel->item(0));
            thumbsViewer->selectionModel()->select(idx, QItemSelectionModel::Toggle);
            thumbsViewer->setCurrentRow(0);
        }

        loadImagefromThumb(idx);
        return;
    } else if (QApplication::focusWidget() == filterLineEdit) {
        setThumbsFilter();
        return;
    } else if (QApplication::focusWidget() == pathLineEdit) {
        goPathBarDir();
        return;
    }
}

void Phototonic::setEditToolBarVisibility() {
    editToolBarVisible = editToolBar->isVisible();
}

void Phototonic::setGoToolBarVisibility() {
    goToolBarVisible = goToolBar->isVisible();
}

void Phototonic::setViewToolBarVisibility() {
    viewToolBarVisible = viewToolBar->isVisible();
}

void Phototonic::setImageToolBarVisibility() {
    imageToolBarVisible = imageToolBar->isVisible();
}

void Phototonic::setFsDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::fileSystemDockVisible = fileSystemDock->isVisible();
    }
}

void Phototonic::setBmDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::bookmarksDockVisible = bookmarksDock->isVisible();
    }
}

void Phototonic::setTagsDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::tagsDockVisible = tagsDock->isVisible();
    }
}

void Phototonic::setIiDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::imageInfoDockVisible = imageInfoDock->isVisible();
    }
}

void Phototonic::showViewer() {
    if (Settings::layoutMode == ThumbViewWidget) {
        Settings::layoutMode = ImageViewWidget;
        Settings::appSettings->setValue("Geometry", saveGeometry());
        Settings::appSettings->setValue("WindowState", saveState());

        imageViewLayout->removeWidget(imageViewer);
        imageViewer->setVisible(true);
        takeCentralWidget();
        setCentralWidget(imageViewer);
        setDocksVisibility(false);

        if (Settings::isFullScreen) {
            shouldMaximize = isMaximized();
            showFullScreen();
            imageViewer->setCursorHiding(true);
            QApplication::processEvents();
        }

        updateActions();
    }
}

void Phototonic::showBusyStatus(bool busy) {
    static int busyStatus = 0;

    if (busy) {
        ++busyStatus;
    } else {
        --busyStatus;
    }

    if (busyStatus > 0) {
        busyMovie->start();
        busyLabel->setVisible(true);
    } else {
        busyLabel->setVisible(false);
        busyMovie->stop();
        busyStatus = 0;
    }
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx) {
    thumbsViewer->setCurrentRow(idx.row());
    showViewer();
    imageViewer->loadImage(
            thumbsViewer->thumbsViewerModel->item(idx.row())->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setImageViewerWindowTitle();
}

void Phototonic::loadImagefromCli() {
    QFile imageFile(cliFileName);
    if (!imageFile.exists()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to open file \"%1\", file not found.").arg(cliFileName));
        cliFileName = "";
        return;
    }

    showViewer();
    imageViewer->loadImage(cliFileName);
    setWindowTitle(cliFileName + " - Phototonic");
}

void Phototonic::toggleSlideShow() {
    if (Settings::slideShowActive) {
        Settings::slideShowActive = false;
        slideShowAction->setText(tr("Slide Show"));
        imageViewer->setFeedback(tr("Slide show stopped"));

        SlideShowTimer->stop();
        delete SlideShowTimer;
        slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));
    } else {
        if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
            return;
        }

        if (Settings::layoutMode == ThumbViewWidget) {
            QModelIndexList indexesList = thumbsViewer->selectionModel()->selectedIndexes();
            if (indexesList.size() != 1) {
                thumbsViewer->setCurrentRow(0);
            } else {
                thumbsViewer->setCurrentRow(indexesList.first().row());
            }

            showViewer();
        }

        Settings::slideShowActive = true;

        SlideShowTimer = new QTimer(this);
        connect(SlideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowHandler()));
        SlideShowTimer->start(Settings::slideShowDelay * 1000);

        slideShowAction->setText(tr("Stop Slide Show"));
        imageViewer->setFeedback(tr("Slide show started"));
        slideShowAction->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/images/stop.png")));

        slideShowHandler();
    }
}

void Phototonic::slideShowHandler() {
    if (Settings::slideShowActive) {
        if (Settings::slideShowRandom) {
            loadRandomImage();
        } else {
            int currentRow = thumbsViewer->getCurrentRow();
            imageViewer->loadImage(
                    thumbsViewer->thumbsViewerModel->item(currentRow)->data(thumbsViewer->FileNameRole).toString());
            thumbsViewer->setImageViewerWindowTitle();

            if (thumbsViewer->getNextRow() > 0) {
                thumbsViewer->setCurrentRow(thumbsViewer->getNextRow());
            } else {
                if (Settings::wrapImageList) {
                    thumbsViewer->setCurrentRow(0);
                } else {
                    toggleSlideShow();
                }
            }
        }
    }
}

void Phototonic::loadNextImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    int nextRow = thumbsViewer->getNextRow();
    if (nextRow < 0) {
        if (Settings::wrapImageList) {
            nextRow = 0;
        } else {
            return;
        }
    }

    imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(nextRow)->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setCurrentRow(nextRow);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(nextRow);
    }
}

void Phototonic::loadPrevImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    int prevRow = thumbsViewer->getPrevRow();
    if (prevRow < 0) {
        if (Settings::wrapImageList)
            prevRow = thumbsViewer->getLastRow();
        else
            return;
    }

    imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(prevRow)->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setCurrentRow(prevRow);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(prevRow);
    }
}

void Phototonic::loadFirstImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(0)->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setCurrentRow(0);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(0);
    }
}

void Phototonic::loadLastImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    int lastRow = thumbsViewer->getLastRow();
    imageViewer->loadImage(thumbsViewer->thumbsViewerModel->item(lastRow)->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setCurrentRow(lastRow);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(lastRow);
    }
}

void Phototonic::loadRandomImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    int randomRow = thumbsViewer->getRandomRow();
    imageViewer->loadImage(
            thumbsViewer->thumbsViewerModel->item(randomRow)->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setCurrentRow(randomRow);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(randomRow);
    }
}

void Phototonic::setViewerKeyEventsEnabled(bool enabled) {
    moveLeftAct->setEnabled(enabled);
    moveRightAct->setEnabled(enabled);
    moveUpAct->setEnabled(enabled);
    moveDownAct->setEnabled(enabled);
}

void Phototonic::updateIndexByViewerImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() > 0 &&
        thumbsViewer->setCurrentIndexByName(imageViewer->currentImageFullPath)) {
        thumbsViewer->selectCurrentIndex();
    }
}

void Phototonic::hideViewer() {
    if (cliImageLoaded && Settings::exitInsteadOfClose) {
        close();
        return;
    }

    showBusyStatus(true);

    if (isFullScreen()) {
        showNormal();
        if (shouldMaximize) {
            showMaximized();
        }
        imageViewer->setCursorHiding(false);
    }

    Settings::layoutMode = ThumbViewWidget;
    takeCentralWidget();
    setCentralWidget(thumbsViewer);
    imageViewLayout->addWidget(imageViewer);
    QApplication::processEvents();

    setDocksVisibility(true);
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    for (int i = 0; i <= 10 && qApp->hasPendingEvents(); ++i) {
        QApplication::processEvents();
    }
    setThumbsViewerWindowTitle();

    if (!cliFileName.isEmpty()) {
        cliFileName = "";
    }
    restoreGeometry(Settings::appSettings->value("Geometry").toByteArray());
    restoreState(Settings::appSettings->value("WindowState").toByteArray());

    if (needThumbsRefresh) {
        needThumbsRefresh = false;
        refreshThumbs(true);
    } else {
        if (thumbsViewer->thumbsViewerModel->rowCount() > 0) {
            if (thumbsViewer->setCurrentIndexByName(imageViewer->currentImageFullPath)) {
                thumbsViewer->selectCurrentIndex();
            }
        }

        thumbsViewer->loadVisibleThumbs();
    }

    imageViewer->loadImage("");
    thumbsViewer->setFocus(Qt::OtherFocusReason);
    showBusyStatus(false);
    setContextMenuPolicy(Qt::DefaultContextMenu);

    updateActions();
}

void Phototonic::goBottom() {
    thumbsViewer->scrollToBottom();
}

void Phototonic::goTop() {
    thumbsViewer->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString copyMoveDirPath) {
    QApplication::restoreOverrideCursor();
    Settings::copyOp = (keyMods == Qt::ControlModifier);
    QMessageBox msgBox;
    QString destDir;

    if (QObject::sender() == fileSystemTree) {
        destDir = getSelectedPath();
    } else if (QObject::sender() == bookmarks) {
        if (bookmarks->currentItem()) {
            destDir = bookmarks->currentItem()->toolTip(0);
        } else {
            addBookmark(copyMoveDirPath);
            return;
        }
    } else {
        // Unknown sender
        return;
    }

    if (!isValidPath(destDir)) {
        msgBox.critical(this, tr("Error"), tr("Can not move or copy images to this folder."));
        selectCurrentViewDir();
        return;
    }

    if (destDir == Settings::currentViewDir) {
        msgBox.critical(this, tr("Error"), tr("Destination folder is same as source."));
        return;
    }

    if (dirOp) {
        QString dirOnly = copyMoveDirPath.right(copyMoveDirPath.size() - copyMoveDirPath.lastIndexOf(QDir::separator()) - 1);

        QString question = tr("Move \"%1\" to \"%2\"?").arg(dirOnly).arg(destDir);
        int ret = QMessageBox::question(this, tr("Move folder"), question,
                                        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (ret == QMessageBox::Yes) {
            QFile dir(copyMoveDirPath);
            bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
            if (!ok) {
                msgBox.critical(this, tr("Error"), tr("Failed to move folder."));
            }
            setStatus(tr("Folder moved"));
        }
    } else {
        CopyMoveDialog *copyMoveDialog = new CopyMoveDialog(this);
        Settings::copyCutIdxList = thumbsViewer->selectionModel()->selectedIndexes();
        copyMoveDialog->exec(thumbsViewer, destDir, false);

        if (!Settings::copyOp) {
            int row = copyMoveDialog->latestRow;
            if (thumbsViewer->thumbsViewerModel->rowCount()) {
                if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
                    row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
                }

                thumbsViewer->setCurrentRow(row);
                thumbsViewer->selectThumbByRow(row);
            }
        }

        QString stateString = QString((Settings::copyOp ? tr("Copied") : tr("Moved")) + " " +
                                      tr("%n image(s)", "", copyMoveDialog->nFiles));
        setStatus(stateString);
        delete (copyMoveDialog);
    }

    thumbsViewer->loadVisibleThumbs();
}

void Phototonic::selectCurrentViewDir() {
    QModelIndex idx = fileSystemTree->fsModel->index(Settings::currentViewDir);
    if (idx.isValid()) {
        fileSystemTree->expand(idx);
        fileSystemTree->setCurrentIndex(idx);
    }
}

void Phototonic::checkDirState(const QModelIndex &, int, int) {
    if (!initComplete) {
        return;
    }

    if (thumbsViewer->busy) {
        thumbsViewer->abort();
    }

    if (!QDir().exists(Settings::currentViewDir)) {
        Settings::currentViewDir = "";
        QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
    }
}

void Phototonic::recordHistory(QString dir) {
    if (!needHistoryRecord) {
        needHistoryRecord = true;
        return;
    }

    if (pathHistoryList.size() && dir == pathHistoryList.at(currentHistoryIdx))
        return;

    pathHistoryList.insert(++currentHistoryIdx, dir);

    // Need to clear irrelevant items from list
    if (currentHistoryIdx != pathHistoryList.size() - 1) {
        goFrwdAction->setEnabled(false);
        for (int i = pathHistoryList.size() - 1; i > currentHistoryIdx; --i) {
            pathHistoryList.removeAt(i);
        }
    }
}

void Phototonic::reloadThumbsSlot() {
    if (thumbsViewer->busy || !initComplete) {
        thumbsViewer->abort();
        QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
        return;
    }

    if (Settings::currentViewDir == "") {
        Settings::currentViewDir = getSelectedPath();
        if (Settings::currentViewDir == "")
            return;
    }

    QDir checkPath(Settings::currentViewDir);
    if (!checkPath.exists() || !checkPath.isReadable()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to open folder:") + " " + Settings::currentViewDir);
        return;
    }

    thumbsViewer->infoView->clear();

    pathLineEdit->setText(Settings::currentViewDir);
    recordHistory(Settings::currentViewDir);
    if (currentHistoryIdx > 0)
        goBackAction->setEnabled(true);

    if (Settings::layoutMode == ThumbViewWidget) {
        setThumbsViewerWindowTitle();
    }

    thumbsViewer->busy = true;

    if (findDupesAction->isChecked()) {
        thumbsViewer->loadDuplicates();
    } else {
        thumbsViewer->load();
    }
}

void Phototonic::setThumbsViewerWindowTitle() {
    if (findDupesAction->isChecked())
        setWindowTitle(tr("Duplicate images in %1").arg(Settings::currentViewDir) + " - Phototonic");
    else
        setWindowTitle(Settings::currentViewDir + " - Phototonic");
}

void Phototonic::renameDir() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QFileInfo dirInfo = QFileInfo(fileSystemTree->fsModel->filePath(selectedDirs[0]));

    bool ok;
    QString title = tr("Rename") + " " + dirInfo.completeBaseName();
    QString newDirName = QInputDialog::getText(this, title,
                                               tr("New name:"), QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

    if (!ok) {
        selectCurrentViewDir();
        return;
    }

    if (newDirName.isEmpty()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Invalid name entered."));
        selectCurrentViewDir();
        return;
    }

    QFile dir(dirInfo.absoluteFilePath());
    QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
    ok = dir.rename(newFullPathName);
    if (!ok) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to rename folder."));
        selectCurrentViewDir();
        return;
    }

    if (Settings::currentViewDir == dirInfo.absoluteFilePath())
        fileSystemTree->setCurrentIndex(fileSystemTree->fsModel->index(newFullPathName));
    else
        selectCurrentViewDir();
}

void Phototonic::rename() {
    if (QApplication::focusWidget() == fileSystemTree) {
        renameDir();
        return;
    }

    if (Settings::layoutMode == ImageViewWidget) {

        if (imageViewer->isNewImage()) {
            showNewImageWarning(this);
            return;
        }

        if (thumbsViewer->thumbsViewerModel->rowCount() > 0) {
            if (thumbsViewer->setCurrentIndexByName(imageViewer->currentImageFullPath))
                thumbsViewer->selectCurrentIndex();
        }
    }

    QString selectedImageFileName = thumbsViewer->getSingleSelectionFilename();
    if (selectedImageFileName.isEmpty()) {
        setStatus(tr("Invalid selection"));
        return;
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    QString currnetFilePath = selectedImageFileName;
    QFile currentFile(currnetFilePath);
    QString newImageFullPath = Settings::currentViewDir;
    bool ok;
    QString title = tr("Rename Image");
    QString newImageName = QInputDialog::getText(this,
                                                 title, tr("Enter a new name for \"%1\":")
                                                                .arg(QFileInfo(selectedImageFileName).fileName())
                                                        + "\t\t\t",
                                                 QLineEdit::Normal,
                                                 QFileInfo(selectedImageFileName).completeBaseName(),
                                                 &ok);

    if (!ok) {
        goto cleanUp;
    }

    if (newImageName.isEmpty()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("No name entered."));
        goto cleanUp;
    }

    newImageName += "." + QFileInfo(selectedImageFileName).suffix();
    if (newImageFullPath.right(1) == QDir::separator()) {
        newImageFullPath += newImageName;
    } else {
        newImageFullPath += QDir::separator() + newImageName;
    }

    ok = currentFile.rename(newImageFullPath);
    if (ok) {
        QModelIndexList indexesList = thumbsViewer->selectionModel()->selectedIndexes();
        thumbsViewer->thumbsViewerModel->item(
                indexesList.first().row())->setData(newImageFullPath, thumbsViewer->FileNameRole);

        imageViewer->setInfo(newImageName);
        imageViewer->currentImageFullPath = newImageFullPath;

        if (Settings::layoutMode == ImageViewWidget) {
            thumbsViewer->setImageViewerWindowTitle();
        }
    } else {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to rename image."));
    }

    cleanUp:
    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::deleteDir() {
    bool ok = true;
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QString deletePath = fileSystemTree->fsModel->filePath(selectedDirs[0]);
    QModelIndex idxAbove = fileSystemTree->indexAbove(selectedDirs[0]);
    QFileInfo dirInfo = QFileInfo(deletePath);
    QString question = tr("Permanently delete \"%1\" and all of its contents?").arg(dirInfo.completeBaseName());

    QMessageBox msgBox;
    msgBox.setText(question);
    msgBox.setWindowTitle(tr("Delete folder"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes)
        ok = removeDirOp(deletePath);
    else {
        selectCurrentViewDir();
        return;
    }

    if (!ok) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to delete folder."));
        selectCurrentViewDir();
    }

    QString state = QString(tr("Removed \"%1\"").arg(deletePath));
    setStatus(state);

    if (Settings::currentViewDir == deletePath) {
        if (idxAbove.isValid())
            fileSystemTree->setCurrentIndex(idxAbove);
    } else
        selectCurrentViewDir();
}

void Phototonic::createSubDirectory() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QFileInfo dirInfo = QFileInfo(fileSystemTree->fsModel->filePath(selectedDirs[0]));

    bool ok;
    QString newDirName = QInputDialog::getText(this, tr("New Sub folder"),
                                               tr("New folder name:"), QLineEdit::Normal, "", &ok);

    if (!ok) {
        selectCurrentViewDir();
        return;
    }

    if (newDirName.isEmpty()) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Invalid name entered."));
        selectCurrentViewDir();
        return;
    }

    QDir dir(dirInfo.absoluteFilePath());
    ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

    if (!ok) {
        QMessageBox msgBox;
        msgBox.critical(this, tr("Error"), tr("Failed to create new folder."));
        selectCurrentViewDir();
        return;
    }

    setStatus(tr("Created \"%1\"").arg(newDirName));
    fileSystemTree->expand(selectedDirs[0]);
}

QString Phototonic::getSelectedPath() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    if (selectedDirs.size() && selectedDirs[0].isValid()) {
        QFileInfo dirInfo = QFileInfo(fileSystemTree->fsModel->filePath(selectedDirs[0]));
        return dirInfo.absoluteFilePath();
    } else
        return "";
}

void Phototonic::wheelEvent(QWheelEvent *event) {
    if (Settings::layoutMode == ImageViewWidget
        || QApplication::focusWidget() == imageViewer->scrlArea) {
        if (event->modifiers() == Qt::ControlModifier) {
            if (event->delta() < 0)
                zoomOut();
            else
                zoomIn();
        } else if (nextImageAction->isEnabled()) {
            if (event->delta() < 0)
                loadNextImage();
            else
                loadPrevImage();
        }
    }
}

void Phototonic::showNewImageWarning(QWidget *parent) {
    QMessageBox msgBox;
    msgBox.warning(parent, tr("Warning"), tr("Cannot perform action with temporary image."));
}

bool Phototonic::removeDirOp(QString dirToDelete) {
    bool ok = true;
    QDir dir(dirToDelete);

    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                                QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                ok = removeDirOp(info.absoluteFilePath());
            } else {
                ok = QFile::remove(info.absoluteFilePath());
            }

            if (!ok) {
                return ok;
            }
        }
    ok = dir.rmdir(dirToDelete);

    return ok;
}

void Phototonic::cleanupCropDialog() {
    setInterfaceEnabled(true);
}

void Phototonic::cleanupScaleDialog() {
    delete resizeDialog;
    resizeDialog = 0;
    setInterfaceEnabled(true);
}

void Phototonic::cleanupColorsDialog() {
    Settings::colorsActive = false;
    setInterfaceEnabled(true);
}

void Phototonic::setInterfaceEnabled(bool enable) {
    // actions
    colorsAct->setEnabled(enable);
    renameAction->setEnabled(enable);
    cropAct->setEnabled(enable);
    resizeAct->setEnabled(enable);
    actionCloseImage->setEnabled(enable);
    nextImageAction->setEnabled(enable);
    prevImageAction->setEnabled(enable);
    firstImageAction->setEnabled(enable);
    lastImageAction->setEnabled(enable);
    randomImageAction->setEnabled(enable);
    slideShowAction->setEnabled(enable);
    copyToAction->setEnabled(enable);
    moveToAction->setEnabled(enable);
    deleteAction->setEnabled(enable);
    actionSettings->setEnabled(enable);
    openAction->setEnabled(enable);

    // other
    thumbsViewer->setEnabled(enable);
    fileSystemTree->setEnabled(enable);
    bookmarks->setEnabled(enable);
    thumbsViewer->imageTags->setEnabled(enable);
    menuBar()->setEnabled(enable);
    editToolBar->setEnabled(enable);
    goToolBar->setEnabled(enable);
    viewToolBar->setEnabled(enable);
    interfaceDisabled = !enable;

    if (enable) {
        if (isFullScreen()) {
            imageViewer->setCursorHiding(true);
        }
        updateActions();
    } else {
        imageViewer->setCursorHiding(false);
    }
}

void Phototonic::addNewBookmark() {
    addBookmark(getSelectedPath());
}

void Phototonic::addBookmark(QString path) {
    Settings::bookmarkPaths.insert(path);
    bookmarks->reloadBookmarks();
}

void Phototonic::findDuplicateImages() {
    refreshThumbs(true);
}

