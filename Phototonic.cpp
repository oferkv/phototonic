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
#include "CopyMoveDialog.h"
#include "ResizeDialog.h"
#include "CropDialog.h"
#include "ColorsDialog.h"
#include "ExternalAppsDialog.h"
#include "ProgressDialog.h"
#include "RangeInputDialog.h"
#include "ImagePreview.h"
#include "FileListWidget.h"
#include "GuideWidget.h"
#include "RenameDialog.h"
#include "Trashcan.h"
#include "MessageBox.h"

Phototonic::Phototonic(QStringList argumentsList, int filesStartAt, QWidget *parent) : QMainWindow(parent) {
    Settings::appSettings = new QSettings("phototonic", "phototonic");
    setDockOptions(QMainWindow::AllowNestedDocks);
    readSettings();
    createThumbsViewer();
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createFileSystemDock();
    createBookmarksDock();
    createImagePreviewDock();
    createImageTagsDock();
    createImageViewer();
    updateExternalApps();
    loadShortcuts();
    setupDocks();

    connect(qApp, SIGNAL(focusChanged(QWidget * , QWidget * )), this, SLOT(updateActions()));

    restoreGeometry(Settings::appSettings->value(Settings::optionGeometry).toByteArray());
    restoreState(Settings::appSettings->value(Settings::optionWindowState).toByteArray());
    defaultApplicationIcon = QIcon(":/images/phototonic.png");
    setWindowIcon(defaultApplicationIcon);

    stackedLayout = new QStackedLayout;
    QWidget *stackedLayoutWidget = new QWidget;
    stackedLayout->addWidget(thumbsViewer);
    stackedLayout->addWidget(imageViewer);
    stackedLayoutWidget->setLayout(stackedLayout);
    setCentralWidget(stackedLayoutWidget);
    processStartupArguments(argumentsList, filesStartAt);

    copyMoveToDialog = nullptr;
    colorsDialog = nullptr;
    cropDialog = nullptr;
    initComplete = true;
    thumbsViewer->isBusy = false;
    currentHistoryIdx = -1;
    needHistoryRecord = true;
    interfaceDisabled = false;

    refreshThumbs(true);
    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->setFocus(Qt::OtherFocusReason);
    }
}

void Phototonic::processStartupArguments(QStringList argumentsList, int filesStartAt) {
    if (argumentsList.size() > filesStartAt) {
        QFileInfo firstArgument(argumentsList.at(filesStartAt));
        if (firstArgument.isDir()) {
            Settings::currentDirectory = argumentsList.at(filesStartAt);
        } else if (argumentsList.size() > filesStartAt + 1) {
            loadStartupFileList(argumentsList, filesStartAt);
            return;
        } else {
            Settings::currentDirectory = firstArgument.absolutePath();
            QString cliFileName = Settings::currentDirectory + QDir::separator() + firstArgument.fileName();
            loadImageFromCliArguments(cliFileName);
            QTimer::singleShot(1000, this, SLOT(updateIndexByViewerImage()));
        }
    } else {
        if (Settings::startupDir == Settings::SpecifiedDir) {
            Settings::currentDirectory = Settings::specifiedStartDir;
        } else if (Settings::startupDir == Settings::RememberLastDir) {
            Settings::currentDirectory = Settings::appSettings->value(Settings::optionLastDir).toString();
        }
    }
    selectCurrentViewDir();
}

QIcon &Phototonic::getDefaultWindowIcon() {
    return defaultApplicationIcon;
}

void Phototonic::loadStartupFileList(QStringList argumentsList, int filesStartAt) {
    Settings::filesList.clear();
    for (int i = filesStartAt; i < argumentsList.size(); i++) {
        QFile currentFileFullPath(argumentsList[i]);
        QFileInfo currentFileInfo(currentFileFullPath);

        if (!Settings::filesList.contains(currentFileInfo.absoluteFilePath())) {
            Settings::filesList << currentFileInfo.absoluteFilePath();
        }
    }
    fileSystemTree->clearSelection();
    fileListWidget->itemAt(0, 0)->setSelected(true);
    Settings::isFileListLoaded = true;
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

    connect(thumbsViewer->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(updateActions()));

    imageInfoDock = new QDockWidget(tr("Image Info"), this);
    imageInfoDock->setObjectName("Image Info");
    imageInfoDock->setWidget(thumbsViewer->infoView);
    connect(imageInfoDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setImageInfoDockVisibility()));
    connect(imageInfoDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setImageInfoDockVisibility()));
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
    connect(applyCropAndRotationAction, SIGNAL(triggered()), imageViewer, SLOT(applyCropAndRotation()));
    imageViewer->ImagePopUpMenu = new QMenu();

    // Widget actions
    imageViewer->addAction(slideShowAction);
    imageViewer->addAction(nextImageAction);
    imageViewer->addAction(prevImageAction);
    imageViewer->addAction(firstImageAction);
    imageViewer->addAction(lastImageAction);
    imageViewer->addAction(randomImageAction);
    imageViewer->addAction(zoomInAction);
    imageViewer->addAction(zoomOutAction);
    imageViewer->addAction(origZoomAction);
    imageViewer->addAction(resetZoomAction);
    imageViewer->addAction(rotateRightAction);
    imageViewer->addAction(rotateLeftAction);
    imageViewer->addAction(freeRotateRightAction);
    imageViewer->addAction(freeRotateLeftAction);
    imageViewer->addAction(flipHorizontalAction);
    imageViewer->addAction(flipVerticalAction);
    imageViewer->addAction(cropAction);
    imageViewer->addAction(applyCropAndRotationAction);
    imageViewer->addAction(resizeAction);
    imageViewer->addAction(saveAction);
    imageViewer->addAction(saveAsAction);
    imageViewer->addAction(copyImageAction);
    imageViewer->addAction(pasteImageAction);
    imageViewer->addAction(deleteAction);
    imageViewer->addAction(deletePermanentlyAction);
    imageViewer->addAction(renameAction);
    imageViewer->addAction(CloseImageAction);
    imageViewer->addAction(fullScreenAction);
    imageViewer->addAction(settingsAction);
    imageViewer->addAction(mirrorDisabledAction);
    imageViewer->addAction(mirrorDualAction);
    imageViewer->addAction(mirrorTripleAction);
    imageViewer->addAction(mirrorDualVerticalAction);
    imageViewer->addAction(mirrorQuadAction);
    imageViewer->addAction(keepTransformAction);
    imageViewer->addAction(keepZoomAction);
    imageViewer->addAction(refreshAction);
    imageViewer->addAction(colorsAction);
    imageViewer->addAction(moveRightAction);
    imageViewer->addAction(moveLeftAction);
    imageViewer->addAction(moveUpAction);
    imageViewer->addAction(moveDownAction);
    imageViewer->addAction(showClipboardAction);
    imageViewer->addAction(copyToAction);
    imageViewer->addAction(moveToAction);
    imageViewer->addAction(resizeAction);
    imageViewer->addAction(viewImageAction);
    imageViewer->addAction(exitAction);
    imageViewer->addAction(showViewerToolbarAction);
    imageViewer->addAction(externalAppsAction);

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
    zoomSubMenuAction = new QAction(tr("Zoom"), this);
    zoomSubMenuAction->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
    zoomSubMenuAction->setMenu(zoomSubMenu);
    imageViewer->ImagePopUpMenu->addAction(zoomSubMenuAction);
    zoomSubMenu->addAction(zoomInAction);
    zoomSubMenu->addAction(zoomOutAction);
    zoomSubMenu->addAction(origZoomAction);
    zoomSubMenu->addAction(resetZoomAction);
    addMenuSeparator(zoomSubMenu);
    zoomSubMenu->addAction(keepZoomAction);

    MirroringSubMenu = new QMenu(tr("Mirroring"));
    mirrorSubMenuAction = new QAction(tr("Mirroring"), this);
    mirrorSubMenuAction->setMenu(MirroringSubMenu);
    mirroringActionGroup = new QActionGroup(this);
    mirroringActionGroup->addAction(mirrorDisabledAction);
    mirroringActionGroup->addAction(mirrorDualAction);
    mirroringActionGroup->addAction(mirrorTripleAction);
    mirroringActionGroup->addAction(mirrorDualVerticalAction);
    mirroringActionGroup->addAction(mirrorQuadAction);
    MirroringSubMenu->addActions(mirroringActionGroup->actions());

    guideSubMenu = new QMenu(tr("Guides"));
    guideSubMenuAction = new QAction(tr("Guides"), this);
    guideSubMenuAction->setMenu(guideSubMenu);
    guideAddVerticalAction = new QAction(tr("Add vertical guide"), this);
    guideAddHorizontalAction = new QAction(tr("Add horizontal guide"), this);
    guideSubMenu->addAction(guideAddVerticalAction);
    guideSubMenu->addAction(guideAddHorizontalAction);
    connect(guideAddVerticalAction, &QAction::triggered, this, &Phototonic::addVerticalGuide);
    connect(guideAddHorizontalAction, &QAction::triggered, this, &Phototonic::addHorizontalGuide);

    transformSubMenu = new QMenu(tr("Transform"));
    transformSubMenuAction = new QAction(tr("Transform"), this);
    transformSubMenuAction->setMenu(transformSubMenu);
    imageViewer->ImagePopUpMenu->addAction(resizeAction);
    imageViewer->ImagePopUpMenu->addAction(applyCropAndRotationAction);
    imageViewer->ImagePopUpMenu->addAction(transformSubMenuAction);
    transformSubMenu->addAction(colorsAction);
    transformSubMenu->addAction(rotateRightAction);
    transformSubMenu->addAction(rotateLeftAction);
    transformSubMenu->addAction(freeRotateRightAction);
    transformSubMenu->addAction(freeRotateLeftAction);
    transformSubMenu->addAction(flipHorizontalAction);
    transformSubMenu->addAction(flipVerticalAction);
    transformSubMenu->addAction(cropAction);

    addMenuSeparator(transformSubMenu);
    transformSubMenu->addAction(keepTransformAction);
    imageViewer->ImagePopUpMenu->addAction(mirrorSubMenuAction);
    imageViewer->ImagePopUpMenu->addAction(guideSubMenuAction);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    imageViewer->ImagePopUpMenu->addAction(copyToAction);
    imageViewer->ImagePopUpMenu->addAction(moveToAction);
    imageViewer->ImagePopUpMenu->addAction(saveAction);
    imageViewer->ImagePopUpMenu->addAction(saveAsAction);
    imageViewer->ImagePopUpMenu->addAction(renameAction);
    imageViewer->ImagePopUpMenu->addAction(deleteAction);
    imageViewer->ImagePopUpMenu->addAction(deletePermanentlyAction);
    imageViewer->ImagePopUpMenu->addAction(openWithMenuAction);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    viewSubMenu = new QMenu(tr("View"));
    viewSubMenuAction = new QAction(tr("View"), this);
    viewSubMenuAction->setMenu(viewSubMenu);
    imageViewer->ImagePopUpMenu->addAction(viewSubMenuAction);
    viewSubMenu->addAction(fullScreenAction);
    viewSubMenu->addAction(showClipboardAction);
    viewSubMenu->addAction(showViewerToolbarAction);
    viewSubMenu->addAction(refreshAction);
    imageViewer->ImagePopUpMenu->addAction(copyImageAction);
    imageViewer->ImagePopUpMenu->addAction(pasteImageAction);
    imageViewer->ImagePopUpMenu->addAction(CloseImageAction);
    imageViewer->ImagePopUpMenu->addAction(exitAction);

    addMenuSeparator(imageViewer->ImagePopUpMenu);
    imageViewer->ImagePopUpMenu->addAction(settingsAction);

    imageViewer->setContextMenuPolicy(Qt::DefaultContextMenu);
    Settings::isFullScreen = Settings::appSettings->value(Settings::optionFullScreenMode).toBool();
    fullScreenAction->setChecked(Settings::isFullScreen);
    thumbsViewer->setImageViewer(imageViewer);
    thumbsViewer->imagePreview->setImageViewer(imageViewer);
}

void Phototonic::createActions() {
    thumbsGoToTopAction = new QAction(tr("Top"), this);
    thumbsGoToTopAction->setObjectName("thumbsGoTop");
    thumbsGoToTopAction->setIcon(QIcon::fromTheme("go-top", QIcon(":/images/top.png")));
    connect(thumbsGoToTopAction, SIGNAL(triggered()), this, SLOT(goTop()));

    thumbsGoToBottomAction = new QAction(tr("Bottom"), this);
    thumbsGoToBottomAction->setObjectName("thumbsGoBottom");
    thumbsGoToBottomAction->setIcon(QIcon::fromTheme("go-bottom", QIcon(":/images/bottom.png")));
    connect(thumbsGoToBottomAction, SIGNAL(triggered()), this, SLOT(goBottom()));

    CloseImageAction = new QAction(tr("Close Viewer"), this);
    CloseImageAction->setObjectName("closeImage");
    connect(CloseImageAction, SIGNAL(triggered()), this, SLOT(hideViewer()));

    fullScreenAction = new QAction(tr("Full Screen"), this);
    fullScreenAction->setObjectName("fullScreen");
    fullScreenAction->setCheckable(true);
    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

    settingsAction = new QAction(tr("Preferences"), this);
    settingsAction->setObjectName("settings");
    settingsAction->setIcon(QIcon::fromTheme("preferences-system", QIcon(":/images/settings.png")));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

    exitAction = new QAction(tr("Exit"), this);
    exitAction->setObjectName("exit");
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    thumbsZoomInAction = new QAction(tr("Enlarge Thumbnails"), this);
    thumbsZoomInAction->setObjectName("thumbsZoomIn");
    connect(thumbsZoomInAction, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
    thumbsZoomInAction->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
    if (thumbsViewer->thumbSize == THUMB_SIZE_MAX) {
        thumbsZoomInAction->setEnabled(false);
    }

    thumbsZoomOutAction = new QAction(tr("Shrink Thumbnails"), this);
    thumbsZoomOutAction->setObjectName("thumbsZoomOut");
    connect(thumbsZoomOutAction, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
    thumbsZoomOutAction->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
    if (thumbsViewer->thumbSize == THUMB_SIZE_MIN) {
        thumbsZoomOutAction->setEnabled(false);
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

    setClassicThumbsAction = new QAction(tr("Show classic thumbnails"), this);
    setClassicThumbsAction->setCheckable(true);
    setClassicThumbsAction->setChecked(Settings::thumbsLayout == ThumbsViewer::Classic);
    setClassicThumbsAction->setObjectName("setClassicThumbs");
    connect(setClassicThumbsAction, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));

    setSquareThumbsAction = new QAction(tr("Show square thumbnails"), this);
    setSquareThumbsAction->setCheckable(true);
    setSquareThumbsAction->setChecked(Settings::thumbsLayout == ThumbsViewer::Squares);
    setSquareThumbsAction->setObjectName("setSquareThumbs");
    connect(setSquareThumbsAction, SIGNAL(triggered()), this, SLOT(setSquareThumbs()));

    copyToAction = new QAction(tr("Copy to..."), this);
    copyToAction->setObjectName("copyTo");
    connect(copyToAction, SIGNAL(triggered()), this, SLOT(copyImagesTo()));

    moveToAction = new QAction(tr("Move to..."), this);
    moveToAction->setObjectName("moveTo");
    connect(moveToAction, SIGNAL(triggered()), this, SLOT(moveImagesTo()));

    deleteAction = new QAction(tr("Move to Trash"), this);
    deleteAction->setObjectName("moveToTrash");
    deleteAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOperation()));

    deletePermanentlyAction = new QAction(tr("Delete"), this);
    deletePermanentlyAction->setObjectName("delete");
    deletePermanentlyAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
    connect(deletePermanentlyAction, SIGNAL(triggered()), this, SLOT(deletePermanentlyOperation()));

    saveAction = new QAction(tr("Save"), this);
    saveAction->setObjectName("save");
    saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

    saveAsAction = new QAction(tr("Save As"), this);
    saveAsAction->setObjectName("saveAs");
    saveAsAction->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/images/save_as.png")));

    copyImageAction = new QAction(tr("Copy Image"), this);
    copyImageAction->setObjectName("copyImage");
    pasteImageAction = new QAction(tr("Paste Image"), this);
    pasteImageAction->setObjectName("pasteImage");

    renameAction = new QAction(tr("Rename"), this);
    renameAction->setObjectName("rename");
    connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

    removeMetadataAction = new QAction(tr("Remove Metadata"), this);
    removeMetadataAction->setObjectName("removeMetadata");
    connect(removeMetadataAction, SIGNAL(triggered()), this, SLOT(removeMetadata()));

    selectAllAction = new QAction(tr("Select All"), this);
    selectAllAction->setObjectName("selectAll");
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

    selectByBrightnesAction = new QAction(tr("Select by Brightness"), this);
    selectByBrightnesAction->setObjectName("selectByBrightness");
    connect(selectByBrightnesAction, SIGNAL(triggered()), this, SLOT(selectByBrightness()));

    aboutAction = new QAction(tr("About"), this);
    aboutAction->setObjectName("about");
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    // Sort actions
    sortByNameAction = new QAction(tr("Sort by Name"), this);
    sortByNameAction->setObjectName("name");
    sortByTimeAction = new QAction(tr("Sort by Time"), this);
    sortByTimeAction->setObjectName("time");
    sortBySizeAction = new QAction(tr("Sort by Size"), this);
    sortBySizeAction->setObjectName("size");
    sortByTypeAction = new QAction(tr("Sort by Type"), this);
    sortByTypeAction->setObjectName("type");
    sortReverseAction = new QAction(tr("Reverse Order"), this);
    sortReverseAction->setObjectName("reverse");
    sortByNameAction->setCheckable(true);
    sortByTimeAction->setCheckable(true);
    sortBySizeAction->setCheckable(true);
    sortByTypeAction->setCheckable(true);
    sortReverseAction->setCheckable(true);
    connect(sortByNameAction, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(sortByTimeAction, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(sortBySizeAction, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(sortByTypeAction, SIGNAL(triggered()), this, SLOT(sortThumbnails()));
    connect(sortReverseAction, SIGNAL(triggered()), this, SLOT(sortThumbnails()));

    if (thumbsViewer->thumbsSortFlags & QDir::Time) {
        sortByTimeAction->setChecked(true);
    } else if (thumbsViewer->thumbsSortFlags & QDir::Size) {
        sortBySizeAction->setChecked(true);
    } else if (thumbsViewer->thumbsSortFlags & QDir::Type) {
        sortByTypeAction->setChecked(true);
    } else {
        sortByNameAction->setChecked(true);
    }
    sortReverseAction->setChecked(thumbsViewer->thumbsSortFlags & QDir::Reversed);

    showHiddenFilesAction = new QAction(tr("Show Hidden Files"), this);
    showHiddenFilesAction->setObjectName("showHidden");
    showHiddenFilesAction->setCheckable(true);
    showHiddenFilesAction->setChecked(Settings::showHiddenFiles);
    connect(showHiddenFilesAction, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

    smallToolbarIconsAction = new QAction(tr("Small Toolbar Icons"), this);
    smallToolbarIconsAction->setObjectName("smallToolbarIcons");
    smallToolbarIconsAction->setCheckable(true);
    smallToolbarIconsAction->setChecked(Settings::smallToolbarIcons);
    connect(smallToolbarIconsAction, SIGNAL(triggered()), this, SLOT(setToolbarIconSize()));

    lockDocksAction = new QAction(tr("Hide Dock Title Bars"), this);
    lockDocksAction->setObjectName("lockDocks");
    lockDocksAction->setCheckable(true);
    lockDocksAction->setChecked(Settings::hideDockTitlebars);
    connect(lockDocksAction, SIGNAL(triggered()), this, SLOT(lockDocks()));

    showViewerToolbarAction = new QAction(tr("Show Toolbar"), this);
    showViewerToolbarAction->setObjectName("showViewerToolbars");
    showViewerToolbarAction->setCheckable(true);
    showViewerToolbarAction->setChecked(Settings::showViewerToolbar);
    connect(showViewerToolbarAction, SIGNAL(triggered()), this, SLOT(toggleImageViewerToolbar()));

    refreshAction = new QAction(tr("Reload"), this);
    refreshAction->setObjectName("refresh");
    refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

    includeSubDirectoriesAction = new QAction(tr("Include Sub-directories"), this);
    includeSubDirectoriesAction->setObjectName("subFolders");
    includeSubDirectoriesAction->setIcon(QIcon(":/images/tree.png"));
    includeSubDirectoriesAction->setCheckable(true);
    connect(includeSubDirectoriesAction, SIGNAL(triggered()), this, SLOT(setIncludeSubDirs()));

    pasteAction = new QAction(tr("Paste Here"), this);
    pasteAction->setObjectName("paste");
    pasteAction->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteThumbs()));
    pasteAction->setEnabled(false);

    createDirectoryAction = new QAction(tr("New Directory"), this);
    createDirectoryAction->setObjectName("createDir");
    connect(createDirectoryAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
    createDirectoryAction->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));

    setSaveDirectoryAction = new QAction(tr("Set Save Directory"), this);
    setSaveDirectoryAction->setObjectName("setSaveDir");
    connect(setSaveDirectoryAction, SIGNAL(triggered()), this, SLOT(setSaveDirectory()));
    setSaveDirectoryAction->setIcon(QIcon::fromTheme("folder-visiting", QIcon(":/images/folder-visiting.png")));

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

    nextImageAction = new QAction(tr("Next Image"), this);
    nextImageAction->setObjectName("nextImage");
    nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
    connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));

    prevImageAction = new QAction(tr("Previous Image"), this);
    prevImageAction->setObjectName("prevImage");
    prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
    connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPreviousImage()));

    firstImageAction = new QAction(tr("First Image"), this);
    firstImageAction->setObjectName("firstImage");
    firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
    connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

    lastImageAction = new QAction(tr("Last Image"), this);
    lastImageAction->setObjectName("lastImage");
    lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
    connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

    randomImageAction = new QAction(tr("Random Image"), this);
    randomImageAction->setObjectName("randomImage");
    connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

    viewImageAction = new QAction(tr("View Image"), this);
    viewImageAction->setObjectName("open");
    viewImageAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
    connect(viewImageAction, SIGNAL(triggered()), this, SLOT(viewImage()));

    showClipboardAction = new QAction(tr("Load Clipboard"), this);
    showClipboardAction->setObjectName("showClipboard");
    showClipboardAction->setIcon(QIcon::fromTheme("insert-image", QIcon(":/images/new.png")));
    connect(showClipboardAction, SIGNAL(triggered()), this, SLOT(newImage()));

    openWithSubMenu = new QMenu(tr("Open With..."));
    openWithMenuAction = new QAction(tr("Open With..."), this);
    openWithMenuAction->setObjectName("openWithMenu");
    openWithMenuAction->setMenu(openWithSubMenu);
    externalAppsAction = new QAction(tr("External Applications"), this);
    externalAppsAction->setIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/settings.png")));
    externalAppsAction->setObjectName("chooseApp");
    connect(externalAppsAction, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));

    addBookmarkAction = new QAction(tr("Add Bookmark"), this);
    addBookmarkAction->setObjectName("addBookmark");
    addBookmarkAction->setIcon(QIcon(":/images/new_bookmark.png"));
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(addNewBookmark()));

    removeBookmarkAction = new QAction(tr("Delete Bookmark"), this);
    removeBookmarkAction->setObjectName("deleteBookmark");
    removeBookmarkAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

    zoomOutAction = new QAction(tr("Zoom Out"), this);
    zoomOutAction->setObjectName("zoomOut");
    connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));
    zoomOutAction->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

    zoomInAction = new QAction(tr("Zoom In"), this);
    zoomInAction->setObjectName("zoomIn");
    connect(zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));
    zoomInAction->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

    resetZoomAction = new QAction(tr("Reset Zoom"), this);
    resetZoomAction->setObjectName("resetZoom");
    resetZoomAction->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom.png")));
    connect(resetZoomAction, SIGNAL(triggered()), this, SLOT(resetZoom()));

    origZoomAction = new QAction(tr("Original Size"), this);
    origZoomAction->setObjectName("origZoom");
    origZoomAction->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
    connect(origZoomAction, SIGNAL(triggered()), this, SLOT(origZoom()));

    keepZoomAction = new QAction(tr("Keep Zoom"), this);
    keepZoomAction->setObjectName("keepZoom");
    keepZoomAction->setCheckable(true);
    connect(keepZoomAction, SIGNAL(triggered()), this, SLOT(keepZoom()));

    rotateLeftAction = new QAction(tr("Rotate 90 degree CCW"), this);
    rotateLeftAction->setObjectName("rotateLeft");
    rotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
    connect(rotateLeftAction, SIGNAL(triggered()), this, SLOT(rotateLeft()));

    rotateRightAction = new QAction(tr("Rotate 90 degree CW"), this);
    rotateRightAction->setObjectName("rotateRight");
    rotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
    connect(rotateRightAction, SIGNAL(triggered()), this, SLOT(rotateRight()));

    flipHorizontalAction = new QAction(tr("Flip Horizontally"), this);
    flipHorizontalAction->setObjectName("flipH");
    flipHorizontalAction->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
    connect(flipHorizontalAction, SIGNAL(triggered()), this, SLOT(flipHorizontal()));

    flipVerticalAction = new QAction(tr("Flip Vertically"), this);
    flipVerticalAction->setObjectName("flipV");
    flipVerticalAction->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
    connect(flipVerticalAction, SIGNAL(triggered()), this, SLOT(flipVertical()));

    cropAction = new QAction(tr("Cropping"), this);
    cropAction->setObjectName("crop");
    cropAction->setIcon(QIcon(":/images/crop.png"));
    connect(cropAction, SIGNAL(triggered()), this, SLOT(cropImage()));

    applyCropAndRotationAction = new QAction(tr("Apply Crop and Rotation"), this);
    applyCropAndRotationAction->setObjectName("applyCropAndRotation");
    applyCropAndRotationAction->setIcon(QIcon(":/images/crop.png"));

    resizeAction = new QAction(tr("Scale Image"), this);
    resizeAction->setObjectName("resize");
    resizeAction->setIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/scale.png")));
    connect(resizeAction, SIGNAL(triggered()), this, SLOT(scaleImage()));

    freeRotateLeftAction = new QAction(tr("Rotate 1 degree CCW"), this);
    freeRotateLeftAction->setObjectName("freeRotateLeft");
    connect(freeRotateLeftAction, SIGNAL(triggered()), this, SLOT(freeRotateLeft()));

    freeRotateRightAction = new QAction(tr("Rotate 1 degree CW"), this);
    freeRotateRightAction->setObjectName("freeRotateRight");
    connect(freeRotateRightAction, SIGNAL(triggered()), this, SLOT(freeRotateRight()));

    colorsAction = new QAction(tr("Colors"), this);
    colorsAction->setObjectName("colors");
    connect(colorsAction, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
    colorsAction->setIcon(QIcon(":/images/colors.png"));

    findDupesAction = new QAction(tr("Find Duplicate Images"), this);
    findDupesAction->setObjectName("findDupes");
    findDupesAction->setIcon(QIcon(":/images/duplicates.png"));
    findDupesAction->setCheckable(true);
    connect(findDupesAction, SIGNAL(triggered()), this, SLOT(findDuplicateImages()));

    mirrorDisabledAction = new QAction(tr("Disable Mirror"), this);
    mirrorDisabledAction->setObjectName("mirrorDisabled");
    mirrorDualAction = new QAction(tr("Dual Mirror"), this);
    mirrorDualAction->setObjectName("mirrorDual");
    mirrorTripleAction = new QAction(tr("Triple Mirror"), this);
    mirrorTripleAction->setObjectName("mirrorTriple");
    mirrorDualVerticalAction = new QAction(tr("Dual Vertical Mirror"), this);
    mirrorDualVerticalAction->setObjectName("mirrorVDual");
    mirrorQuadAction = new QAction(tr("Quad Mirror"), this);
    mirrorQuadAction->setObjectName("mirrorQuad");

    mirrorDisabledAction->setCheckable(true);
    mirrorDualAction->setCheckable(true);
    mirrorTripleAction->setCheckable(true);
    mirrorDualVerticalAction->setCheckable(true);
    mirrorQuadAction->setCheckable(true);
    connect(mirrorDisabledAction, SIGNAL(triggered()), this, SLOT(setMirrorDisabled()));
    connect(mirrorDualAction, SIGNAL(triggered()), this, SLOT(setMirrorDual()));
    connect(mirrorTripleAction, SIGNAL(triggered()), this, SLOT(setMirrorTriple()));
    connect(mirrorDualVerticalAction, SIGNAL(triggered()), this, SLOT(setMirrorVDual()));
    connect(mirrorQuadAction, SIGNAL(triggered()), this, SLOT(setMirrorQuad()));
    mirrorDisabledAction->setChecked(true);

    keepTransformAction = new QAction(tr("Keep Transformations"), this);
    keepTransformAction->setObjectName("keepTransform");
    keepTransformAction->setCheckable(true);
    connect(keepTransformAction, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

    moveLeftAction = new QAction(tr("Move Image Left"), this);
    moveLeftAction->setObjectName("moveLeft");
    connect(moveLeftAction, SIGNAL(triggered()), this, SLOT(moveLeft()));
    moveRightAction = new QAction(tr("Move Image Right"), this);
    moveRightAction->setObjectName("moveRight");
    connect(moveRightAction, SIGNAL(triggered()), this, SLOT(moveRight()));
    moveUpAction = new QAction(tr("Move Image Up"), this);
    moveUpAction->setObjectName("moveUp");
    connect(moveUpAction, SIGNAL(triggered()), this, SLOT(moveUp()));
    moveDownAction = new QAction(tr("Move Image Down"), this);
    moveDownAction->setObjectName("moveDown");
    connect(moveDownAction, SIGNAL(triggered()), this, SLOT(moveDown()));

    invertSelectionAction = new QAction(tr("Invert Selection"), this);
    invertSelectionAction->setObjectName("invertSelection");
    connect(invertSelectionAction, SIGNAL(triggered()), thumbsViewer, SLOT(invertSelection()));

    // There could be a Batch submenu if we had any more items to put there
    batchSubMenu = new QMenu(tr("Batch"));
    batchSubMenuAction = new QAction(tr("Batch"), this);
    batchSubMenuAction->setMenu(batchSubMenu);
    batchTransformAction = new QAction(tr("Repeat Rotate and Crop"), this);
    batchTransformAction->setObjectName("batchTransform");
    connect(batchTransformAction, SIGNAL(triggered()), this, SLOT(batchTransform()));
    batchSubMenu->addAction(batchTransformAction);

    filterImagesFocusAction = new QAction(tr("Filter by Name"), this);
    filterImagesFocusAction->setObjectName("filterImagesFocus");
    connect(filterImagesFocusAction, SIGNAL(triggered()), this, SLOT(filterImagesFocus()));
    setPathFocusAction = new QAction(tr("Edit Current Path"), this);
    setPathFocusAction->setObjectName("setPathFocus");
    connect(setPathFocusAction, SIGNAL(triggered()), this, SLOT(setPathFocus()));
}

void Phototonic::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(includeSubDirectoriesAction);
    fileMenu->addAction(createDirectoryAction);
    fileMenu->addAction(setSaveDirectoryAction);
    fileMenu->addAction(showClipboardAction);
    fileMenu->addAction(addBookmarkAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(copyToAction);
    editMenu->addAction(moveToAction);
    editMenu->addAction(pasteAction);
    editMenu->addAction(renameAction);
    editMenu->addAction(removeMetadataAction);
    editMenu->addAction(deleteAction);
    editMenu->addAction(deletePermanentlyAction);
    editMenu->addSeparator();
    editMenu->addAction(selectAllAction);
    editMenu->addAction(selectByBrightnesAction);
    editMenu->addAction(invertSelectionAction);
    editMenu->addAction(batchSubMenuAction);
    addAction(filterImagesFocusAction);
    addAction(setPathFocusAction);
    editMenu->addSeparator();
    editMenu->addAction(externalAppsAction);
    editMenu->addAction(settingsAction);

    goMenu = menuBar()->addMenu(tr("&Go"));
    goMenu->addAction(goBackAction);
    goMenu->addAction(goFrwdAction);
    goMenu->addAction(goUpAction);
    goMenu->addAction(goHomeAction);
    goMenu->addSeparator();
    goMenu->addAction(prevImageAction);
    goMenu->addAction(nextImageAction);
    goMenu->addSeparator();
    goMenu->addAction(thumbsGoToTopAction);
    goMenu->addAction(thumbsGoToBottomAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(slideShowAction);
    viewMenu->addSeparator();

    thumbLayoutsGroup = new QActionGroup(this);
    thumbLayoutsGroup->addAction(setClassicThumbsAction);
    thumbLayoutsGroup->addAction(setSquareThumbsAction);
    viewMenu->addActions(thumbLayoutsGroup->actions());
    viewMenu->addSeparator();

    viewMenu->addAction(thumbsZoomInAction);
    viewMenu->addAction(thumbsZoomOutAction);
    sortMenu = viewMenu->addMenu(tr("Thumbnails Sorting"));
    sortTypesGroup = new QActionGroup(this);
    sortTypesGroup->addAction(sortByNameAction);
    sortTypesGroup->addAction(sortByTimeAction);
    sortTypesGroup->addAction(sortBySizeAction);
    sortTypesGroup->addAction(sortByTypeAction);
    sortMenu->addActions(sortTypesGroup->actions());
    sortMenu->addSeparator();
    sortMenu->addAction(sortReverseAction);
    viewMenu->addSeparator();

    viewMenu->addAction(showHiddenFilesAction);
    viewMenu->addSeparator();
    viewMenu->addAction(refreshAction);
    viewMenu->addSeparator();

    viewMenu->addAction(findDupesAction);

    // thumbs viewer context menu
    thumbsViewer->addAction(viewImageAction);
    thumbsViewer->addAction(openWithMenuAction);
    thumbsViewer->addAction(cutAction);
    thumbsViewer->addAction(copyAction);
    thumbsViewer->addAction(pasteAction);
    addMenuSeparator(thumbsViewer);
    thumbsViewer->addAction(copyToAction);
    thumbsViewer->addAction(moveToAction);
    thumbsViewer->addAction(renameAction);
    thumbsViewer->addAction(removeMetadataAction);
    thumbsViewer->addAction(deleteAction);
    thumbsViewer->addAction(deletePermanentlyAction);
    addMenuSeparator(thumbsViewer);
    thumbsViewer->addAction(selectAllAction);
    thumbsViewer->addAction(selectByBrightnesAction);
    thumbsViewer->addAction(invertSelectionAction);
    thumbsViewer->addAction(batchSubMenuAction);
    thumbsViewer->setContextMenuPolicy(Qt::ActionsContextMenu);
    menuBar()->setVisible(true);
}

void Phototonic::createToolBars() {
    /* Edit */
    editToolBar = addToolBar(tr("Edit Toolbar"));
    editToolBar->setObjectName("Edit");
    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(pasteAction);
    editToolBar->addAction(deleteAction);
    editToolBar->addAction(deletePermanentlyAction);
    editToolBar->addAction(showClipboardAction);
    connect(editToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setEditToolBarVisibility()));

    /* Navigation */
    goToolBar = addToolBar(tr("Navigation Toolbar"));
    goToolBar->setObjectName("Navigation");
    goToolBar->addAction(goBackAction);
    goToolBar->addAction(goFrwdAction);
    goToolBar->addAction(goUpAction);
    goToolBar->addAction(goHomeAction);
    goToolBar->addAction(refreshAction);

    /* path bar */
    pathLineEdit = new QLineEdit;
    pathLineEdit->setCompleter(new DirCompleter(pathLineEdit));
    pathLineEdit->setMinimumWidth(200);
    pathLineEdit->setMaximumWidth(600);
    connect(pathLineEdit, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
    goToolBar->addWidget(pathLineEdit);
    goToolBar->addAction(includeSubDirectoriesAction);
    goToolBar->addAction(findDupesAction);
    connect(goToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setGoToolBarVisibility()));

    /* View */
    viewToolBar = addToolBar(tr("View Toolbar"));
    viewToolBar->setObjectName("View");
    viewToolBar->addAction(thumbsZoomInAction);
    viewToolBar->addAction(thumbsZoomOutAction);
    viewToolBar->addAction(slideShowAction);

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
    viewToolBar->addAction(settingsAction);
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
    imageToolBar->addAction(deletePermanentlyAction);
    imageToolBar->addSeparator();
    imageToolBar->addAction(zoomInAction);
    imageToolBar->addAction(zoomOutAction);
    imageToolBar->addAction(resetZoomAction);
    imageToolBar->addAction(origZoomAction);
    imageToolBar->addSeparator();
    imageToolBar->addAction(resizeAction);
    imageToolBar->addAction(rotateRightAction);
    imageToolBar->addAction(rotateLeftAction);
    imageToolBar->addAction(flipHorizontalAction);
    imageToolBar->addAction(flipVerticalAction);
    imageToolBar->addAction(cropAction);
    imageToolBar->addAction(colorsAction);
    imageToolBar->setVisible(false);
    connect(imageToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setImageToolBarVisibility()));

    setToolbarIconSize();
}

void Phototonic::setClassicThumbs() {
    Settings::thumbsLayout = ThumbsViewer::Classic;
    refreshThumbs(false);
}

void Phototonic::setSquareThumbs() {
    Settings::thumbsLayout = ThumbsViewer::Squares;
    refreshThumbs(false);
}

void Phototonic::setToolbarIconSize() {
    if (initComplete) {
        Settings::smallToolbarIcons = smallToolbarIconsAction->isChecked();
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

void Phototonic::onFileListSelected() {
    if (initComplete && fileListWidget->itemAt(0, 0)->isSelected()) {
        Settings::isFileListLoaded = true;
        fileSystemTree->clearSelection();
        refreshThumbs(true);
    }
}

void Phototonic::createFileSystemDock() {
    fileSystemDock = new QDockWidget(tr("File System"), this);
    fileSystemDock->setObjectName("File System");

    fileListWidget = new FileListWidget(fileSystemDock);
    connect(fileListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onFileListSelected()));

    fileSystemTree = new FileSystemTree(fileSystemDock);
    fileSystemTree->addAction(createDirectoryAction);
    fileSystemTree->addAction(renameAction);
    fileSystemTree->addAction(deleteAction);
    fileSystemTree->addAction(deletePermanentlyAction);
    addMenuSeparator(fileSystemTree);
    fileSystemTree->addAction(pasteAction);
    addMenuSeparator(fileSystemTree);
    fileSystemTree->addAction(openWithMenuAction);
    fileSystemTree->addAction(addBookmarkAction);
    fileSystemTree->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(fileSystemTree, SIGNAL(clicked(
                                           const QModelIndex&)), this, SLOT(goSelectedDir(
                                                                                    const QModelIndex &)));

    connect(fileSystemTree->fileSystemModel, SIGNAL(rowsRemoved(
                                                            const QModelIndex &, int, int)),
            this, SLOT(checkDirState(
                               const QModelIndex &, int, int)));

    connect(fileSystemTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
            this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

    fileSystemTree->setCurrentIndex(fileSystemTree->fileSystemModel->index(QDir::currentPath()));
    fileSystemTree->scrollTo(fileSystemTree->fileSystemModel->index(QDir::currentPath()));

    connect(fileSystemTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(updateActions()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(fileListWidget);
    mainLayout->addWidget(fileSystemTree);

    QWidget *fileSystemTreeMainWidget = new QWidget(fileSystemDock);
    fileSystemTreeMainWidget->setLayout(mainLayout);

    fileSystemDock->setWidget(fileSystemTreeMainWidget);
    connect(fileSystemDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setFileSystemDockVisibility()));
    connect(fileSystemDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setFileSystemDockVisibility()));
    addDockWidget(Qt::LeftDockWidgetArea, fileSystemDock);
}

void Phototonic::createBookmarksDock() {
    bookmarksDock = new QDockWidget(tr("Bookmarks"), this);
    bookmarksDock->setObjectName("Bookmarks");
    bookmarks = new BookMarks(bookmarksDock);
    bookmarksDock->setWidget(bookmarks);

    connect(bookmarksDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setBookmarksDockVisibility()));
    connect(bookmarksDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setBookmarksDockVisibility()));
    connect(bookmarks, SIGNAL(itemClicked(QTreeWidgetItem * , int)),
            this, SLOT(bookmarkClicked(QTreeWidgetItem * , int)));
    connect(removeBookmarkAction, SIGNAL(triggered()), bookmarks, SLOT(removeBookmark()));
    connect(bookmarks, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
            this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

    addDockWidget(Qt::LeftDockWidgetArea, bookmarksDock);

    bookmarks->addAction(pasteAction);
    bookmarks->addAction(removeBookmarkAction);
    bookmarks->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::createImagePreviewDock() {
    imagePreviewDock = new QDockWidget(tr("Preview"), this);
    imagePreviewDock->setObjectName("ImagePreview");
    imagePreviewDock->setWidget(thumbsViewer->imagePreview);
    connect(imagePreviewDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setImagePreviewDockVisibility()));
    connect(imagePreviewDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setImagePreviewDockVisibility()));
    addDockWidget(Qt::RightDockWidgetArea, imagePreviewDock);
}

void Phototonic::createImageTagsDock() {
    tagsDock = new QDockWidget(tr("Tags"), this);
    tagsDock->setObjectName("Tags");
    thumbsViewer->imageTags = new ImageTags(tagsDock, thumbsViewer, metadataCache);
    tagsDock->setWidget(thumbsViewer->imageTags);

    connect(tagsDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setTagsDockVisibility()));
    connect(tagsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setTagsDockVisibility()));
    connect(thumbsViewer->imageTags, SIGNAL(reloadThumbs()), this, SLOT(onReloadThumbs()));
    connect(thumbsViewer->imageTags->removeTagAction, SIGNAL(triggered()), this, SLOT(deleteOperation()));
}

void Phototonic::sortThumbnails() {
    thumbsViewer->thumbsSortFlags = QDir::IgnoreCase;

    if (sortByNameAction->isChecked()) {
        thumbsViewer->thumbsViewerModel->setSortRole(ThumbsViewer::SortRole);
    } else if (sortByTimeAction->isChecked()) {
        thumbsViewer->thumbsViewerModel->setSortRole(ThumbsViewer::TimeRole);
    } else if (sortBySizeAction->isChecked()) {
        thumbsViewer->thumbsViewerModel->setSortRole(ThumbsViewer::SizeRole);
    } else if (sortByTypeAction->isChecked()) {
        thumbsViewer->thumbsViewerModel->setSortRole(ThumbsViewer::TypeRole);
    }
    thumbsViewer->thumbsViewerModel->sort(0, sortReverseAction->isChecked() ? Qt::AscendingOrder : Qt::DescendingOrder);
    thumbsViewer->loadVisibleThumbs(-1);
}

void Phototonic::reload() {
    findDupesAction->setChecked(false);
    if (Settings::layoutMode == ThumbViewWidget) {
        refreshThumbs(false);
    } else {
        imageViewer->reload();
    }
}

void Phototonic::setIncludeSubDirs() {
    findDupesAction->setChecked(false);
    Settings::includeSubDirectories = includeSubDirectoriesAction->isChecked();
    refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop) {
    thumbsViewer->setNeedToScroll(scrollToTop);
    QTimer::singleShot(0, this, SLOT(onReloadThumbs()));
}

void Phototonic::showHiddenFiles() {
    Settings::showHiddenFiles = showHiddenFilesAction->isChecked();
    fileSystemTree->setModelFlags();
    refreshThumbs(false);
}

void Phototonic::toggleImageViewerToolbar() {
    imageToolBar->setVisible(showViewerToolbarAction->isChecked());
    addToolBar(imageToolBar);
    Settings::showViewerToolbar = showViewerToolbarAction->isChecked();
}

void Phototonic::about() {
    MessageBox messageBox(this);
    messageBox.about();
}

void Phototonic::filterImagesFocus() {
    if (Settings::layoutMode == ThumbViewWidget) {
        if (!viewToolBar->isVisible()) {
            viewToolBar->setVisible(true);
        }
        setViewToolBarVisibility();
        filterLineEdit->setFocus(Qt::OtherFocusReason);
        filterLineEdit->selectAll();
    }
}

void Phototonic::setPathFocus() {
    if (Settings::layoutMode == ThumbViewWidget) {
        if (!goToolBar->isVisible()) {
            goToolBar->setVisible(true);
        }
        setGoToolBarVisibility();
        pathLineEdit->setFocus(Qt::OtherFocusReason);
        pathLineEdit->selectAll();
    }
}

void Phototonic::cleanupSender() {
    if (QObject::sender()) {
        QObject::sender()->deleteLater();
    }
}

void Phototonic::externalAppError() {
    MessageBox msgBox(this);
    msgBox.critical(tr("Error"), tr("Failed to start external application."));
}

void Phototonic::runExternalApp() {
    QString execCommand;
    QString selectedFileNames("");
    execCommand = Settings::externalApps[((QAction *) sender())->text()];

    if (Settings::layoutMode == ImageViewWidget) {
        if (imageViewer->isNewImage()) {
            showNewImageWarning();
            return;
        }

        execCommand += " \"" + imageViewer->viewerImageFullPath + "\"";
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
    int actionNumber = 0;
    QMapIterator<QString, QString> externalAppsIterator(Settings::externalApps);

    QList<QAction *> actionList = openWithSubMenu->actions();
    if (!actionList.empty()) {

        for (int i = 0; i < actionList.size(); ++i) {
            QAction *action = actionList.at(i);
            if (action->isSeparator()) {
                break;
            }

            openWithSubMenu->removeAction(action);
            imageViewer->removeAction(action);
            action->deleteLater();
        }

        openWithSubMenu->clear();
    }

    while (externalAppsIterator.hasNext()) {
        ++actionNumber;
        externalAppsIterator.next();
        QAction *extAppAct = new QAction(externalAppsIterator.key(), this);
        if (actionNumber < 10) {
            extAppAct->setShortcut(QKeySequence("Alt+" + QString::number(actionNumber)));
        }
        extAppAct->setIcon(QIcon::fromTheme(externalAppsIterator.key()));
        connect(extAppAct, SIGNAL(triggered()), this, SLOT(runExternalApp()));
        openWithSubMenu->addAction(extAppAct);
        imageViewer->addAction(extAppAct);
    }

    openWithSubMenu->addSeparator();
    openWithSubMenu->addAction(externalAppsAction);
}

void Phototonic::chooseExternalApp() {
    ExternalAppsDialog *externalAppsDialog = new ExternalAppsDialog(this);

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    externalAppsDialog->exec();
    updateExternalApps();
    externalAppsDialog->deleteLater();

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::showSettings() {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    imageViewer->setCursorHiding(false);

    SettingsDialog *settingsDialog = new SettingsDialog(this);
    if (settingsDialog->exec()) {
        imageViewer->setBackgroundColor();
        thumbsViewer->setThumbColors();
        thumbsViewer->imagePreview->setBackgroundColor();
        Settings::imageZoomFactor = 1.0;
        imageViewer->imageInfoLabel->setVisible(Settings::showImageName);

        if (Settings::layoutMode == ImageViewWidget) {
            imageViewer->reload();
            needThumbsRefresh = true;
        } else {
            refreshThumbs(false);
        }

        if (!Settings::setWindowIcon) {
            setWindowIcon(defaultApplicationIcon);
        }
        writeSettings();
    }

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
    settingsDialog->deleteLater();
}

void Phototonic::toggleFullScreen() {
    if (fullScreenAction->isChecked()) {
        shouldMaximize = isMaximized();
        showFullScreen();
        Settings::isFullScreen = true;
        imageViewer->setCursorHiding(true);
    } else {
        showNormal();
        if (shouldMaximize) {
            showMaximized();
        }
        imageViewer->setCursorHiding(false);
        Settings::isFullScreen = false;
    }
}

void Phototonic::selectAllThumbs() {
    thumbsViewer->selectAll();
}

void Phototonic::selectByBrightness() {
    RangeInputDialog dlg(this);
    if (dlg.exec()) {
        qreal min = dlg.minimumValue();
        qreal max = dlg.maximumValue();
        thumbsViewer->selectByBrightness(min, max);
    }
}

void Phototonic::copyOrCutThumbs(bool isCopyOperation) {
    Settings::copyCutIndexList = thumbsViewer->selectionModel()->selectedIndexes();
    copyCutThumbsCount = Settings::copyCutIndexList.size();

    Settings::copyCutFileList.clear();

    QList<QUrl> urlList;
    for (int thumb = 0; thumb < copyCutThumbsCount; ++thumb) {
        const QString filePath = thumbsViewer->thumbsViewerModel->item(Settings::copyCutIndexList[thumb].
                row())->data(thumbsViewer->FileNameRole).toString();
        Settings::copyCutFileList.append(filePath);

        urlList.append(QUrl::fromLocalFile(filePath)); // The standard apparently is URLs even for local files...
    }

    QMimeData *mimedata = new QMimeData;
    mimedata->setUrls(urlList);
    QGuiApplication::clipboard()->setMimeData(mimedata);

    Settings::isCopyOperation = isCopyOperation;
    pasteAction->setEnabled(true);

    QString state = QString((Settings::isCopyOperation ? tr("Copied") : tr("Cut")) + " " +
                            tr("%n image(s) to clipboard", "", copyCutThumbsCount));
    setStatus(state);
}

void Phototonic::cutThumbs() {
    copyOrCutThumbs(false);
}

void Phototonic::copyThumbs() {
    copyOrCutThumbs(true);
}

void Phototonic::copyImagesTo() {
    copyOrMoveImages(false);
}

void Phototonic::moveImagesTo() {
    copyOrMoveImages(true);
}

void Phototonic::copyOrMoveImages(bool isMoveOperation) {
    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    copyMoveToDialog = new CopyMoveToDialog(this, getSelectedPath(), isMoveOperation);
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
                showNewImageWarning();
                if (isFullScreen()) {
                    imageViewer->setCursorHiding(true);
                }

                return;
            }

            QFileInfo fileInfo = QFileInfo(imageViewer->viewerImageFullPath);
            QString fileName = fileInfo.fileName();
            QString destFile = copyMoveToDialog->selectedPath + QDir::separator() + fileInfo.fileName();

            int result = CopyMoveDialog::copyOrMoveFile(copyMoveToDialog->copyOp, fileName,
                                                        imageViewer->viewerImageFullPath,
                                                        destFile, copyMoveToDialog->selectedPath);

            if (!result) {
                MessageBox msgBox(this);
                msgBox.critical(tr("Error"), tr("Failed to copy or move image."));
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
    copyMoveToDialog->deleteLater();
    copyMoveToDialog = nullptr;

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::thumbsZoomIn() {
    if (thumbsViewer->thumbSize < THUMB_SIZE_MAX) {
        thumbsViewer->thumbSize += THUMB_SIZE_MIN;
        thumbsZoomOutAction->setEnabled(true);
        if (thumbsViewer->thumbSize == THUMB_SIZE_MAX)
            thumbsZoomInAction->setEnabled(false);
        refreshThumbs(false);
    }
}

void Phototonic::thumbsZoomOut() {
    if (thumbsViewer->thumbSize > THUMB_SIZE_MIN) {
        thumbsViewer->thumbSize -= THUMB_SIZE_MIN;
        thumbsZoomInAction->setEnabled(true);
        if (thumbsViewer->thumbSize == THUMB_SIZE_MIN)
            thumbsZoomOutAction->setEnabled(false);
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
    Settings::keepZoomFactor = keepZoomAction->isChecked();
    if (Settings::keepZoomFactor) {
        imageViewer->setFeedback(tr("Zoom Locked"));
    } else {
        imageViewer->setFeedback(tr("Zoom Unlocked"));
    }
}

void Phototonic::keepTransformClicked() {
    Settings::keepTransform = keepTransformAction->isChecked();

    if (Settings::keepTransform) {
        imageViewer->setFeedback(tr("Transformations Locked"));
        if (cropDialog) {
            cropDialog->applyCrop(0);
        }
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

void Phototonic::flipVertical() {
    Settings::flipV = !Settings::flipV;
    imageViewer->refresh();
    imageViewer->setFeedback(Settings::flipV ? tr("Flipped Vertically") : tr("Unflipped Vertically"));
}

void Phototonic::flipHorizontal() {
    Settings::flipH = !Settings::flipH;
    imageViewer->refresh();
    imageViewer->setFeedback(Settings::flipH ? tr("Flipped Horizontally") : tr("Unflipped Horizontally"));
}

void Phototonic::addVerticalGuide()
{
    GuideWidget *g = new GuideWidget(imageViewer, true);
    g->move(imageViewer->getContextMenuPosition().x() - GuideWidget::halfThickness(), 0);
    g->show();
}

void Phototonic::addHorizontalGuide()
{
    GuideWidget *g = new GuideWidget(imageViewer, false);
    g->move(0, imageViewer->getContextMenuPosition().y() - GuideWidget::halfThickness());
    g->show();
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
    connect(resizeDialog, SIGNAL(accepted()), this, SLOT(cleanupResizeDialog()));
    connect(resizeDialog, SIGNAL(rejected()), this, SLOT(cleanupResizeDialog()));

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

void Phototonic::batchTransform() {
    QModelIndexList idxs = thumbsViewer->selectionModel()->selectedIndexes();
    MessageBox msgBox(this);
    if (idxs.count() < 1) {
        msgBox.critical(tr("No images selected"), tr("Please select the images to transform."));
        return;
    }
    QString saveMessage = tr("overwiting the original files");
    if (!Settings::saveDirectory.isEmpty())
        saveMessage = tr("saving the transformed images to %1").arg(Settings::saveDirectory);
    QString message(tr("Rotate %1 images by %2 degrees, then crop them to %3, %4 %5 x %6, %7?")
                    .arg(idxs.count()).arg(Settings::rotation, 0, 'f', 2)
                    .arg(Settings::cropLeft).arg(Settings::cropTop).arg(Settings::cropWidth).arg(Settings::cropHeight)
                    .arg(saveMessage));
    msgBox.setText(tr("Perform batch transformation?"));
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    if (msgBox.exec() == QMessageBox::Ok) {
        bool keepTransformWas = Settings::keepTransform;
        imageViewer->batchMode = true;
        Settings::keepTransform = true;
        for (QModelIndex i : idxs) {
            loadSelectedThumbImage(i);
            imageViewer->applyCropAndRotation();
            imageViewer->saveImage();
        }
        Settings::keepTransform = keepTransformWas;
        imageViewer->batchMode = false;
    }
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
    if (!copyCutThumbsCount) {
        return;
    }

    QString destDir;
    if (copyMoveToDialog) {
        destDir = copyMoveToDialog->selectedPath;
    } else {
        if (QApplication::focusWidget() == bookmarks) {
            if (bookmarks->currentItem()) {
                destDir = bookmarks->currentItem()->toolTip(0);
            }
        } else {
            destDir = getSelectedPath();
        }
    }

    if (!isValidPath(destDir)) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Can not copy or move to ") + destDir);
        selectCurrentViewDir();
        return;
    }

    bool pasteInCurrDir = (Settings::currentDirectory == destDir);
    QFileInfo fileInfo;
    if (!Settings::isCopyOperation && pasteInCurrDir) {
        for (int thumb = 0; thumb < Settings::copyCutFileList.size(); ++thumb) {
            fileInfo = QFileInfo(Settings::copyCutFileList[thumb]);
            if (fileInfo.absolutePath() == destDir) {
                MessageBox msgBox(this);
                msgBox.critical(tr("Error"), tr("Can not move to the same directory"));
                return;
            }
        }
    }

    CopyMoveDialog *copyMoveDialog = new CopyMoveDialog(this);
    copyMoveDialog->exec(thumbsViewer, destDir, pasteInCurrDir);
    if (pasteInCurrDir) {
        for (int thumb = 0; thumb < Settings::copyCutFileList.size(); ++thumb) {
            thumbsViewer->addThumb(Settings::copyCutFileList[thumb]);
        }
    } else {
        int row = copyMoveDialog->latestRow;
        if (thumbsViewer->thumbsViewerModel->rowCount()) {
            if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
                row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
            }

            thumbsViewer->setCurrentRow(row);
            thumbsViewer->selectThumbByRow(row);
        }
    }

    QString state = QString((Settings::isCopyOperation ? tr("Copied") : tr("Moved")) + " " +
                            tr("%n image(s)", "", copyMoveDialog->nFiles));
    setStatus(state);
    copyMoveDialog->deleteLater();
    selectCurrentViewDir();

    copyCutThumbsCount = 0;
    Settings::copyCutIndexList.clear();
    Settings::copyCutFileList.clear();
    pasteAction->setEnabled(false);

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

void Phototonic::deleteImages(bool trash) {
    // Deleting selected thumbnails
    if (thumbsViewer->selectionModel()->selectedIndexes().size() < 1) {
        setStatus(tr("No selection"));
        return;
    }

    if (Settings::deleteConfirm) {
        MessageBox msgBox(this);
        msgBox.setText(trash ? tr("Move selected images to the trash?") : tr("Permanently delete selected images?"));
        msgBox.setWindowTitle(trash ? tr("Move to Trash") : tr("Delete images"));
        msgBox.setIcon(MessageBox::Warning);
        msgBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
        msgBox.setDefaultButton(MessageBox::Yes);
        msgBox.setButtonText(MessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(MessageBox::Cancel, tr("Cancel"));

        if (msgBox.exec() != MessageBox::Yes) {
            return;
        }
    }

    // To only show progress dialog if deleting actually takes time
    QElapsedTimer timer;
    timer.start();

    // Avoid a lot of not interesting updates while deleting
    QSignalBlocker fsBlocker(fileSystemTree->fileSystemModel);
    QSignalBlocker scrollbarBlocker(thumbsViewer->verticalScrollBar());

    // Avoid reloading thumbnails all the time
    thumbsViewer->isBusy = true;

    ProgressDialog *progressDialog = new ProgressDialog(this);

    int deleteFilesCount = 0;
    bool deleteOk;
    QList<int> rows;
    int row;
    QModelIndexList indexesList;
    while ((indexesList = thumbsViewer->selectionModel()->selectedIndexes()).size()) {
        QString fileNameFullPath = thumbsViewer->thumbsViewerModel->item(
                indexesList.first().row())->data(thumbsViewer->FileNameRole).toString();

        // Only show if it takes a lot of time, since popping this up for just
        // deleting a single image is annoying
        if (timer.elapsed() > 100) {
            progressDialog->opLabel->setText("Deleting " + fileNameFullPath);
            progressDialog->show();
        }

        QString deleteError;
        if (trash) {
            deleteOk = Trash::moveToTrash(fileNameFullPath, deleteError) == Trash::Success;
        } else {
            QFile fileToRemove(fileNameFullPath);
            deleteOk = fileToRemove.remove();
            if (!deleteOk) {
                deleteError = fileToRemove.errorString();
            }
        }

        ++deleteFilesCount;
        if (deleteOk) {
            row = indexesList.first().row();
            rows << row;
            thumbsViewer->thumbsViewerModel->removeRow(row);
        } else {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"),
                            (trash ? tr("Failed to move image to the trash.") : tr("Failed to delete image.")) + "\n" +
                            deleteError);
            break;
        }

        Settings::filesList.removeOne(fileNameFullPath);

        if (progressDialog->abortOp) {
            break;
        }
    }

    if (thumbsViewer->thumbsViewerModel->rowCount() && rows.count()) {
        std::sort(rows.begin(), rows.end());
        row = rows.at(0);

        if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
            row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
        }

        thumbsViewer->setCurrentRow(row);
        thumbsViewer->selectThumbByRow(row);
    }

    progressDialog->close();
    progressDialog->deleteLater();

    QString state = QString(tr("Deleted") + " " + tr("%n image(s)", "", deleteFilesCount));
    setStatus(state);

    thumbsViewer->isBusy = false;
}

void Phototonic::deleteFromViewer(bool trash) {
    if (imageViewer->isNewImage()) {
        showNewImageWarning();
        return;
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }
    imageViewer->setCursorHiding(false);

    bool ok;
    QFileInfo fileInfo = QFileInfo(imageViewer->viewerImageFullPath);
    QString fileName = fileInfo.fileName();

    bool deleteConfirmed = true;
    if (Settings::deleteConfirm) {
        MessageBox msgBox(this);
        msgBox.setText(trash ? tr("Move %1 to the trash").arg(fileName) : tr("Permanently delete %1").arg(fileName));
        msgBox.setWindowTitle(trash ? tr("Move to Trash") : tr("Delete images"));
        msgBox.setIcon(MessageBox::Warning);
        msgBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
        msgBox.setDefaultButton(MessageBox::Yes);
        msgBox.setButtonText(MessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(MessageBox::Cancel, tr("Cancel"));

        if (msgBox.exec() != MessageBox::Yes) {
            deleteConfirmed = false;
        }
    }

    if (deleteConfirmed) {
        int currentRow = thumbsViewer->getCurrentRow();

        QString trashError;
        ok = trash ? (Trash::moveToTrash(imageViewer->viewerImageFullPath, trashError) == Trash::Success)
                   : QFile::remove(imageViewer->viewerImageFullPath);
        if (ok) {
            thumbsViewer->thumbsViewerModel->removeRow(currentRow);
            imageViewer->setFeedback(tr("Deleted ") + fileName);
        } else {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), trash ? trashError : tr("Failed to delete image"));
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

// Main delete operation
void Phototonic::deleteOperation() {
    if (QApplication::focusWidget() == thumbsViewer->imageTags->tagsTree) {
        thumbsViewer->imageTags->removeTag();
        return;
    }

    if (QApplication::focusWidget() == bookmarks) {
        bookmarks->removeBookmark();
        return;
    }

    if (QApplication::focusWidget() == fileSystemTree) {
        deleteDirectory(true);
        return;
    }

    if (Settings::layoutMode == ImageViewWidget) {
        deleteFromViewer(true);
        return;
    }

    deleteImages(true);
}

void Phototonic::deletePermanentlyOperation() {
    if (QApplication::focusWidget() == fileSystemTree) {
        deleteDirectory(false);
        return;
    }

    if (Settings::layoutMode == ImageViewWidget) {
        deleteFromViewer(false);
        return;
    }

    deleteImages(false);
}

void Phototonic::goTo(QString path) {
    findDupesAction->setChecked(false);
    Settings::isFileListLoaded = false;
    fileListWidget->clearSelection();
    thumbsViewer->setNeedToScroll(true);
    fileSystemTree->setCurrentIndex(fileSystemTree->fileSystemModel->index(path));
    Settings::currentDirectory = path;
    refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex &idx) {
    findDupesAction->setChecked(false);
    Settings::isFileListLoaded = false;
    fileListWidget->clearSelection();
    thumbsViewer->setNeedToScroll(true);
    Settings::currentDirectory = getSelectedPath();
    refreshThumbs(true);
    fileSystemTree->expand(idx);
}

void Phototonic::goPathBarDir() {
    findDupesAction->setChecked(false);
    thumbsViewer->setNeedToScroll(true);

    QDir checkPath(pathLineEdit->text());
    if (!checkPath.exists() || !checkPath.isReadable()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Invalid Path:") + " " + pathLineEdit->text());
        pathLineEdit->setText(Settings::currentDirectory);
        return;
    }

    Settings::currentDirectory = pathLineEdit->text();
    refreshThumbs(true);
    selectCurrentViewDir();
}

void Phototonic::bookmarkClicked(QTreeWidgetItem *item, int col) {
    goTo(item->toolTip(col));
}

void Phototonic::setThumbsFilter() {
    thumbsViewer->filterString = filterLineEdit->text();
    refreshThumbs(true);
}

void Phototonic::clearThumbsFilter() {
    if (filterLineEdit->text() == "") {
        thumbsViewer->filterString = filterLineEdit->text();
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
    QFileInfo fileInfo = QFileInfo(Settings::currentDirectory);
    goTo(fileInfo.dir().absolutePath());
}

void Phototonic::goHome() {
    goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled) {
    cutAction->setEnabled(setEnabled);
    copyAction->setEnabled(setEnabled);
}

void Phototonic::updateActions() {
    if (QApplication::focusWidget() == thumbsViewer) {
        bool hasSelectedItems = thumbsViewer->selectionModel()->selectedIndexes().size() > 0;
        setCopyCutActions(hasSelectedItems);
    } else if (QApplication::focusWidget() == bookmarks) {
        setCopyCutActions(false);
    } else if (QApplication::focusWidget() == fileSystemTree) {
        setCopyCutActions(false);
    } else if (Settings::layoutMode == ImageViewWidget || QApplication::focusWidget() == imageViewer->scrollArea) {
        setCopyCutActions(false);
    } else {
        setCopyCutActions(false);
    }

    if (Settings::layoutMode == ImageViewWidget && !interfaceDisabled) {
        setViewerKeyEventsEnabled(true);
        fullScreenAction->setEnabled(true);
        CloseImageAction->setEnabled(true);
    } else {
        if (QApplication::focusWidget() == imageViewer->scrollArea) {
            setViewerKeyEventsEnabled(true);
            fullScreenAction->setEnabled(false);
            CloseImageAction->setEnabled(false);
        } else {
            setViewerKeyEventsEnabled(false);
            fullScreenAction->setEnabled(false);
            CloseImageAction->setEnabled(false);
        }
    }
}

void Phototonic::writeSettings() {
    if (Settings::layoutMode == ThumbViewWidget) {
        Settings::appSettings->setValue(Settings::optionGeometry, saveGeometry());
        Settings::appSettings->setValue(Settings::optionWindowState, saveState());
    }

    Settings::appSettings->setValue(Settings::optionThumbsSortFlags, (int) thumbsViewer->thumbsSortFlags);
    Settings::appSettings->setValue(Settings::optionThumbsZoomLevel, thumbsViewer->thumbSize);
    Settings::appSettings->setValue(Settings::optionFullScreenMode, (bool) Settings::isFullScreen);
    Settings::appSettings->setValue(Settings::optionViewerBackgroundColor, Settings::viewerBackgroundColor);
    Settings::appSettings->setValue(Settings::optionThumbsBackgroundColor, Settings::thumbsBackgroundColor);
    Settings::appSettings->setValue(Settings::optionThumbsTextColor, Settings::thumbsTextColor);
    Settings::appSettings->setValue(Settings::optionThumbsPagesReadCount, (int) Settings::thumbsPagesReadCount);
    Settings::appSettings->setValue(Settings::optionThumbsLayout, (int) Settings::thumbsLayout);
    Settings::appSettings->setValue(Settings::optionEnableAnimations, (bool) Settings::enableAnimations);
    Settings::appSettings->setValue(Settings::optionExifRotationEnabled, (bool) Settings::exifRotationEnabled);
    Settings::appSettings->setValue(Settings::optionExifThumbRotationEnabled,
                                    (bool) Settings::exifThumbRotationEnabled);
    Settings::appSettings->setValue(Settings::optionReverseMouseBehavior, (bool) Settings::reverseMouseBehavior);
    Settings::appSettings->setValue(Settings::optionDeleteConfirm, (bool) Settings::deleteConfirm);
    Settings::appSettings->setValue(Settings::optionShowHiddenFiles, (bool) Settings::showHiddenFiles);
    Settings::appSettings->setValue(Settings::optionWrapImageList, (bool) Settings::wrapImageList);
    Settings::appSettings->setValue(Settings::optionImageZoomFactor, Settings::imageZoomFactor);
    Settings::appSettings->setValue(Settings::optionShouldMaximize, (bool) isMaximized());
    Settings::appSettings->setValue(Settings::optionDefaultSaveQuality, Settings::defaultSaveQuality);
    Settings::appSettings->setValue(Settings::optionSlideShowDelay, Settings::slideShowDelay);
    Settings::appSettings->setValue(Settings::optionSlideShowRandom, (bool) Settings::slideShowRandom);
    Settings::appSettings->setValue(Settings::optionEditToolBarVisible, (bool) editToolBarVisible);
    Settings::appSettings->setValue(Settings::optionGoToolBarVisible, (bool) goToolBarVisible);
    Settings::appSettings->setValue(Settings::optionViewToolBarVisible, (bool) viewToolBarVisible);
    Settings::appSettings->setValue(Settings::optionImageToolBarVisible, (bool) imageToolBarVisible);
    Settings::appSettings->setValue(Settings::optionFileSystemDockVisible, (bool) Settings::fileSystemDockVisible);
    Settings::appSettings->setValue(Settings::optionImageInfoDockVisible, (bool) Settings::imageInfoDockVisible);
    Settings::appSettings->setValue(Settings::optionBookmarksDockVisible, (bool) Settings::bookmarksDockVisible);
    Settings::appSettings->setValue(Settings::optionTagsDockVisible, (bool) Settings::tagsDockVisible);
    Settings::appSettings->setValue(Settings::optionImagePreviewDockVisible, (bool) Settings::imagePreviewDockVisible);
    Settings::appSettings->setValue(Settings::optionStartupDir, (int) Settings::startupDir);
    Settings::appSettings->setValue(Settings::optionSpecifiedStartDir, Settings::specifiedStartDir);
    Settings::appSettings->setValue(Settings::optionThumbsBackgroundImage, Settings::thumbsBackgroundImage);
    Settings::appSettings->setValue(Settings::optionLastDir,
                                    Settings::startupDir == Settings::RememberLastDir ? Settings::currentDirectory
                                                                                      : "");
    Settings::appSettings->setValue(Settings::optionShowImageName, (bool) Settings::showImageName);
    Settings::appSettings->setValue(Settings::optionSmallToolbarIcons, (bool) Settings::smallToolbarIcons);
    Settings::appSettings->setValue(Settings::optionHideDockTitlebars, (bool) Settings::hideDockTitlebars);
    Settings::appSettings->setValue(Settings::optionShowViewerToolbar, (bool) Settings::showViewerToolbar);
    Settings::appSettings->setValue(Settings::optionSetWindowIcon, (bool) Settings::setWindowIcon);

    /* Action shortcuts */
    Settings::appSettings->beginGroup(Settings::optionShortcuts);
    QMapIterator<QString, QAction *> shortcutsIterator(Settings::actionKeys);
    while (shortcutsIterator.hasNext()) {
        shortcutsIterator.next();
        Settings::appSettings->setValue(shortcutsIterator.key(), shortcutsIterator.value()->shortcut().toString());
    }
    Settings::appSettings->endGroup();

    /* External apps */
    Settings::appSettings->beginGroup(Settings::optionExternalApps);
    Settings::appSettings->remove("");
    QMapIterator<QString, QString> eaIter(Settings::externalApps);
    while (eaIter.hasNext()) {
        eaIter.next();
        Settings::appSettings->setValue(eaIter.key(), eaIter.value());
    }
    Settings::appSettings->endGroup();

    /* save bookmarks */
    int idx = 0;
    Settings::appSettings->beginGroup(Settings::optionCopyMoveToPaths);
    Settings::appSettings->remove("");
    QSetIterator<QString> pathsIter(Settings::bookmarkPaths);
    while (pathsIter.hasNext()) {
        Settings::appSettings->setValue("path" + QString::number(++idx), pathsIter.next());
    }
    Settings::appSettings->endGroup();

    /* save known Tags */
    idx = 0;
    Settings::appSettings->beginGroup(Settings::optionKnownTags);
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
        Settings::appSettings->setValue(Settings::optionViewerBackgroundColor, QColor(25, 25, 25));
        Settings::appSettings->setValue(Settings::optionThumbsBackgroundColor, QColor(200, 200, 200));
        Settings::appSettings->setValue(Settings::optionThumbsTextColor, QColor(25, 25, 25));
        Settings::appSettings->setValue(Settings::optionThumbsPagesReadCount, (int) 2);
        Settings::appSettings->setValue(Settings::optionThumbsLayout, (int) ThumbsViewer::Classic);
        Settings::appSettings->setValue(Settings::optionViewerZoomOutFlags, (int) 1);
        Settings::appSettings->setValue(Settings::optionViewerZoomInFlags, (int) 0);
        Settings::appSettings->setValue(Settings::optionWrapImageList, (bool) false);
        Settings::appSettings->setValue(Settings::optionImageZoomFactor, (float) 1.0);
        Settings::appSettings->setValue(Settings::optionDefaultSaveQuality, (int) 90);
        Settings::appSettings->setValue(Settings::optionEnableAnimations, (bool) true);
        Settings::appSettings->setValue(Settings::optionExifRotationEnabled, (bool) true);
        Settings::appSettings->setValue(Settings::optionExifThumbRotationEnabled, (bool) false);
        Settings::appSettings->setValue(Settings::optionReverseMouseBehavior, (bool) false);
        Settings::appSettings->setValue(Settings::optionDeleteConfirm, (bool) true);
        Settings::appSettings->setValue(Settings::optionShowHiddenFiles, (bool) false);
        Settings::appSettings->setValue(Settings::optionSlideShowDelay, (int) 5);
        Settings::appSettings->setValue(Settings::optionSlideShowRandom, (bool) false);
        Settings::appSettings->setValue(Settings::optionEditToolBarVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionGoToolBarVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionViewToolBarVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionImageToolBarVisible, (bool) false);
        Settings::appSettings->setValue(Settings::optionFileSystemDockVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionBookmarksDockVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionTagsDockVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionImagePreviewDockVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionImageInfoDockVisible, (bool) true);
        Settings::appSettings->setValue(Settings::optionShowImageName, (bool) false);
        Settings::appSettings->setValue(Settings::optionSmallToolbarIcons, (bool) false);
        Settings::appSettings->setValue(Settings::optionHideDockTitlebars, (bool) false);
        Settings::appSettings->setValue(Settings::optionShowViewerToolbar, (bool) false);
        Settings::appSettings->setValue(Settings::optionSmallToolbarIcons, (bool) true);
        Settings::bookmarkPaths.insert(QDir::homePath());
        const QString picturesLocation = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (!picturesLocation.isEmpty()) {
            Settings::bookmarkPaths.insert(picturesLocation);
        }
    }

    Settings::viewerBackgroundColor = Settings::appSettings->value(
            Settings::optionViewerBackgroundColor).value<QColor>();
    Settings::enableAnimations = Settings::appSettings->value(Settings::optionEnableAnimations).toBool();
    Settings::exifRotationEnabled = Settings::appSettings->value(Settings::optionExifRotationEnabled).toBool();
    Settings::exifThumbRotationEnabled = Settings::appSettings->value(
            Settings::optionExifThumbRotationEnabled).toBool();
    Settings::thumbsLayout = Settings::appSettings->value(
            Settings::optionThumbsLayout).toInt();
    Settings::reverseMouseBehavior = Settings::appSettings->value(Settings::optionReverseMouseBehavior).toBool();
    Settings::deleteConfirm = Settings::appSettings->value(Settings::optionDeleteConfirm).toBool();
    Settings::showHiddenFiles = Settings::appSettings->value(Settings::optionShowHiddenFiles).toBool();
    Settings::wrapImageList = Settings::appSettings->value(Settings::optionWrapImageList).toBool();
    Settings::imageZoomFactor = Settings::appSettings->value(Settings::optionImageZoomFactor).toFloat();
    Settings::zoomOutFlags = Settings::appSettings->value(Settings::optionViewerZoomOutFlags).toUInt();
    Settings::zoomInFlags = Settings::appSettings->value(Settings::optionViewerZoomInFlags).toUInt();
    Settings::rotation = 0;
    Settings::keepTransform = false;
    shouldMaximize = Settings::appSettings->value(Settings::optionShouldMaximize).toBool();
    Settings::flipH = false;
    Settings::flipV = false;
    Settings::defaultSaveQuality = Settings::appSettings->value(Settings::optionDefaultSaveQuality).toInt();
    Settings::slideShowDelay = Settings::appSettings->value(Settings::optionSlideShowDelay).toInt();
    Settings::slideShowRandom = Settings::appSettings->value(Settings::optionSlideShowRandom).toBool();
    Settings::slideShowActive = false;
    editToolBarVisible = Settings::appSettings->value(Settings::optionEditToolBarVisible).toBool();
    goToolBarVisible = Settings::appSettings->value(Settings::optionGoToolBarVisible).toBool();
    viewToolBarVisible = Settings::appSettings->value(Settings::optionViewToolBarVisible).toBool();
    imageToolBarVisible = Settings::appSettings->value(Settings::optionImageToolBarVisible).toBool();
    Settings::fileSystemDockVisible = Settings::appSettings->value(Settings::optionFileSystemDockVisible).toBool();
    Settings::bookmarksDockVisible = Settings::appSettings->value(Settings::optionBookmarksDockVisible).toBool();
    Settings::tagsDockVisible = Settings::appSettings->value(Settings::optionTagsDockVisible).toBool();
    Settings::imagePreviewDockVisible = Settings::appSettings->value(Settings::optionImagePreviewDockVisible).toBool();
    Settings::imageInfoDockVisible = Settings::appSettings->value(Settings::optionImageInfoDockVisible).toBool();
    Settings::startupDir = (Settings::StartupDir) Settings::appSettings->value(Settings::optionStartupDir).toInt();
    Settings::specifiedStartDir = Settings::appSettings->value(Settings::optionSpecifiedStartDir).toString();
    Settings::thumbsBackgroundImage = Settings::appSettings->value(Settings::optionThumbsBackgroundImage).toString();
    Settings::showImageName = Settings::appSettings->value(Settings::optionShowImageName).toBool();
    Settings::smallToolbarIcons = Settings::appSettings->value(Settings::optionSmallToolbarIcons).toBool();
    Settings::hideDockTitlebars = Settings::appSettings->value(Settings::optionHideDockTitlebars).toBool();
    Settings::showViewerToolbar = Settings::appSettings->value(Settings::optionShowViewerToolbar).toBool();
    Settings::setWindowIcon = Settings::appSettings->value(Settings::optionSetWindowIcon).toBool();

    /* read external apps */
    Settings::appSettings->beginGroup(Settings::optionExternalApps);
    QStringList extApps = Settings::appSettings->childKeys();
    for (int i = 0; i < extApps.size(); ++i) {
        Settings::externalApps[extApps.at(i)] = Settings::appSettings->value(extApps.at(i)).toString();
    }
    Settings::appSettings->endGroup();

    /* read bookmarks */
    Settings::appSettings->beginGroup(Settings::optionCopyMoveToPaths);
    QStringList paths = Settings::appSettings->childKeys();
    for (int i = 0; i < paths.size(); ++i) {
        Settings::bookmarkPaths.insert(Settings::appSettings->value(paths.at(i)).toString());
    }
    Settings::appSettings->endGroup();

    /* read known tags */
    Settings::appSettings->beginGroup(Settings::optionKnownTags);
    QStringList tags = Settings::appSettings->childKeys();
    for (int i = 0; i < tags.size(); ++i) {
        Settings::knownTags.insert(Settings::appSettings->value(tags.at(i)).toString());
    }
    Settings::appSettings->endGroup();

    Settings::isFileListLoaded = false;
}

void Phototonic::setupDocks() {

    addDockWidget(Qt::RightDockWidgetArea, imageInfoDock);
    addDockWidget(Qt::RightDockWidgetArea, tagsDock);

    menuBar()->addMenu(createPopupMenu())->setText(tr("Window"));
    menuBar()->addSeparator();
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

    fileSystemDockOrigWidget = fileSystemDock->titleBarWidget();
    bookmarksDockOrigWidget = bookmarksDock->titleBarWidget();
    imagePreviewDockOrigWidget = imagePreviewDock->titleBarWidget();
    tagsDockOrigWidget = tagsDock->titleBarWidget();
    imageInfoDockOrigWidget = imageInfoDock->titleBarWidget();
    fileSystemDockEmptyWidget = new QWidget;
    bookmarksDockEmptyWidget = new QWidget;
    imagePreviewDockEmptyWidget = new QWidget;
    tagsDockEmptyWidget = new QWidget;
    imageInfoDockEmptyWidget = new QWidget;
    lockDocks();
}

void Phototonic::lockDocks() {
    if (initComplete)
        Settings::hideDockTitlebars = lockDocksAction->isChecked();

    if (Settings::hideDockTitlebars) {
        fileSystemDock->setTitleBarWidget(fileSystemDockEmptyWidget);
        bookmarksDock->setTitleBarWidget(bookmarksDockEmptyWidget);
        imagePreviewDock->setTitleBarWidget(imagePreviewDockEmptyWidget);
        tagsDock->setTitleBarWidget(tagsDockEmptyWidget);
        imageInfoDock->setTitleBarWidget(imageInfoDockEmptyWidget);
    } else {
        fileSystemDock->setTitleBarWidget(fileSystemDockOrigWidget);
        bookmarksDock->setTitleBarWidget(bookmarksDockOrigWidget);
        imagePreviewDock->setTitleBarWidget(imagePreviewDockOrigWidget);
        tagsDock->setTitleBarWidget(tagsDockOrigWidget);
        imageInfoDock->setTitleBarWidget(imageInfoDockOrigWidget);
    }
}

QMenu *Phototonic::createPopupMenu() {
    QMenu *extraActsMenu = QMainWindow::createPopupMenu();
    extraActsMenu->addSeparator();
    extraActsMenu->addAction(smallToolbarIconsAction);
    extraActsMenu->addAction(lockDocksAction);
    return extraActsMenu;
}

void Phototonic::loadShortcuts() {
    // Add customizable key shortcut actions
    Settings::actionKeys[thumbsGoToTopAction->objectName()] = thumbsGoToTopAction;
    Settings::actionKeys[thumbsGoToBottomAction->objectName()] = thumbsGoToBottomAction;
    Settings::actionKeys[CloseImageAction->objectName()] = CloseImageAction;
    Settings::actionKeys[fullScreenAction->objectName()] = fullScreenAction;
    Settings::actionKeys[settingsAction->objectName()] = settingsAction;
    Settings::actionKeys[exitAction->objectName()] = exitAction;
    Settings::actionKeys[thumbsZoomInAction->objectName()] = thumbsZoomInAction;
    Settings::actionKeys[thumbsZoomOutAction->objectName()] = thumbsZoomOutAction;
    Settings::actionKeys[cutAction->objectName()] = cutAction;
    Settings::actionKeys[copyAction->objectName()] = copyAction;
    Settings::actionKeys[nextImageAction->objectName()] = nextImageAction;
    Settings::actionKeys[prevImageAction->objectName()] = prevImageAction;
    Settings::actionKeys[deletePermanentlyAction->objectName()] = deletePermanentlyAction;
    Settings::actionKeys[deleteAction->objectName()] = deleteAction;
    Settings::actionKeys[saveAction->objectName()] = saveAction;
    Settings::actionKeys[saveAsAction->objectName()] = saveAsAction;
    Settings::actionKeys[keepTransformAction->objectName()] = keepTransformAction;
    Settings::actionKeys[keepZoomAction->objectName()] = keepZoomAction;
    Settings::actionKeys[showClipboardAction->objectName()] = showClipboardAction;
    Settings::actionKeys[copyImageAction->objectName()] = copyImageAction;
    Settings::actionKeys[pasteImageAction->objectName()] = pasteImageAction;
    Settings::actionKeys[renameAction->objectName()] = renameAction;
    Settings::actionKeys[refreshAction->objectName()] = refreshAction;
    Settings::actionKeys[pasteAction->objectName()] = pasteAction;
    Settings::actionKeys[goBackAction->objectName()] = goBackAction;
    Settings::actionKeys[goFrwdAction->objectName()] = goFrwdAction;
    Settings::actionKeys[slideShowAction->objectName()] = slideShowAction;
    Settings::actionKeys[firstImageAction->objectName()] = firstImageAction;
    Settings::actionKeys[lastImageAction->objectName()] = lastImageAction;
    Settings::actionKeys[randomImageAction->objectName()] = randomImageAction;
    Settings::actionKeys[viewImageAction->objectName()] = viewImageAction;
    Settings::actionKeys[zoomOutAction->objectName()] = zoomOutAction;
    Settings::actionKeys[zoomInAction->objectName()] = zoomInAction;
    Settings::actionKeys[resetZoomAction->objectName()] = resetZoomAction;
    Settings::actionKeys[origZoomAction->objectName()] = origZoomAction;
    Settings::actionKeys[rotateLeftAction->objectName()] = rotateLeftAction;
    Settings::actionKeys[rotateRightAction->objectName()] = rotateRightAction;
    Settings::actionKeys[freeRotateLeftAction->objectName()] = freeRotateLeftAction;
    Settings::actionKeys[freeRotateRightAction->objectName()] = freeRotateRightAction;
    Settings::actionKeys[flipHorizontalAction->objectName()] = flipHorizontalAction;
    Settings::actionKeys[flipVerticalAction->objectName()] = flipVerticalAction;
    Settings::actionKeys[cropAction->objectName()] = cropAction;
    Settings::actionKeys[applyCropAndRotationAction->objectName()] = applyCropAndRotationAction;
    Settings::actionKeys[colorsAction->objectName()] = colorsAction;
    Settings::actionKeys[mirrorDisabledAction->objectName()] = mirrorDisabledAction;
    Settings::actionKeys[mirrorDualAction->objectName()] = mirrorDualAction;
    Settings::actionKeys[mirrorTripleAction->objectName()] = mirrorTripleAction;
    Settings::actionKeys[mirrorDualVerticalAction->objectName()] = mirrorDualVerticalAction;
    Settings::actionKeys[mirrorQuadAction->objectName()] = mirrorQuadAction;
    Settings::actionKeys[moveDownAction->objectName()] = moveDownAction;
    Settings::actionKeys[moveUpAction->objectName()] = moveUpAction;
    Settings::actionKeys[moveRightAction->objectName()] = moveRightAction;
    Settings::actionKeys[moveLeftAction->objectName()] = moveLeftAction;
    Settings::actionKeys[copyToAction->objectName()] = copyToAction;
    Settings::actionKeys[moveToAction->objectName()] = moveToAction;
    Settings::actionKeys[goUpAction->objectName()] = goUpAction;
    Settings::actionKeys[resizeAction->objectName()] = resizeAction;
    Settings::actionKeys[filterImagesFocusAction->objectName()] = filterImagesFocusAction;
    Settings::actionKeys[setPathFocusAction->objectName()] = setPathFocusAction;
    Settings::actionKeys[invertSelectionAction->objectName()] = invertSelectionAction;
    Settings::actionKeys[includeSubDirectoriesAction->objectName()] = includeSubDirectoriesAction;
    Settings::actionKeys[createDirectoryAction->objectName()] = createDirectoryAction;
    Settings::actionKeys[addBookmarkAction->objectName()] = addBookmarkAction;
    Settings::actionKeys[removeMetadataAction->objectName()] = removeMetadataAction;
    Settings::actionKeys[externalAppsAction->objectName()] = externalAppsAction;
    Settings::actionKeys[goHomeAction->objectName()] = goHomeAction;
    Settings::actionKeys[sortByNameAction->objectName()] = sortByNameAction;
    Settings::actionKeys[sortBySizeAction->objectName()] = sortBySizeAction;
    Settings::actionKeys[sortByTimeAction->objectName()] = sortByTimeAction;
    Settings::actionKeys[sortByTypeAction->objectName()] = sortByTypeAction;
    Settings::actionKeys[sortReverseAction->objectName()] = sortReverseAction;
    Settings::actionKeys[showHiddenFilesAction->objectName()] = showHiddenFilesAction;
    Settings::actionKeys[showViewerToolbarAction->objectName()] = showViewerToolbarAction;

    Settings::appSettings->beginGroup(Settings::optionShortcuts);
    QStringList groupKeys = Settings::appSettings->childKeys();

    if (groupKeys.size()) {
        if (groupKeys.contains(thumbsGoToTopAction->text())) {
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
        thumbsGoToTopAction->setShortcut(QKeySequence("Ctrl+Home"));
        thumbsGoToBottomAction->setShortcut(QKeySequence("Ctrl+End"));
        CloseImageAction->setShortcut(Qt::Key_Escape);
        fullScreenAction->setShortcut(QKeySequence("Alt+Return"));
        settingsAction->setShortcut(QKeySequence("Ctrl+P"));
        exitAction->setShortcut(QKeySequence("Ctrl+Q"));
        cutAction->setShortcut(QKeySequence("Ctrl+X"));
        copyAction->setShortcut(QKeySequence("Ctrl+C"));
        deleteAction->setShortcut(QKeySequence("Del"));
        deletePermanentlyAction->setShortcut(QKeySequence("Shift+Del"));
        saveAction->setShortcut(QKeySequence("Ctrl+S"));
        copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
        pasteImageAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
        renameAction->setShortcut(QKeySequence("F2"));
        refreshAction->setShortcut(QKeySequence("F5"));
        pasteAction->setShortcut(QKeySequence("Ctrl+V"));
        goBackAction->setShortcut(QKeySequence("Alt+Left"));
        goFrwdAction->setShortcut(QKeySequence("Alt+Right"));
        goUpAction->setShortcut(QKeySequence("Alt+Up"));
        slideShowAction->setShortcut(QKeySequence("Ctrl+W"));
        nextImageAction->setShortcut(QKeySequence("PgDown"));
        prevImageAction->setShortcut(QKeySequence("PgUp"));
        firstImageAction->setShortcut(QKeySequence("Home"));
        lastImageAction->setShortcut(QKeySequence("End"));
        randomImageAction->setShortcut(QKeySequence("Ctrl+D"));
        viewImageAction->setShortcut(QKeySequence("Return"));
        zoomOutAction->setShortcut(QKeySequence("-"));
        zoomInAction->setShortcut(QKeySequence("+"));
        resetZoomAction->setShortcut(QKeySequence("*"));
        origZoomAction->setShortcut(QKeySequence("/"));
        rotateLeftAction->setShortcut(QKeySequence("Ctrl+Left"));
        rotateRightAction->setShortcut(QKeySequence("Ctrl+Right"));
        freeRotateLeftAction->setShortcut(QKeySequence("Ctrl+Shift+Left"));
        freeRotateRightAction->setShortcut(QKeySequence("Ctrl+Shift+Right"));
        flipHorizontalAction->setShortcut(QKeySequence("Ctrl+Down"));
        flipVerticalAction->setShortcut(QKeySequence("Ctrl+Up"));
        cropAction->setShortcut(QKeySequence("Ctrl+G"));
        applyCropAndRotationAction->setShortcut(QKeySequence("Ctrl+R"));
        colorsAction->setShortcut(QKeySequence("Ctrl+O"));
        mirrorDisabledAction->setShortcut(QKeySequence("Ctrl+1"));
        mirrorDualAction->setShortcut(QKeySequence("Ctrl+2"));
        mirrorTripleAction->setShortcut(QKeySequence("Ctrl+3"));
        mirrorDualVerticalAction->setShortcut(QKeySequence("Ctrl+4"));
        mirrorQuadAction->setShortcut(QKeySequence("Ctrl+5"));
        moveDownAction->setShortcut(QKeySequence("Down"));
        moveUpAction->setShortcut(QKeySequence("Up"));
        moveLeftAction->setShortcut(QKeySequence("Left"));
        moveRightAction->setShortcut(QKeySequence("Right"));
        copyToAction->setShortcut(QKeySequence("Ctrl+Y"));
        moveToAction->setShortcut(QKeySequence("Ctrl+M"));
        resizeAction->setShortcut(QKeySequence("Ctrl+I"));
        filterImagesFocusAction->setShortcut(QKeySequence("Ctrl+F"));
        setPathFocusAction->setShortcut(QKeySequence("Ctrl+L"));
        keepTransformAction->setShortcut(QKeySequence("Ctrl+K"));
        showHiddenFilesAction->setShortcut(QKeySequence("Ctrl+H"));
    }

    Settings::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event) {
    thumbsViewer->abort();
    writeSettings();
    hide();
    if (!QApplication::clipboard()->image().isNull()) {
        QApplication::clipboard()->clear();
    }
    event->accept();
}

void Phototonic::setStatus(QString state) {
    statusLabel->setText("    " + state + "    ");
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event) {
    if (interfaceDisabled) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (Settings::layoutMode == ImageViewWidget) {
            if (Settings::reverseMouseBehavior) {
                fullScreenAction->setChecked(!(fullScreenAction->isChecked()));
                toggleFullScreen();
                event->accept();
            } else if (CloseImageAction->isEnabled()) {
                hideViewer();
                event->accept();
            }
        } else {
            if (QApplication::focusWidget() == thumbsViewer->imagePreview->scrollArea) {
                viewImage();
            }
        }
    }
}

void Phototonic::mousePressEvent(QMouseEvent *event) {
    if (interfaceDisabled) {
        return;
    }

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

            if (Settings::reverseMouseBehavior && CloseImageAction->isEnabled()) {
                hideViewer();
                event->accept();
            } else {
                fullScreenAction->setChecked(!(fullScreenAction->isChecked()));
                toggleFullScreen();
                event->accept();
            }
        }
    } else if (QApplication::focusWidget() == thumbsViewer->imagePreview->scrollArea) {
        if (event->button() == Qt::MiddleButton) {
            viewImage();
        }
    }
}

void Phototonic::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Left) {
		loadPreviousImage();
   } else if (event->key() == Qt::Key_Right) {
		loadNextImage();
   }
   event->accept();
}

void Phototonic::newImage() {
    if (Settings::layoutMode == ThumbViewWidget) {
        showViewer();
    }

    imageViewer->loadImage("");
}

void Phototonic::setDocksVisibility(bool visible) {
    fileSystemDock->setVisible(visible ? Settings::fileSystemDockVisible : false);
    bookmarksDock->setVisible(visible ? Settings::bookmarksDockVisible : false);
    imagePreviewDock->setVisible(visible ? Settings::imagePreviewDockVisible : false);
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

void Phototonic::viewImage() {
    if (Settings::layoutMode == ImageViewWidget) {
        hideViewer();
        return;
    }

    if (QApplication::focusWidget() == fileSystemTree) {
        goSelectedDir(fileSystemTree->getCurrentIndex());
        return;
    } else if (QApplication::focusWidget() == thumbsViewer
               || QApplication::focusWidget() == thumbsViewer->imagePreview->scrollArea
               || QApplication::focusWidget() == imageViewer->scrollArea) {
        QModelIndex selectedImageIndex;
        QModelIndexList selectedIndexes = thumbsViewer->selectionModel()->selectedIndexes();
        if (selectedIndexes.size() > 0) {
            selectedImageIndex = selectedIndexes.first();
        } else {
            if (thumbsViewer->thumbsViewerModel->rowCount() == 0) {
                setStatus(tr("No images"));
                return;
            }

            selectedImageIndex = thumbsViewer->thumbsViewerModel->indexFromItem(
                    thumbsViewer->thumbsViewerModel->item(0));
            thumbsViewer->selectionModel()->select(selectedImageIndex, QItemSelectionModel::Toggle);
            thumbsViewer->setCurrentRow(0);
        }

        loadSelectedThumbImage(selectedImageIndex);
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

void Phototonic::setFileSystemDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::fileSystemDockVisible = fileSystemDock->isVisible();
    }
}

void Phototonic::setBookmarksDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::bookmarksDockVisible = bookmarksDock->isVisible();
    }
}

void Phototonic::setImagePreviewDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::imagePreviewDockVisible = imagePreviewDock->isVisible();
    }
}

void Phototonic::setTagsDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::tagsDockVisible = tagsDock->isVisible();
    }
}

void Phototonic::setImageInfoDockVisibility() {
    if (Settings::layoutMode != ImageViewWidget) {
        Settings::imageInfoDockVisible = imageInfoDock->isVisible();
    }
}

void Phototonic::showViewer() {
    if (Settings::layoutMode == ThumbViewWidget) {
        Settings::layoutMode = ImageViewWidget;
        Settings::appSettings->setValue("Geometry", saveGeometry());
        Settings::appSettings->setValue("WindowState", saveState());

        stackedLayout->setCurrentWidget(imageViewer);
        setDocksVisibility(false);

        if (Settings::isFullScreen) {
            shouldMaximize = isMaximized();
            showFullScreen();
            imageViewer->setCursorHiding(true);
            QApplication::processEvents();
        }
        imageViewer->setFocus(Qt::OtherFocusReason);
    }
}

void Phototonic::showBusyAnimation(bool busy) {
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

void Phototonic::loadSelectedThumbImage(const QModelIndex &idx) {
    thumbsViewer->setCurrentRow(idx.row());
    showViewer();
    imageViewer->loadImage(
            thumbsViewer->thumbsViewerModel->item(idx.row())->data(thumbsViewer->FileNameRole).toString());
    thumbsViewer->setImageViewerWindowTitle();
}

void Phototonic::loadImageFromCliArguments(QString cliFileName) {
    QFile imageFile(cliFileName);
    if (!imageFile.exists()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Failed to open file %1, file not found.").arg(cliFileName));
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
        SlideShowTimer->deleteLater();
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

    int nextThumb = thumbsViewer->getNextRow();
    if (nextThumb < 0) {
        if (Settings::wrapImageList) {
            nextThumb = 0;
        } else {
            return;
        }
    }

    if (Settings::layoutMode == ImageViewWidget) {
        imageViewer->loadImage(
                thumbsViewer->thumbsViewerModel->item(nextThumb)->data(thumbsViewer->FileNameRole).toString());
    }

    thumbsViewer->setCurrentRow(nextThumb);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(nextThumb);
    }
}

void Phototonic::loadPreviousImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() <= 0) {
        return;
    }

    int previousThumb = thumbsViewer->getPrevRow();
    if (previousThumb < 0) {
        if (Settings::wrapImageList) {
            previousThumb = thumbsViewer->getLastRow();
        } else {
            return;
        }
    }

    if (Settings::layoutMode == ImageViewWidget) {
        imageViewer->loadImage(
                thumbsViewer->thumbsViewerModel->item(previousThumb)->data(thumbsViewer->FileNameRole).toString());
    }

    thumbsViewer->setCurrentRow(previousThumb);
    thumbsViewer->setImageViewerWindowTitle();

    if (Settings::layoutMode == ThumbViewWidget) {
        thumbsViewer->selectThumbByRow(previousThumb);
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
    nextImageAction->setEnabled(enabled);
    prevImageAction->setEnabled(enabled);
    moveLeftAction->setEnabled(enabled);
    moveRightAction->setEnabled(enabled);
    moveUpAction->setEnabled(enabled);
    moveDownAction->setEnabled(enabled);
}

void Phototonic::updateIndexByViewerImage() {
    if (thumbsViewer->thumbsViewerModel->rowCount() > 0 &&
        thumbsViewer->setCurrentIndexByName(imageViewer->viewerImageFullPath)) {
        thumbsViewer->selectCurrentIndex();
    }
}

void Phototonic::hideViewer() {
    if (isFullScreen()) {
        showNormal();
        if (shouldMaximize) {
            showMaximized();
        }
        imageViewer->setCursorHiding(false);
    }

    restoreGeometry(Settings::appSettings->value(Settings::optionGeometry).toByteArray());
    restoreState(Settings::appSettings->value(Settings::optionWindowState).toByteArray());

    Settings::layoutMode = ThumbViewWidget;
    stackedLayout->setCurrentWidget(thumbsViewer);

    setDocksVisibility(true);
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    setThumbsViewerWindowTitle();

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    if (needThumbsRefresh) {
        needThumbsRefresh = false;
        refreshThumbs(true);
    } else {
        if (thumbsViewer->thumbsViewerModel->rowCount() > 0) {
            if (thumbsViewer->setCurrentIndexByName(imageViewer->viewerImageFullPath)) {
                thumbsViewer->selectCurrentIndex();
            }
        }

        thumbsViewer->loadVisibleThumbs();
    }

    imageViewer->clearImage();
    thumbsViewer->setFocus(Qt::OtherFocusReason);
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void Phototonic::goBottom() {
    thumbsViewer->scrollToBottom();
}

void Phototonic::goTop() {
    thumbsViewer->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString copyMoveDirPath) {
    QApplication::restoreOverrideCursor();
    Settings::isCopyOperation = (keyMods == Qt::ControlModifier);
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

    MessageBox msgBox(this);
    if (!isValidPath(destDir)) {
        msgBox.critical(tr("Error"), tr("Can not move or copy images to this directory."));
        selectCurrentViewDir();
        return;
    }

    if (destDir == Settings::currentDirectory) {
        msgBox.critical(tr("Error"), tr("Destination directory is the same as the source directory."));
        return;
    }

    if (dirOp) {
        QString dirOnly = copyMoveDirPath.right(
                copyMoveDirPath.size() - copyMoveDirPath.lastIndexOf(QDir::separator()) - 1);

        QString question = tr("Move directory %1 to %2?").arg(dirOnly).arg(destDir);

        MessageBox moveDirMessageBox(this);
        moveDirMessageBox.setText(question);
        moveDirMessageBox.setWindowTitle(tr("Move directory"));
        moveDirMessageBox.setIcon(MessageBox::Warning);
        moveDirMessageBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
        moveDirMessageBox.setDefaultButton(MessageBox::Cancel);
        moveDirMessageBox.setButtonText(MessageBox::Yes, tr("Move Directory"));
        moveDirMessageBox.setButtonText(MessageBox::Cancel, tr("Cancel"));
        int ret = moveDirMessageBox.exec();

        if (ret == MessageBox::Yes) {
            QFile dir(copyMoveDirPath);
            bool moveOk = dir.rename(destDir + QDir::separator() + dirOnly);
            if (!moveOk) {
                moveDirMessageBox.critical(tr("Error"), tr("Failed to move directory."));
            }
            setStatus(tr("Directory moved"));
        }
    } else {
        CopyMoveDialog *copyMoveDialog = new CopyMoveDialog(this);
        Settings::copyCutIndexList = thumbsViewer->selectionModel()->selectedIndexes();
        copyMoveDialog->exec(thumbsViewer, destDir, false);

        if (!Settings::isCopyOperation) {
            int row = copyMoveDialog->latestRow;
            if (thumbsViewer->thumbsViewerModel->rowCount()) {
                if (row >= thumbsViewer->thumbsViewerModel->rowCount()) {
                    row = thumbsViewer->thumbsViewerModel->rowCount() - 1;
                }

                thumbsViewer->setCurrentRow(row);
                thumbsViewer->selectThumbByRow(row);
            }
        }

        QString stateString = QString((Settings::isCopyOperation ? tr("Copied") : tr("Moved")) + " " +
                                      tr("%n image(s)", "", copyMoveDialog->nFiles));
        setStatus(stateString);
        copyMoveDialog->deleteLater();
    }

    thumbsViewer->loadVisibleThumbs();
}

void Phototonic::selectCurrentViewDir() {
    QModelIndex idx = fileSystemTree->fileSystemModel->index(Settings::currentDirectory);
    if (idx.isValid()) {
        fileSystemTree->setCurrentIndex(idx);
        fileSystemTree->scrollTo(idx);
    }
}

void Phototonic::checkDirState(const QModelIndex &, int, int) {
    if (!initComplete) {
        return;
    }

    if (thumbsViewer->isBusy) {
        thumbsViewer->abort();
    }

    if (!QDir().exists(Settings::currentDirectory)) {
        Settings::currentDirectory.clear();
        QTimer::singleShot(0, this, SLOT(onReloadThumbs()));
    }
}

void Phototonic::addPathHistoryRecord(QString dir) {
    if (!needHistoryRecord) {
        needHistoryRecord = true;
        return;
    }

    if (pathHistoryList.size() && dir == pathHistoryList.at(currentHistoryIdx)) {
        return;
    }

    pathHistoryList.insert(++currentHistoryIdx, dir);

    // Need to clear irrelevant items from list
    if (currentHistoryIdx != pathHistoryList.size() - 1) {
        goFrwdAction->setEnabled(false);
        for (int i = pathHistoryList.size() - 1; i > currentHistoryIdx; --i) {
            pathHistoryList.removeAt(i);
        }
    }
}

void Phototonic::onReloadThumbs() {
    if (thumbsViewer->isBusy || !initComplete) {
        thumbsViewer->abort();
        QTimer::singleShot(0, this, SLOT(onReloadThumbs()));
        return;
    }

    if (!Settings::isFileListLoaded) {
        if (Settings::currentDirectory.isEmpty()) {
            Settings::currentDirectory = getSelectedPath();
            if (Settings::currentDirectory.isEmpty()) {
                return;
            }
        }

        QDir checkPath(Settings::currentDirectory);
        if (!checkPath.exists() || !checkPath.isReadable()) {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), tr("Failed to open directory ") + Settings::currentDirectory);
            setStatus(tr("No directory selected"));
            return;
        }

        thumbsViewer->infoView->clear();
        thumbsViewer->imagePreview->clear();
        if (Settings::setWindowIcon && Settings::layoutMode == Phototonic::ThumbViewWidget) {
            setWindowIcon(defaultApplicationIcon);
        }
        pathLineEdit->setText(Settings::currentDirectory);
        addPathHistoryRecord(Settings::currentDirectory);
        if (currentHistoryIdx > 0) {
            goBackAction->setEnabled(true);
        }
    }

    if (Settings::layoutMode == ThumbViewWidget) {
        setThumbsViewerWindowTitle();
    }

    if (findDupesAction->isChecked()) {
        thumbsViewer->loadDuplicates();
    } else {
        thumbsViewer->reLoad();
    }
}

void Phototonic::setThumbsViewerWindowTitle() {

    if (findDupesAction->isChecked()) {
        setWindowTitle(tr("Duplicate images in %1").arg(Settings::currentDirectory) + " - Phototonic");
    } else if (Settings::isFileListLoaded) {
        setWindowTitle(tr("Files List") + " - Phototonic");
    } else {
        setWindowTitle(Settings::currentDirectory + " - Phototonic");
    }
}

void Phototonic::renameDir() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QFileInfo dirInfo = QFileInfo(fileSystemTree->fileSystemModel->filePath(selectedDirs[0]));

    bool renameOk;
    QString title = tr("Rename") + " " + dirInfo.completeBaseName();
    QString newDirName = QInputDialog::getText(this, title,
                                               tr("New name:"), QLineEdit::Normal, dirInfo.completeBaseName(),
                                               &renameOk);

    if (!renameOk) {
        selectCurrentViewDir();
        return;
    }

    if (newDirName.isEmpty()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Invalid name entered."));
        selectCurrentViewDir();
        return;
    }

    QFile dir(dirInfo.absoluteFilePath());
    QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
    renameOk = dir.rename(newFullPathName);
    if (!renameOk) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Failed to rename directory."));
        selectCurrentViewDir();
        return;
    }

    if (Settings::currentDirectory == dirInfo.absoluteFilePath()) {
        fileSystemTree->setCurrentIndex(fileSystemTree->fileSystemModel->index(newFullPathName));
    } else {
        selectCurrentViewDir();
    }
}

void Phototonic::rename() {
    if (QApplication::focusWidget() == fileSystemTree) {
        renameDir();
        return;
    }

    if (Settings::layoutMode == ImageViewWidget) {
        if (imageViewer->isNewImage()) {
            showNewImageWarning();
            return;
        }

        if (thumbsViewer->thumbsViewerModel->rowCount() > 0) {
            if (thumbsViewer->setCurrentIndexByName(imageViewer->viewerImageFullPath))
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

    QFile currentFileFullPath(selectedImageFileName);
    QFileInfo currentFileInfo(currentFileFullPath);
    int renameConfirmed;

    RenameDialog *renameDialog = new RenameDialog(this);
    renameDialog->setModal(true);
    renameDialog->setFileName(currentFileInfo.fileName());
    renameConfirmed = renameDialog->exec();

    QString newFileName = renameDialog->getFileName();
    renameDialog->deleteLater();

    if (renameConfirmed && newFileName.isEmpty()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("No name entered."));
        renameConfirmed = 0;
    }

    if (renameConfirmed) {
        QString newFileNameFullPath = currentFileInfo.absolutePath() + QDir::separator() + newFileName;
        if (currentFileFullPath.rename(newFileNameFullPath)) {
            QModelIndexList indexesList = thumbsViewer->selectionModel()->selectedIndexes();
            thumbsViewer->thumbsViewerModel->item(indexesList.first().row())->setData(newFileNameFullPath,
                                                                                      thumbsViewer->FileNameRole);
            thumbsViewer->thumbsViewerModel->item(indexesList.first().row())->setData(newFileName, Qt::DisplayRole);

            imageViewer->setInfo(newFileName);
            imageViewer->viewerImageFullPath = newFileNameFullPath;

            if (Settings::filesList.contains(currentFileInfo.absoluteFilePath())) {
                Settings::filesList.replace(Settings::filesList.indexOf(currentFileInfo.absoluteFilePath()),
                                            newFileNameFullPath);
            }

            if (Settings::layoutMode == ImageViewWidget) {
                thumbsViewer->setImageViewerWindowTitle();
            }
        } else {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), tr("Failed to rename image."));
        }
    }

    if (isFullScreen()) {
        imageViewer->setCursorHiding(true);
    }
}

void Phototonic::removeMetadata() {

    QModelIndexList indexList = thumbsViewer->selectionModel()->selectedIndexes();
    QStringList fileList;
    copyCutThumbsCount = indexList.size();

    for (int thumb = 0; thumb < copyCutThumbsCount; ++thumb) {
        fileList.append(thumbsViewer->thumbsViewerModel->item(indexList[thumb].
                row())->data(thumbsViewer->FileNameRole).toString());
    }

    if (fileList.isEmpty()) {
        setStatus(tr("Invalid selection"));
        return;
    }

    if (Settings::slideShowActive) {
        toggleSlideShow();
    }

    MessageBox msgBox(this);
    msgBox.setText(tr("Permanently remove all Exif metadata from selected images?"));
    msgBox.setWindowTitle(tr("Remove Metadata"));
    msgBox.setIcon(MessageBox::Warning);
    msgBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
    msgBox.setDefaultButton(MessageBox::Cancel);
    msgBox.setButtonText(MessageBox::Yes, tr("Remove Metadata"));
    msgBox.setButtonText(MessageBox::Cancel, tr("Cancel"));
    int ret = msgBox.exec();

    if (ret == MessageBox::Yes) {
        for (int file = 0; file < fileList.size(); ++file) {
            Exiv2::Image::AutoPtr image;
            try {
                image = Exiv2::ImageFactory::open(fileList[file].toStdString());
                image->clearMetadata();
                image->writeMetadata();
                metadataCache->removeImage(fileList[file]);
            }
            catch (Exiv2::Error &error) {
                msgBox.critical(tr("Error"), tr("Failed to remove Exif metadata."));
                return;
            }
        }

        thumbsViewer->onSelectionChanged();
        QString state = QString(tr("Metadata removed from selected images"));
        setStatus(state);
    }
}

void Phototonic::deleteDirectory(bool trash) {
    bool removeDirectoryOk;
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QString deletePath = fileSystemTree->fileSystemModel->filePath(selectedDirs[0]);
    QModelIndex idxAbove = fileSystemTree->indexAbove(selectedDirs[0]);
    QFileInfo dirInfo = QFileInfo(deletePath);
    QString question = (trash ? tr("Move directory %1 to the trash?") : tr(
            "Permanently delete the directory %1 and all of its contents?")).arg(
            dirInfo.completeBaseName());

    MessageBox msgBox(this);
    msgBox.setText(question);
    msgBox.setWindowTitle(tr("Delete directory"));
    msgBox.setIcon(MessageBox::Warning);
    msgBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
    msgBox.setDefaultButton(MessageBox::Cancel);
    msgBox.setButtonText(MessageBox::Yes, trash ? tr("OK") : tr("Delete Directory"));
    msgBox.setButtonText(MessageBox::Cancel, tr("Cancel"));
    int ret = msgBox.exec();

    QString trashError;
    if (ret == MessageBox::Yes) {
        if (trash) {
            removeDirectoryOk = Trash::moveToTrash(deletePath, trashError) == Trash::Success;
        } else {
            removeDirectoryOk = removeDirectoryOperation(deletePath);
        }
    } else {
        selectCurrentViewDir();
        return;
    }

    if (!removeDirectoryOk) {
        msgBox.critical(tr("Error"), trash ? tr("Failed to move directory to the trash: %1").arg(trashError)
                                           : tr("Failed to delete directory."));
        selectCurrentViewDir();
        return;
    }

    QString state = QString(tr("Removed \"%1\"").arg(deletePath));
    setStatus(state);

    if (Settings::currentDirectory == deletePath) {
        if (idxAbove.isValid()) {
            fileSystemTree->setCurrentIndex(idxAbove);
        }
    } else {
        selectCurrentViewDir();
    }
}

void Phototonic::createSubDirectory() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    QFileInfo dirInfo = QFileInfo(fileSystemTree->fileSystemModel->filePath(selectedDirs[0]));

    bool ok;
    QString newDirName = QInputDialog::getText(this, tr("New Sub directory"),
                                               tr("New directory name:"), QLineEdit::Normal, "", &ok);

    if (!ok) {
        selectCurrentViewDir();
        return;
    }

    if (newDirName.isEmpty()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Invalid name entered."));
        selectCurrentViewDir();
        return;
    }

    QDir dir(dirInfo.absoluteFilePath());
    ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

    if (!ok) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Failed to create new directory."));
        selectCurrentViewDir();
        return;
    }

    setStatus(tr("Created %1").arg(newDirName));
    fileSystemTree->expand(selectedDirs[0]);
}

void Phototonic::setSaveDirectory(QString path) {
    Settings::saveDirectory = path.isEmpty() ?
        QFileDialog::getExistingDirectory(this, tr("Directory to save images into:"),
            QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks) :
        path;
}

QString Phototonic::getSelectedPath() {
    QModelIndexList selectedDirs = fileSystemTree->selectionModel()->selectedRows();
    if (selectedDirs.size() && selectedDirs[0].isValid()) {
        QFileInfo dirInfo = QFileInfo(fileSystemTree->fileSystemModel->filePath(selectedDirs[0]));
        return dirInfo.absoluteFilePath();
    } else
        return "";
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
    const int scrollDelta = event->angleDelta().y();

    if (Settings::layoutMode == ImageViewWidget) {
        if (event->modifiers() == Qt::ControlModifier) {
            if (scrollDelta < 0) {
                zoomOut();
            } else {
                zoomIn();
            }
        } else if (nextImageAction->isEnabled()) {
            if (scrollDelta < 0) {
                loadNextImage();
            } else {
                loadPreviousImage();
            }
        }
        event->accept();
    } else if (event->modifiers() == Qt::ControlModifier && QApplication::focusWidget() == thumbsViewer) {
        if (scrollDelta < 0) {
            thumbsZoomOut();
        } else {
            thumbsZoomIn();
        }
    }
}

void Phototonic::showNewImageWarning() {
    MessageBox msgBox(this);
    msgBox.warning(tr("Warning"), tr("Cannot perform action with temporary image."));
}

bool Phototonic::removeDirectoryOperation(QString dirToDelete) {
    bool removeDirOk;
    QDir dir(dirToDelete);

    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                                QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                removeDirOk = removeDirectoryOperation(info.absoluteFilePath());
            } else {
                removeDirOk = QFile::remove(info.absoluteFilePath());
            }

            if (!removeDirOk) {
                return removeDirOk;
            }
        }
    removeDirOk = dir.rmdir(dirToDelete);
    return removeDirOk;
}

void Phototonic::cleanupCropDialog() {
    setInterfaceEnabled(true);
}

void Phototonic::cleanupResizeDialog() {
    if (resizeDialog) {
        resizeDialog->deleteLater();
    }
    resizeDialog = nullptr;
    setInterfaceEnabled(true);
}

void Phototonic::cleanupColorsDialog() {
    Settings::colorsActive = false;
    setInterfaceEnabled(true);
}

void Phototonic::setInterfaceEnabled(bool enable) {
    // actions
    colorsAction->setEnabled(enable);
    renameAction->setEnabled(enable);
    removeMetadataAction->setEnabled(enable);
    cropAction->setEnabled(enable);
    resizeAction->setEnabled(enable);
    CloseImageAction->setEnabled(enable);
    nextImageAction->setEnabled(enable);
    prevImageAction->setEnabled(enable);
    firstImageAction->setEnabled(enable);
    lastImageAction->setEnabled(enable);
    randomImageAction->setEnabled(enable);
    slideShowAction->setEnabled(enable);
    copyToAction->setEnabled(enable);
    moveToAction->setEnabled(enable);
    deleteAction->setEnabled(enable);
    deletePermanentlyAction->setEnabled(enable);
    settingsAction->setEnabled(enable);
    viewImageAction->setEnabled(enable);

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

void Phototonic::findDuplicateImages()
{
    refreshThumbs(true);
}
