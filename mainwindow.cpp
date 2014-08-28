/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
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

#include "mainwindow.h"
#include "thumbview.h"
#include "global.h"

#define THUMB_SIZE_MIN	50
#define THUMB_SIZE_MAX	300

Phototonic::Phototonic(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("phototonic", "phototonic_103");
	stackedWidget = new QStackedWidget;
	readSettings();
	createThumbView();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createFSTree();
	createImageView();
	updateExternalApps();
	loadShortcuts();
	setupDocks();

	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), 
				this, SLOT(updateActions(QWidget*, QWidget*)));

	restoreGeometry(GData::appSettings->value("Geometry").toByteArray());
	restoreState(GData::appSettings->value("WindowState").toByteArray());
	setWindowIcon(QIcon(":/images/phototonic.png"));

	stackedWidget->addWidget(thumbView);
	stackedWidget->addWidget(imageView);
	setCentralWidget(stackedWidget);

	handleStartupArgs();

	copyMoveToDialog = 0;
	initComplete = true;
	thumbViewBusy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;

	refreshThumbs(true);
	if (stackedWidget->currentIndex() == thumbViewIdx)
		thumbView->setFocus(Qt::OtherFocusReason);
	if (!cliImageLoaded)
		QTimer::singleShot(100, this, SLOT(scrollToLastImage()));
}

void Phototonic::handleStartupArgs()
{
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			cliImageLoaded = false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
			cliFileName = thumbView->currentViewDir + QDir::separator() + cliArg.fileName();
			cliImageLoaded = true;
			loadImagefromCli();
		}
	}
	else
	{
		if (GData::startupDir == GData::specifiedDir)
			thumbView->currentViewDir = GData::specifiedStartDir;
		else if (GData::startupDir == GData::rememberLastDir)
			thumbView->currentViewDir = GData::appSettings->value("lastDir").toString();
	}
	selectCurrentViewDir();
}

void Phototonic::unsetBusy()
{	
	thumbViewBusy = false;
}

bool Phototonic::event(QEvent *event)
{
	if (event->type() == QEvent::ActivationChange && stackedWidget->currentIndex() == thumbViewIdx)
	{ 
		thumbView->loadVisibleThumbs();
	}
	
	return QMainWindow::event(event);
}

void Phototonic::createThumbView()
{
	thumbView = new ThumbView(this);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();
	thumbView->thumbsSortFlags |= QDir::IgnoreCase;

	connect(thumbView, SIGNAL(unsetBusy()), this, SLOT(unsetBusy()));
	connect(thumbView, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));

	iiDock = new QDockWidget(tr("Image Info"), this);
	iiDock->setObjectName("Image Info");
	iiDock->setWidget(thumbView->infoView);
	connect(iiDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setIiDockVisibility()));	
	connect(iiDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setIiDockVisibility()));	
}

void Phototonic::addMenuSeparator(QWidget *widget)
{
	QAction *separator = new QAction(this);
	separator->setSeparator(true);
	widget->addAction(separator);
}

void Phototonic::createImageView()
{
	imageView = new ImageView(this);
	connect(saveAction, SIGNAL(triggered()), imageView, SLOT(saveImage()));
	connect(saveAsAction, SIGNAL(triggered()), imageView, SLOT(saveImageAs()));
	connect(copyImageAction, SIGNAL(triggered()), imageView, SLOT(copyImage()));
	connect(pasteImageAction, SIGNAL(triggered()), imageView, SLOT(pasteImage()));
	imageView->ImagePopUpMenu = new QMenu();

	// Widget actions
	imageView->addAction(slideShowAction);
	imageView->addAction(nextImageAction);
	imageView->addAction(prevImageAction);
	imageView->addAction(firstImageAction);
	imageView->addAction(lastImageAction);
	imageView->addAction(randomImageAction);
	imageView->addAction(zoomInAct);
	imageView->addAction(zoomOutAct);
	imageView->addAction(origZoomAct);
	imageView->addAction(resetZoomAct);
	imageView->addAction(rotateRightAct);
	imageView->addAction(rotateLeftAct);
	imageView->addAction(freeRotateRightAct);
	imageView->addAction(freeRotateLeftAct);
	imageView->addAction(flipHAct);
	imageView->addAction(flipVAct);
	imageView->addAction(cropAct);
	imageView->addAction(resizeAct);
	imageView->addAction(saveAction);
	imageView->addAction(saveAsAction);
	imageView->addAction(copyImageAction);
	imageView->addAction(pasteImageAction);
	imageView->addAction(deleteAction);
	imageView->addAction(closeImageAct);
	imageView->addAction(fullScreenAct);
	imageView->addAction(settingsAction);
	imageView->addAction(exitAction);
	imageView->addAction(mirrorDisabledAct);
	imageView->addAction(mirrorDualAct);
	imageView->addAction(mirrorTripleAct);
	imageView->addAction(mirrorVDualAct);
	imageView->addAction(mirrorQuadAct);
	imageView->addAction(keepTransformAct);
	imageView->addAction(keepZoomAct);
	imageView->addAction(refreshAction);
	imageView->addAction(colorsAct);
	imageView->addAction(moveRightAct);
	imageView->addAction(moveLeftAct);
	imageView->addAction(moveUpAct);
	imageView->addAction(moveDownAct);
	imageView->addAction(showClipboardAction);
	imageView->addAction(copyMoveAction);
	imageView->addAction(resizeAct);

	// Actions
	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(nextImageAction);
	imageView->ImagePopUpMenu->addAction(prevImageAction);
	imageView->ImagePopUpMenu->addAction(firstImageAction);
	imageView->ImagePopUpMenu->addAction(lastImageAction);
	imageView->ImagePopUpMenu->addAction(randomImageAction);
	imageView->ImagePopUpMenu->addAction(slideShowAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	zoomSubMenu = new QMenu(tr("Zoom"));
	zoomSubMenuAct = new QAction(tr("Zoom"), this);
	zoomSubMenuAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
	zoomSubMenuAct->setMenu(zoomSubMenu);
	imageView->ImagePopUpMenu->addAction(zoomSubMenuAct);
	zoomSubMenu->addAction(zoomInAct);
	zoomSubMenu->addAction(zoomOutAct);
	zoomSubMenu->addAction(origZoomAct);
	zoomSubMenu->addAction(resetZoomAct);
	addMenuSeparator(zoomSubMenu);
	zoomSubMenu->addAction(keepZoomAct);

	transformSubMenu = new QMenu(tr("Transform"));
	transformSubMenuAct = new QAction(tr("Transform"), this);
	transformSubMenuAct->setMenu(transformSubMenu);
	imageView->ImagePopUpMenu->addAction(transformSubMenuAct);
	transformSubMenu->addAction(rotateRightAct);
	transformSubMenu->addAction(rotateLeftAct);
	transformSubMenu->addAction(freeRotateRightAct);
	transformSubMenu->addAction(freeRotateLeftAct);
	transformSubMenu->addAction(flipHAct);
	transformSubMenu->addAction(flipVAct);
	transformSubMenu->addAction(cropAct);

	MirroringSubMenu = new QMenu(tr("Mirroring"));
	mirrorSubMenuAct = new QAction(tr("Mirroring"), this);
	mirrorSubMenuAct->setMenu(MirroringSubMenu);
	imageView->ImagePopUpMenu->addAction(mirrorSubMenuAct);
	mirroringGroup = new QActionGroup(this);
	mirroringGroup->addAction(mirrorDisabledAct);
	mirroringGroup->addAction(mirrorDualAct);
	mirroringGroup->addAction(mirrorTripleAct);
	mirroringGroup->addAction(mirrorVDualAct);
	mirroringGroup->addAction(mirrorQuadAct);
	MirroringSubMenu->addActions(mirroringGroup->actions());
	addMenuSeparator(transformSubMenu);
	transformSubMenu->addAction(keepTransformAct);
	imageView->ImagePopUpMenu->addAction(colorsAct);
	imageView->ImagePopUpMenu->addAction(resizeAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(copyMoveAction);
	imageView->ImagePopUpMenu->addAction(saveAction);
	imageView->ImagePopUpMenu->addAction(saveAsAction);
	imageView->ImagePopUpMenu->addAction(deleteAction);
	imageView->ImagePopUpMenu->addAction(openWithMenuAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(copyImageAction);
	imageView->ImagePopUpMenu->addAction(pasteImageAction);
	imageView->ImagePopUpMenu->addAction(fullScreenAct);
	imageView->ImagePopUpMenu->addAction(refreshAction);
	imageView->ImagePopUpMenu->addAction(showClipboardAction);
	imageView->ImagePopUpMenu->addAction(closeImageAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(settingsAction);
	imageView->ImagePopUpMenu->addAction(exitAction);

	imageView->setContextMenuPolicy(Qt::DefaultContextMenu);
	GData::isFullScreen = GData::appSettings->value("isFullScreen").toBool();
	fullScreenAct->setChecked(GData::isFullScreen); 
}

void Phototonic::createActions()
{
	thumbsGoTopAct = new QAction(tr("Top"), this);
	thumbsGoTopAct->setIcon(QIcon::fromTheme("go-top", QIcon(":/images/top.png")));
	connect(thumbsGoTopAct, SIGNAL(triggered()), this, SLOT(goTop()));

	thumbsGoBottomAct = new QAction(tr("Bottom"), this);
	thumbsGoBottomAct->setIcon(QIcon::fromTheme("go-bottom", QIcon(":/images/bottom.png")));
	connect(thumbsGoBottomAct, SIGNAL(triggered()), this, SLOT(goBottom()));

	closeImageAct = new QAction(tr("Close Image"), this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(closeImage()));

	fullScreenAct = new QAction(tr("Full Screen"), this);
	fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	
	settingsAction = new QAction(tr("Preferences"), this);
	settingsAction->setIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/settings.png")));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction(tr("Exit"), this);
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction(tr("Enlarge Thumbnails"), this);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MAX)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction(tr("Shrink Thumbnails"), this);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MIN)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction(tr("Cut"), this);
	cutAction->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png")));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutThumbs()));
	cutAction->setEnabled(false);

	copyAction = new QAction(tr("Copy"), this);
	copyAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png")));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyThumbs()));
	copyAction->setEnabled(false);

	copyMoveAction = new QAction(tr("Copy or Move to..."), this);
	connect(copyMoveAction, SIGNAL(triggered()), this, SLOT(copyMoveImages()));
	
	deleteAction = new QAction(tr("Delete"), this);
	deleteAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	saveAction = new QAction(tr("Save"), this);
	saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

	saveAsAction = new QAction(tr("Save As"), this);
	copyImageAction = new QAction(tr("Copy Image"), this);
	pasteImageAction = new QAction(tr("Paste Image"), this);

	renameAction = new QAction(tr("Rename"), this);
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction(tr("Select All"), this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction(tr("About"), this);
	aboutAction->setIcon(QIcon::fromTheme("help-about", QIcon(":/images/about.png")));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	// Sort actions
	actName = new QAction(tr("Name"), this);
	actTime = new QAction(tr("Time"), this);
	actSize = new QAction(tr("Size"), this);
	actType = new QAction(tr("Type"), this);
	actReverse = new QAction(tr("Reverse"), this);
	actName->setCheckable(true);
	actTime->setCheckable(true);
	actSize->setCheckable(true);
	actType->setCheckable(true);
	actReverse->setCheckable(true);
	connect(actName, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actTime, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actSize, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actType, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actReverse, SIGNAL(triggered()), this, SLOT(sortThumbnains()));

	if (thumbView->thumbsSortFlags & QDir::Time)
		actTime->setChecked(true);	
	else if (thumbView->thumbsSortFlags & QDir::Size)
		actSize->setChecked(true);	
	else if (thumbView->thumbsSortFlags & QDir::Type)
		actType->setChecked(true);	
	else
		actName->setChecked(true);	
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	actShowHidden = new QAction(tr("Show Hidden Files"), this);
	actShowHidden->setCheckable(true);
	actShowHidden->setChecked(GData::showHiddenFiles);
	connect(actShowHidden, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

	actShowLabels = new QAction(tr("Show Labels"), this);
	actShowLabels->setCheckable(true);
	actShowLabels->setChecked(GData::showLabels);
	connect(actShowLabels, SIGNAL(triggered()), this, SLOT(showLabels()));

	actSmallIcons = new QAction(tr("Small Icons"), this);
	actSmallIcons->setCheckable(true);
	actSmallIcons->setChecked(GData::smallIcons);
	connect(actSmallIcons, SIGNAL(triggered()), this, SLOT(setToolbarIconSize()));

	actLockDocks = new QAction(tr("Hide Docks Title Bar"), this);
	actLockDocks->setCheckable(true);
	actLockDocks->setChecked(GData::LockDocks);
	connect(actLockDocks, SIGNAL(triggered()), this, SLOT(lockDocks()));

	actClassic = new QAction(tr("Classic Thumbs"), this);
	actCompact = new QAction(tr("Compact"), this);
	actSquarish = new QAction(tr("Squarish"), this);
	connect(actClassic, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));
	connect(actCompact, SIGNAL(triggered()), this, SLOT(setCompactThumbs()));
	connect(actSquarish, SIGNAL(triggered()), this, SLOT(setSquarishThumbs()));
	actClassic->setCheckable(true);
	actCompact->setCheckable(true);
	actSquarish->setCheckable(true);
	actClassic->setChecked(GData::thumbsLayout == ThumbView::Classic); 
	actCompact->setChecked(GData::thumbsLayout == ThumbView::Compact); 
	actSquarish->setChecked(GData::thumbsLayout == ThumbView::Squares); 

	refreshAction = new QAction(tr("Reload"), this);
	refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

	subFoldersAction = new QAction(tr("Include Sub-folders"), this);
	subFoldersAction->setIcon(QIcon(":/images/tree.png"));
	subFoldersAction->setCheckable(true);
	connect(subFoldersAction, SIGNAL(triggered()), this, SLOT(setIncludeSubFolders()));

	pasteAction = new QAction(tr("Paste Here"), this);
	pasteAction->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteThumbs()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction(tr("New Folder"), this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));
	
	manageDirAction = new QAction(tr("Manage"), this);
	connect(manageDirAction, SIGNAL(triggered()), this, SLOT(manageDir()));

	goBackAction = new QAction(tr("Back"), this);
	goBackAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);

	goFrwdAction = new QAction(tr("Forward"), this);
	goFrwdAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);

	goUpAction = new QAction(tr("Up"), this);
	goUpAction->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/up.png")));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction(tr("Home"), this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon::fromTheme("go-home", QIcon(":/images/home.png")));

	slideShowAction = new QAction(tr("Slide Show"), this);
	connect(slideShowAction, SIGNAL(triggered()), this, SLOT(slideShow()));
	slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));

	nextImageAction = new QAction(tr("Next"), this);
	nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));
	
	prevImageAction = new QAction(tr("Previous"), this);
	prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

	firstImageAction = new QAction(tr("First"), this);
	firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
	connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

	lastImageAction = new QAction(tr("Last"), this);
	lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
	connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

	randomImageAction = new QAction(tr("Random"), this);
	connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

	openAction = new QAction(tr("Open"), this);
	openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	connect(openAction, SIGNAL(triggered()), this, SLOT(openOp()));

	showClipboardAction = new QAction(tr("Show Clipboard"), this);
	showClipboardAction->setIcon(QIcon::fromTheme("window-new", QIcon(":/images/new.png")));
	connect(showClipboardAction, SIGNAL(triggered()), this, SLOT(newImage()));

	openWithSubMenu = new QMenu(tr("Open With"));
	openWithMenuAct = new QAction(tr("Open With"), this);
	openWithMenuAct->setMenu(openWithSubMenu);
	chooseAppAct = new QAction(tr("Manage External Applications"), this);
	connect(chooseAppAct, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));

	zoomOutAct = new QAction(tr("Zoom Out"), this);
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
	zoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

	zoomInAct = new QAction(tr("Zoom In"), this);
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
	zoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

	resetZoomAct = new QAction(tr("Reset Zoom"), this);
	resetZoomAct->setIcon(QIcon::fromTheme("zoom-fit-best"));
	connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

	origZoomAct = new QAction(tr("Original Size"), this);
	origZoomAct->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
	connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

	keepZoomAct = new QAction(tr("Keep Zoom"), this);
	keepZoomAct->setCheckable(true);
	connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

	rotateLeftAct = new QAction(tr("Rotate 90 degree CCW"), this);
	rotateLeftAct->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
	connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

	rotateRightAct = new QAction(tr("Rotate 90 degree CW"), this);
	rotateRightAct->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
	connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

	flipHAct = new QAction(tr("Flip Horizontally"), this);
	flipHAct->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
	connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));

	flipVAct = new QAction(tr("Flip Vertically"), this);
	flipVAct->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
	connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));

	cropAct = new QAction(tr("Cropping"), this);
	connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));

	resizeAct = new QAction(tr("Scale Image"), this);
	resizeAct->setIcon(QIcon::fromTheme("transform-scale"));
	connect(resizeAct, SIGNAL(triggered()), this, SLOT(scaleImage()));

	freeRotateLeftAct = new QAction(tr("Rotate 1 degree CCW"), this);
	connect(freeRotateLeftAct, SIGNAL(triggered()), this, SLOT(freeRotateLeft()));

	freeRotateRightAct = new QAction(tr("Rotate 1 degree CW"), this);
	connect(freeRotateRightAct, SIGNAL(triggered()), this, SLOT(freeRotateRight()));

	colorsAct = new QAction(tr("Colors"), this);
	connect(colorsAct, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
	colorsAct->setIcon(QIcon(":/images/colors.png"));

	mirrorDisabledAct = new QAction(tr("Disable"), this);
	mirrorDualAct = new QAction(tr("Dual"), this);
	mirrorTripleAct = new QAction(tr("Triple"), this);
	mirrorVDualAct = new QAction(tr("Dual Vertical"), this);
	mirrorQuadAct = new QAction(tr("Quad"), this);

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

	keepTransformAct = new QAction(tr("Keep Transformations"), this);
	keepTransformAct->setCheckable(true);
	connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

	moveLeftAct = new QAction(tr("Move Left"), this);
	connect(moveLeftAct, SIGNAL(triggered()), this, SLOT(moveLeft()));
	moveRightAct = new QAction(tr("Move Right"), this);
	connect(moveRightAct, SIGNAL(triggered()), this, SLOT(moveRight()));
	moveUpAct = new QAction(tr("Move Up"), this);
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveUp()));
	moveDownAct = new QAction(tr("Move Down"), this);
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveDown()));

	invertSelectionAct = new QAction(tr("Invert Selection"), this);
	connect(invertSelectionAct, SIGNAL(triggered()), thumbView, SLOT(invertSelection()));
}

void Phototonic::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(subFoldersAction);
	fileMenu->addAction(createDirAction);
	fileMenu->addAction(showClipboardAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(copyMoveAction);
	editMenu->addAction(pasteAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
	editMenu->addSeparator();
	editMenu->addAction(selectAllAction);
	editMenu->addAction(invertSelectionAct);
	editMenu->addSeparator();
	editMenu->addAction(settingsAction);

	goMenu = menuBar()->addMenu(tr("&Go"));
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);
	goMenu->addSeparator();
	goMenu->addAction(thumbsGoTopAct);
	goMenu->addAction(thumbsGoBottomAct);

	viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(slideShowAction);
	viewMenu->addSeparator();
	
	viewMenu->addAction(thumbsZoomInAct);
	viewMenu->addAction(thumbsZoomOutAct);
	sortMenu = viewMenu->addMenu(tr("Sort By"));
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
	sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
	viewMenu->addSeparator();

	thumbLayoutsGroup = new QActionGroup(this);
	thumbLayoutsGroup->addAction(actClassic);
	thumbLayoutsGroup->addAction(actCompact);
	thumbLayoutsGroup->addAction(actSquarish);
	viewMenu->addActions(thumbLayoutsGroup->actions());
	viewMenu->addSeparator();

	viewMenu->addAction(actShowLabels);
	viewMenu->addAction(actShowHidden);
	viewMenu->addSeparator();
	viewMenu->addAction(refreshAction);

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAction);

	// thumbview context menu
	thumbView->addAction(openAction);
	thumbView->addAction(openWithMenuAct);
	thumbView->addAction(cutAction);
	thumbView->addAction(copyAction);
	thumbView->addAction(pasteAction);
	thumbView->addAction(copyMoveAction);
	thumbView->addAction(renameAction);
	thumbView->addAction(deleteAction);
	addMenuSeparator(thumbView);
	thumbView->addAction(selectAllAction);
	thumbView->addAction(invertSelectionAct);
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
	menuBar()->setVisible(true);
}

void Phototonic::createToolBars()
{
	/* Edit */
	editToolBar = addToolBar(tr("Edit"));
	editToolBar->setObjectName("Edit");
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	editToolBar->addAction(showClipboardAction);
	connect(editToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setEditToolBarVisibility()));

	/* Navigation */
	goToolBar = addToolBar(tr("Navigation"));
	goToolBar->setObjectName("Navigation");
	goToolBar->addAction(goBackAction);
	goToolBar->addAction(goFrwdAction);
	goToolBar->addAction(goUpAction);
	goToolBar->addAction(goHomeAction);

	/* path bar */
	pathBar = new QLineEdit;
	pathComplete = new QCompleter(this);
	QDirModel *pathCompleteDirMod = new QDirModel;
	pathCompleteDirMod->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	pathComplete->setModel(pathCompleteDirMod);
	pathBar->setCompleter(pathComplete);
	pathBar->setMinimumWidth(200);
	pathBar->setMaximumWidth(300);
	connect(pathBar, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
	goToolBar->addWidget(pathBar);
	goToolBar->addAction(refreshAction);
	goToolBar->addAction(subFoldersAction);
	connect(goToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setGoToolBarVisibility()));

	/* View */
	viewToolBar = addToolBar(tr("View"));
	viewToolBar->setObjectName("View");
	viewToolBar->addAction(slideShowAction);
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);

	/* filter bar */ 
	QAction *filterAct = new QAction(tr("Filter"), this);
	filterAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
	connect(filterAct, SIGNAL(triggered()), this, SLOT(setThumbsFilter()));
	filterBar = new QLineEdit;
	filterBar->setMinimumWidth(100);
	filterBar->setMaximumWidth(200);
	connect(filterBar, SIGNAL(returnPressed()), this, SLOT(setThumbsFilter()));
	connect(filterBar, SIGNAL(textChanged(const QString&)), this, SLOT(clearThumbsFilter()));
	filterBar->setClearButtonEnabled(true);
	filterBar->addAction(filterAct, QLineEdit::LeadingPosition);

	viewToolBar->addSeparator();
	viewToolBar->addWidget(filterBar);
	viewToolBar->addAction(settingsAction);
	connect(viewToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setViewToolBarVisibility()));	

	setToolbarIconSize();
}

void Phototonic::setToolbarIconSize()
{
	int iconSize;
	if (initComplete)
		GData::smallIcons = actSmallIcons->isChecked();
	iconSize = GData::smallIcons? 16 : 24;
	QSize iconQSize(iconSize, iconSize);

	editToolBar->setIconSize(iconQSize);
	goToolBar->setIconSize(iconQSize);
	viewToolBar->setIconSize(iconQSize);
}

void Phototonic::createStatusBar()
{
	stateLabel = new QLabel(tr("Initializing..."));
	statusBar()->addWidget(stateLabel);
}

void Phototonic::setfsModelFlags()
{
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	if (GData::showHiddenFiles)
		fsModel->setFilter(fsModel->filter() | QDir::Hidden);
}

void Phototonic::createFSTree()
{
	fsDock = new QDockWidget(tr("File System"), this);
	fsDock->setObjectName("File System");

	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	setfsModelFlags();

	fsTree = new FSTree(fsDock);
	fsDock->setWidget(fsTree);
	connect(fsDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setFsDockVisibility()));	
	connect(fsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setFsDockVisibility()));	
	addDockWidget(Qt::LeftDockWidgetArea, fsDock);

	// Context menu
	fsTree->addAction(openAction);
	fsTree->addAction(createDirAction);
	fsTree->addAction(renameAction);
	fsTree->addAction(deleteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(pasteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(manageDirAction);
	fsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

	fsTree->setModel(fsModel);
	for (int i = 1; i <= 3; ++i)
		fsTree->hideColumn(i);
	fsTree->setHeaderHidden(true);
	connect(fsTree, SIGNAL(clicked(const QModelIndex&)),
				this, SLOT(goSelectedDir(const QModelIndex &)));

	connect(fsModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
				this, SLOT(checkDirState(const QModelIndex &, int, int)));

	connect(fsTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

	fsTree->setCurrentIndex(fsModel->index(QDir::currentPath()));
}

void Phototonic::sortThumbnains()
{
	thumbView->thumbsSortFlags = QDir::IgnoreCase;

	if (actName->isChecked())
		thumbView->thumbsSortFlags |= QDir::Name;
	else if (actTime->isChecked())
		thumbView->thumbsSortFlags |= QDir::Time;
	else if (actSize->isChecked())
		thumbView->thumbsSortFlags |= QDir::Size;
	else if (actType->isChecked())
		thumbView->thumbsSortFlags |= QDir::Type;

	if (actReverse->isChecked())
		thumbView->thumbsSortFlags |= QDir::Reversed;

	refreshThumbs(false);
}

void Phototonic::reload()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		imageView->reload();
	}
	else
		refreshThumbs(false);
}

void Phototonic::setIncludeSubFolders()
{
	GData::includeSubFolders = subFoldersAction->isChecked();
	refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop)
{
	thumbView->setNeedScroll(scrollToTop);
	QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	if (scrollToTop)
		QTimer::singleShot(100, this, SLOT(scrollToLastImage()));
	else
		QTimer::singleShot(100, this, SLOT(selectRecentThumb()));
}

void Phototonic::setClassicThumbs()
{
	GData::thumbsLayout = ThumbView::Classic;
	refreshThumbs(false);
}

void Phototonic::setCompactThumbs()
{
	GData::thumbsLayout = ThumbView::Compact;
	refreshThumbs(false);
}

void Phototonic::setSquarishThumbs()
{
	GData::thumbsLayout = ThumbView::Squares;
	refreshThumbs(false);
}

void Phototonic::showHiddenFiles()
{
	GData::showHiddenFiles = actShowHidden->isChecked();
	setfsModelFlags();
	refreshThumbs(false);
}

void Phototonic::showLabels()
{
	GData::showLabels = actShowLabels->isChecked();
	refreshThumbs(false);
}

void Phototonic::about()
{
	QString aboutString = "<h2>Phototonic v1.03</h2>"
		+ tr("<p>Image viewer and organizer</p>")
		+ tr("<p>Git release") + " v1.03.10 (built " __DATE__ " " __TIME__ ")</p>"
		+ tr("Built with Qt ") + QT_VERSION_STR
		+ "<p><a href=\"http://oferkv.github.io/phototonic/\">" + tr("Home page") + "</a></p>"
		+ "<p><a href=\"https://github.com/oferkv/phototonic/issues\">" + tr("Bug reports") + "</a></p>"
		+ "<p>Copyright &copy; 2013-2014 Ofer Kashayov (oferkv@live.com)</p>"
		+ tr("Contributors / Code:") + "<br>"
		+ "Christopher Roy Bratusek (nano@jpberlin.de)<br><br>"
		+ tr("Contributors / Translations:")
		+ "<table><tr><td>Czech:</td><td>Pavel Fric (pavelfric@seznam.cz)</td></tr>"
		+ "<tr><td>French:</td><td>David Geiger (david.david@mageialinux-online.org)</td></tr>"
		+ "<tr><td></td><td>Adrien Daugabel (adrien.d@mageialinux-online.org)</td></tr>"
		+ "<tr><td>German:</td><td>Jonathan Hooverman (jonathan.hooverman@gmail.com)</td></tr>"
		+ QString::fromUtf8("<tr><td>Polish:</td><td>Robert Wojew\u00F3dzki (robwoj44@poczta.onet.pl)</td></tr>")
		+ "<tr><td>Russian:</td><td>Ilya Alexandrovich (yast4ik@gmail.com)</td></tr></table>"
		+ "<p>Phototonic is licensed under the GNU General Public License version 3</p>";

	QMessageBox::about(this, tr("About") + " Phototonic", aboutString);
}

void Phototonic::cleanupSender()
{
	delete QObject::sender();
}

void Phototonic::runExternalApp()
{
	QString execCommand;
	QString selectedFileNames("");
	execCommand = GData::externalApps[((QAction*) sender())->text()];

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (imageView->isNewImage())
		{
			showNewImageWarning(this);
			return;
		}

		execCommand += " \"" + imageView->currentImageFullPath + "\"";
	}
	else
	{
		QModelIndexList selectedIdxList = thumbView->selectionModel()->selectedIndexes();
		if (selectedIdxList.size() < 1)
		{
			setStatus(tr("Invalid selection"));
			return;
		}

		selectedFileNames += " ";
		for (int tn = selectedIdxList.size() - 1; tn >= 0 ; --tn)
		{
			selectedFileNames += "\"" +
				thumbView->thumbViewModel->item(selectedIdxList[tn].row())->data(thumbView->FileNameRole).toString();
			if (tn) 
				selectedFileNames += "\" ";
		}
		
		execCommand += selectedFileNames;
	}

	QProcess *externalProcess = new QProcess();
	connect(externalProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
						this, SLOT(cleanupSender()));
	externalProcess->start(execCommand);
}

void Phototonic::updateExternalApps()
{
	int actionNum = 0;
	QMapIterator<QString, QString> eaIter(GData::externalApps);

    QList<QAction*> actionList = openWithSubMenu->actions();
    if (!actionList.empty()) {

    	for (int i = 0; i < actionList.size(); ++i)
    	{
      		QAction *action = actionList.at(i);
      		if (action->isSeparator())
      			break;
			openWithSubMenu->removeAction(action);
			imageView->removeAction(action);
			delete action;
   		}

      	openWithSubMenu->clear();
    }

	while (eaIter.hasNext())
	{
		++actionNum;
		eaIter.next();
		QAction *extAppAct = new QAction(eaIter.key(), this);
		if (actionNum < 10)
			extAppAct->setShortcut(QKeySequence("Alt+" + QString::number(actionNum)));
		extAppAct->setIcon(QIcon::fromTheme(eaIter.key()));
		connect(extAppAct, SIGNAL(triggered()), this, SLOT(runExternalApp()));
		openWithSubMenu->addAction(extAppAct);
		imageView->addAction(extAppAct);
	}

	openWithSubMenu->addSeparator();
	openWithSubMenu->addAction(chooseAppAct);
}

void Phototonic::chooseExternalApp()
{
	AppMgmtDialog *dialog = new AppMgmtDialog(this);
	dialog->exec();
	updateExternalApps();
	delete(dialog);
}

void Phototonic::showSettings()
{
	if (GData::slideShowActive)
		slideShow();
	
	SettingsDialog *dialog = new SettingsDialog(this);
	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->imagePreview->setPalette(QPalette(GData::thumbsBackgroundColor));
		thumbView->setThumbColors();
		GData::imageZoomFactor = 1.0;

		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			imageView->reload();
			needThumbsRefresh = true;
		}
		else
			refreshThumbs(false);
	}

	delete dialog;
}

void Phototonic::toggleFullScreen()
{
	if (fullScreenAct->isChecked())
	{
		shouldMaximize = isMaximized();
		showFullScreen();
		GData::isFullScreen = true;
		imageView->setCursorHiding(true);
	}
	else
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
		GData::isFullScreen = false;
	}
}

void Phototonic::selectAllThumbs()
{
	thumbView->selectAll();
}

void Phototonic::copyOrCutThumbs(bool copy)
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();

	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; ++tn)
	{
		GData::copyCutFileList.append(thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].
										row())->data(thumbView->FileNameRole).toString());
	}

	GData::copyOp = copy;
	pasteAction->setEnabled(true);
}

void Phototonic::cutThumbs()
{
	copyOrCutThumbs(false);
}

void Phototonic::copyThumbs()
{
	copyOrCutThumbs(true);
}

void Phototonic::copyMoveImages()
{
	copyMoveToDialog = new CopyMoveToDialog(this, getSelectedPath());
	if (copyMoveToDialog->exec())
	{
		if (stackedWidget->currentIndex() == thumbViewIdx)
		{
			if (copyMoveToDialog->copyOp)
				copyThumbs();
			else
				cutThumbs();

			pasteThumbs();
		}
		else
		{
			if (imageView->isNewImage())
			{
				showNewImageWarning(this);
				return;
			}
		
			QFileInfo fileInfo = QFileInfo(imageView->currentImageFullPath);
			QString fileName = fileInfo.fileName();
			QString destFile = copyMoveToDialog->selectedPath + QDir::separator() + fileInfo.fileName();
			
			int res = cpMvFile(copyMoveToDialog->copyOp, fileName, imageView->currentImageFullPath,
				 									destFile, copyMoveToDialog->selectedPath);

			if (!res)
			{
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to copy or move image"));
			}
			else
			{
				if (!copyMoveToDialog->copyOp)
				{
					int currentRow = thumbView->getCurrentRow();
					thumbView->thumbViewModel->removeRow(currentRow);
					updateCurrentImage(currentRow);
				}
			}
		}
	}
	
	delete(copyMoveToDialog);
	copyMoveToDialog = 0;
}

void Phototonic::thumbsZoomIn()
{
	if (thumbView->thumbSize < THUMB_SIZE_MAX)
	{
		thumbView->thumbSize += 50;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MAX)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::thumbsZoomOut()
{
	if (thumbView->thumbSize > THUMB_SIZE_MIN)
	{
		thumbView->thumbSize -= 50;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MIN)
			thumbsZoomOutAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::zoomOut()
{
	GData::imageZoomFactor -= (GData::imageZoomFactor <= 0.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::zoomIn()
{
	GData::imageZoomFactor += (GData::imageZoomFactor >= 3.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::resetZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::origZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = true;
	imageView->resizeImage();
}

void Phototonic::keepZoom()
{
	GData::keepZoomFactor = keepZoomAct->isChecked();
}

void Phototonic::keepTransformClicked()
{
	GData::keepTransform = keepTransformAct->isChecked();
}

void Phototonic::rotateLeft()
{
	GData::rotation -= 90;
	if (GData::rotation < 0)
		GData::rotation = 270;
	imageView->refresh();
}

void Phototonic::rotateRight()
{
	GData::rotation += 90;
	if (GData::rotation > 270)
		GData::rotation = 0;
	imageView->refresh();
}

void Phototonic::flipVert()
{
	GData::flipV = !GData::flipV;
	imageView->refresh();
}

void Phototonic::cropImage()
{
	if (GData::slideShowActive)
		slideShow();

	CropDialog *cropDialog = new CropDialog(this, imageView);
	connect(cropDialog, SIGNAL(accepted()), this, SLOT(enableImageView()));
	connect(cropDialog, SIGNAL(rejected()), this, SLOT(enableImageView()));
	connect(cropDialog, SIGNAL(accepted()), this, SLOT(cleanupSender()));
	connect(cropDialog, SIGNAL(rejected()), this, SLOT(cleanupSender()));

	cropDialog->show();
	disableImageView();
}

void Phototonic::scaleImage()
{
	if (GData::slideShowActive)
		slideShow();

	ResizeDialog *resizeDialog = new ResizeDialog(this, imageView);
	connect(resizeDialog, SIGNAL(accepted()), this, SLOT(enableImageView()));
	connect(resizeDialog, SIGNAL(rejected()), this, SLOT(enableImageView()));
	connect(resizeDialog, SIGNAL(accepted()), this, SLOT(cleanupSender()));
	connect(resizeDialog, SIGNAL(rejected()), this, SLOT(cleanupSender()));

	resizeDialog->show();
	disableImageView();
}

void Phototonic::freeRotateLeft()
{
	--GData::rotation;
	if (GData::rotation < 0)
		GData::rotation = 359;
	imageView->refresh();
}

void Phototonic::freeRotateRight()
{
	++GData::rotation;
	if (GData::rotation > 360)
		GData::rotation = 1;
	imageView->refresh();
}

void Phototonic::showColorsDialog()
{
	if (GData::slideShowActive)
		slideShow();

	ColorsDialog *colorsDialog = new ColorsDialog(this, imageView);
	connect(colorsDialog, SIGNAL(accepted()), this, SLOT(enableImageView()));
	connect(colorsDialog, SIGNAL(rejected()), this, SLOT(enableImageView()));
	connect(colorsDialog, SIGNAL(accepted()), this, SLOT(cleanupSender()));
	connect(colorsDialog, SIGNAL(rejected()), this, SLOT(cleanupSender()));

	colorsDialog->show();
	disableImageView();
}

void Phototonic::flipHoriz()
{
	GData::flipH = !GData::flipH;
	imageView->refresh();
}

void Phototonic::moveRight()
{
	imageView->keyMoveEvent(ImageView::MoveRight);
}

void Phototonic::moveLeft()
{
	imageView->keyMoveEvent(ImageView::MoveLeft);
}

void Phototonic::moveUp()
{
	imageView->keyMoveEvent(ImageView::MoveUp);
}

void Phototonic::moveDown()
{
	imageView->keyMoveEvent(ImageView::MoveDown);
}

void Phototonic::setMirrorDisabled()
{
	imageView->mirrorLayout = ImageView::LayNone;
	imageView->refresh();
}

void Phototonic::setMirrorDual()
{
	imageView->mirrorLayout = ImageView::LayDual;
	imageView->refresh();
}

void Phototonic::setMirrorTriple()
{
	imageView->mirrorLayout = ImageView::LayTriple;
	imageView->refresh();
}

void Phototonic::setMirrorVDual()
{
	imageView->mirrorLayout = ImageView::LayVDual;
	imageView->refresh();
}

void Phototonic::setMirrorQuad()
{
	imageView->mirrorLayout = ImageView::LayQuad;
	imageView->refresh();
}

bool Phototonic::isValidPath(QString &path)
{
	QDir checkPath(path);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		return false;
	}
	return true;
}

void Phototonic::pasteThumbs()
{
	if (!copyCutCount)
		return;

	QString destDir;
	if (copyMoveToDialog)
		destDir = copyMoveToDialog->selectedPath;
	else
		destDir = getSelectedPath();

	if (!isValidPath(destDir))
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Can not copy or move to ") + destDir);
		selectCurrentViewDir();
		return;
	}

	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);

	QFileInfo fileInfo;
	if (!GData::copyOp && pasteInCurrDir)
	{
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			fileInfo = QFileInfo(GData::copyCutFileList[tn]);
			if (fileInfo.absolutePath() == destDir)
			{
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Can not cut and paste in the same folder"));
				return;
			}
		}
	}

	CpMvDialog *dialog = new CpMvDialog(this);
	dialog->exec(thumbView, destDir, pasteInCurrDir);
	if (pasteInCurrDir)
	{
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			thumbView->addThumb(GData::copyCutFileList[tn]);
		}
	}
	
	QString state = QString((GData::copyOp? tr("Copied ") : tr("Moved ")) + 
								QString::number(dialog->nfiles) + tr(" images"));
	setStatus(state);
	delete(dialog);
	selectCurrentViewDir();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	thumbView->loadVisibleThumbs();
}

void Phototonic::updateCurrentImage(int currentRow)
{
	bool wrapImageListTmp = GData::wrapImageList;
	GData::wrapImageList = false;

	if (currentRow == thumbView->thumbViewModel->rowCount())
	{
		thumbView->setCurrentRow(currentRow - 1);
	}

	if (thumbView->getNextRow() < 0 && currentRow > 0)
	{
		loadImageFile(thumbView->thumbViewModel->item(currentRow - 1)->data(thumbView->FileNameRole).toString());
	}
	else
	{
		if (thumbView->thumbViewModel->rowCount() == 0)
		{
			closeImage();
			refreshThumbs(true);
			return;
		}
		loadImageFile(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());
	}
		
	GData::wrapImageList = wrapImageListTmp;
	thumbView->setImageviewWindowTitle();
}

void Phototonic::deleteViewerImage()
{
	if (imageView->isNewImage())
	{
		showNewImageWarning(this);
		return;
	}

	bool ok;
	QFileInfo fileInfo = QFileInfo(imageView->currentImageFullPath);
	QString fileName = fileInfo.fileName();

	QMessageBox msgBox;
	msgBox.setText(tr("Permanently delete ") + fileName);
	msgBox.setWindowTitle(tr("Delete image"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		int currentRow = thumbView->getCurrentRow();

		ok = QFile::remove(imageView->currentImageFullPath);
		if (ok)
		{
			 thumbView->thumbViewModel->removeRow(currentRow);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.critical(this, tr("Error"), tr("Failed to delete image"));
			return;
		}

		updateCurrentImage(currentRow);
	}
}

void Phototonic::deleteOp()
{
	if (QApplication::focusWidget() == fsTree)
	{
		deleteDir();
		return;
	}

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		deleteViewerImage();
		return;
	}

	if (thumbView->selectionModel()->selectedIndexes().size() < 1)
	{
		setStatus(tr("No selection"));
		return;
	}

	QMessageBox msgBox;
	msgBox.setText(tr("Permanently delete selected images?"));
	msgBox.setWindowTitle(tr("Delete images"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		QModelIndexList indexesList;
		int nfiles = 0;
		bool ok;
	
		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->thumbViewModel->item(
								indexesList.first().row())->data(thumbView->FileNameRole).toString());

			++nfiles;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to delete image"));
				return;
			}
		}
		
		QString state = QString(tr("Deleted ") + QString::number(nfiles) + tr(" images"));
		setStatus(state);

		thumbView->loadVisibleThumbs();
	}
}

void Phototonic::goTo(QString path)
{
	thumbView->setNeedScroll(true);
	fsTree->setCurrentIndex(fsModel->index(path));
	thumbView->currentViewDir = path;
	refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex &idx)
{
	thumbView->setNeedScroll(true);
	thumbView->currentViewDir = getSelectedPath();
	refreshThumbs(true);
	fsTree->expand(idx);
}

void Phototonic::goPathBarDir()
{
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid Path: ") + pathBar->text());
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	selectCurrentViewDir();
	refreshThumbs(true);
}

void Phototonic::setThumbsFilter()
{
	thumbView->filterStr = filterBar->text();
	refreshThumbs(true);
}

void Phototonic::clearThumbsFilter()
{
	if (filterBar->text() == "")
	{
		thumbView->filterStr = filterBar->text();
		refreshThumbs(true);
	}
}

void Phototonic::goBack()
{
	if (currentHistoryIdx > 0)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(--currentHistoryIdx));
		goFrwdAction->setEnabled(true);
		if (currentHistoryIdx == 0)
			goBackAction->setEnabled(false);
	}
}

void Phototonic::goForward()
{

	if (currentHistoryIdx < pathHistory.size() - 1)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(++currentHistoryIdx));
		if (currentHistoryIdx == (pathHistory.size() - 1))
			goFrwdAction->setEnabled(false);
	}
}

void Phototonic::goUp()
{
	QFileInfo fileInfo = QFileInfo(thumbView->currentViewDir);
	goTo(fileInfo.dir().absolutePath());
}

void Phototonic::goHome()
{
	goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled)
{
	cutAction->setEnabled(setEnabled);
	copyAction->setEnabled(setEnabled);
}

void Phototonic::changeActionsBySelection(const QItemSelection&, const QItemSelection&)
{
	setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
}

void Phototonic::updateActions(QWidget*, QWidget *selectedWidget)
{
	if (selectedWidget == fsTree)
		setCopyCutActions(false);
	else if (selectedWidget == thumbView)
		setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
}

void Phototonic::writeSettings()
{
	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		GData::appSettings->setValue("Geometry", saveGeometry());
		GData::appSettings->setValue("WindowState", saveState());
	}

	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbSize);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
	GData::appSettings->setValue("thumbPagesReadahead", (int)GData::thumbPagesReadahead);
	GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
	GData::appSettings->setValue("exitInsteadOfClose", (int)GData::exitInsteadOfClose);
	GData::appSettings->setValue("enableAnimations", (bool)GData::enableAnimations);
	GData::appSettings->setValue("exifRotationEnabled", (bool)GData::exifRotationEnabled);
	GData::appSettings->setValue("exifThumbRotationEnabled", (bool)GData::exifThumbRotationEnabled);
	GData::appSettings->setValue("reverseMouseBehavior", (bool)GData::reverseMouseBehavior);
	GData::appSettings->setValue("showHiddenFiles", (bool)GData::showHiddenFiles);
	GData::appSettings->setValue("wrapImageList", (bool)GData::wrapImageList);
	GData::appSettings->setValue("imageZoomFactor", (float)GData::imageZoomFactor);
	GData::appSettings->setValue("shouldMaximize", (bool)isMaximized());
	GData::appSettings->setValue("defaultSaveQuality", (int)GData::defaultSaveQuality);
	GData::appSettings->setValue("noEnlargeSmallThumb", (bool)GData::noEnlargeSmallThumb);
	GData::appSettings->setValue("slideShowDelay", (int)GData::slideShowDelay);
	GData::appSettings->setValue("slideShowRandom", (bool)GData::slideShowRandom);
	GData::appSettings->setValue("editToolBarVisible", (bool)editToolBarVisible);
	GData::appSettings->setValue("goToolBarVisible", (bool)goToolBarVisible);
	GData::appSettings->setValue("viewToolBarVisible", (bool)viewToolBarVisible);
	GData::appSettings->setValue("fsDockVisible", (bool)GData::fsDockVisible);
	GData::appSettings->setValue("iiDockVisible", (bool)GData::iiDockVisible);
	GData::appSettings->setValue("pvDockVisible", (bool)GData::pvDockVisible);
	GData::appSettings->setValue("startupDir", (int)GData::startupDir);
	GData::appSettings->setValue("specifiedStartDir", GData::specifiedStartDir);
	GData::appSettings->setValue("lastDir", GData::startupDir == GData::rememberLastDir?
																		thumbView->currentViewDir: "");
	GData::appSettings->setValue("enableImageInfoFS", (bool)GData::enableImageInfoFS);
	GData::appSettings->setValue("showLabels", (bool)GData::showLabels);
	GData::appSettings->setValue("smallIcons", (bool)GData::smallIcons);
	GData::appSettings->setValue("LockDocks", (bool)GData::LockDocks);

	/* Action shortcuts */
	GData::appSettings->beginGroup("Shortcuts");
	QMapIterator<QString, QAction *> scIter(GData::actionKeys);
	while (scIter.hasNext())
	{
		scIter.next();
		GData::appSettings->setValue(scIter.key(), scIter.value()->shortcut().toString());
	}
	GData::appSettings->endGroup();

	/* External apps */
	GData::appSettings->beginGroup("ExternalApps");
	GData::appSettings->remove("");
	QMapIterator<QString, QString> eaIter(GData::externalApps);
	while (eaIter.hasNext())
	{
		eaIter.next();
		GData::appSettings->setValue(eaIter.key(), eaIter.value());
	}
	GData::appSettings->endGroup();

	/* copyMoveTo paths */
	int idx = 0;
	GData::appSettings->beginGroup("CopyMoveToPaths");
	GData::appSettings->remove("");
	QSetIterator<QString> pathsIter(GData::copyMoveToPaths);
	while (pathsIter.hasNext())
	{
		GData::appSettings->setValue("path" + ++idx, pathsIter.next());
	}
	GData::appSettings->endGroup();
}

void Phototonic::readSettings()
{
	initComplete = false;
	needThumbsRefresh = false;

	if (!GData::appSettings->contains("thumbsZoomVal"))
	{
		resize(800, 600);
		GData::appSettings->setValue("thumbsSortFlags", (int)0);
		GData::appSettings->setValue("thumbsZoomVal", (int)150);
		GData::appSettings->setValue("isFullScreen", (bool)false);
		GData::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
		GData::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
		GData::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
		GData::appSettings->setValue("thumbSpacing", (int)3);
		GData::appSettings->setValue("thumbPagesReadahead", (int)2);
		GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
		GData::appSettings->setValue("zoomOutFlags", (int)1);
		GData::appSettings->setValue("zoomInFlags", (int)0);
		GData::appSettings->setValue("wrapImageList", (bool)false);
		GData::appSettings->setValue("exitInsteadOfClose", (int)0);
		GData::appSettings->setValue("imageZoomFactor", (float)1.0);
		GData::appSettings->setValue("defaultSaveQuality", (int)90);
		GData::appSettings->setValue("noEnlargeSmallThumb", (bool)true);
		GData::appSettings->setValue("enableAnimations", (bool)true);
		GData::appSettings->setValue("exifRotationEnabled", (bool)true);
		GData::appSettings->setValue("exifThumbRotationEnabled", (bool)false);
		GData::appSettings->setValue("reverseMouseBehavior", (bool)false);
		GData::appSettings->setValue("showHiddenFiles", (bool)false);
		GData::appSettings->setValue("slideShowDelay", (int)5);
		GData::appSettings->setValue("slideShowRandom", (bool)false);
		GData::appSettings->setValue("editToolBarVisible", (bool)true);
		GData::appSettings->setValue("goToolBarVisible", (bool)true);
		GData::appSettings->setValue("viewToolBarVisible", (bool)true);
		GData::appSettings->setValue("fsDockVisible", (bool)true);
		GData::appSettings->setValue("iiDockVisible", (bool)true);
		GData::appSettings->setValue("pvDockVisible", (bool)true);
		GData::appSettings->setValue("enableImageInfoFS", (bool)false);
		GData::appSettings->setValue("showLabels", (bool)true);
		GData::appSettings->setValue("smallIcons", (bool)false);
		GData::appSettings->setValue("LockDocks", (bool)true);
	}

	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
	GData::exitInsteadOfClose = GData::appSettings->value("exitInsteadOfClose").toBool();
	GData::enableAnimations = GData::appSettings->value("enableAnimations").toBool();
	GData::exifRotationEnabled = GData::appSettings->value("exifRotationEnabled").toBool();
	GData::exifThumbRotationEnabled = GData::appSettings->value("exifThumbRotationEnabled").toBool();
	GData::reverseMouseBehavior = GData::appSettings->value("reverseMouseBehavior").toBool();
	GData::showHiddenFiles = GData::appSettings->value("showHiddenFiles").toBool();
	GData::wrapImageList = GData::appSettings->value("wrapImageList").toBool();
	GData::imageZoomFactor = GData::appSettings->value("imageZoomFactor").toFloat();
	GData::zoomOutFlags = GData::appSettings->value("zoomOutFlags").toInt();
	GData::zoomInFlags = GData::appSettings->value("zoomInFlags").toInt();
	GData::rotation = 0;
	GData::keepTransform = false;
	shouldMaximize = GData::appSettings->value("shouldMaximize").toBool();
	GData::flipH = false;
	GData::flipV = false;
	GData::defaultSaveQuality = GData::appSettings->value("defaultSaveQuality").toInt();
	GData::noEnlargeSmallThumb = GData::appSettings->value("noEnlargeSmallThumb").toBool();
	GData::slideShowDelay = GData::appSettings->value("slideShowDelay").toInt();
	GData::slideShowRandom = GData::appSettings->value("slideShowRandom").toBool();
	GData::slideShowActive = false;
	editToolBarVisible = GData::appSettings->value("editToolBarVisible").toBool();
	goToolBarVisible = GData::appSettings->value("goToolBarVisible").toBool();
	viewToolBarVisible = GData::appSettings->value("viewToolBarVisible").toBool();
	GData::fsDockVisible = GData::appSettings->value("fsDockVisible").toBool();
	GData::iiDockVisible = GData::appSettings->value("iiDockVisible").toBool();
	GData::pvDockVisible = GData::appSettings->value("pvDockVisible").toBool();
	GData::startupDir = (GData::StartupDir)GData::appSettings->value("startupDir").toInt();
	GData::specifiedStartDir = GData::appSettings->value("specifiedStartDir").toString();
	GData::enableImageInfoFS = GData::appSettings->value("enableImageInfoFS").toBool();
	GData::showLabels = GData::appSettings->value("showLabels").toBool();
	GData::smallIcons = GData::appSettings->value("smallIcons").toBool();
	GData::LockDocks = GData::appSettings->value("LockDocks").toBool();

	GData::appSettings->beginGroup("ExternalApps");
	QStringList extApps = GData::appSettings->childKeys();
	for (int i = 0; i < extApps.size(); ++i)
	{
		GData::externalApps[extApps.at(i)] = GData::appSettings->value(extApps.at(i)).toString();
	}
	GData::appSettings->endGroup();

	GData::appSettings->beginGroup("CopyMoveToPaths");
	QStringList paths = GData::appSettings->childKeys();
	for (int i = 0; i < paths.size(); ++i)
	{
		GData::copyMoveToPaths.insert(GData::appSettings->value(paths.at(i)).toString());
	}
	GData::appSettings->endGroup();
}

void Phototonic::setupDocks()
{
	pvDock = new QDockWidget(tr("Preview"), this);
	pvDock->setObjectName("Preview");
	pvDock->setWidget(thumbView->imagePreview);
	connect(pvDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setPvDockVisibility()));	
	connect(pvDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setPvDockVisibility()));	
	addDockWidget(Qt::LeftDockWidgetArea, pvDock);

	addDockWidget(Qt::LeftDockWidgetArea, iiDock);

	QAction *docksNToolbarsAct = viewMenu->insertMenu(refreshAction, QMainWindow::createPopupMenu());
	docksNToolbarsAct->setText(tr("Docks and Toolbars"));
	docksNToolbarsAct->menu()->addSeparator();
	docksNToolbarsAct->menu()->addAction(actSmallIcons);
	docksNToolbarsAct->menu()->addAction(actLockDocks);

	fsDockOrigWidget = fsDock->titleBarWidget();
	iiDockOrigWidget = iiDock->titleBarWidget();
	pvDockOrigWidget = pvDock->titleBarWidget();
	fsDockEmptyWidget = new QWidget;
	iiDockEmptyWidget = new QWidget;
	pvDockEmptyWidget = new QWidget;
	lockDocks();
}

void Phototonic::lockDocks()
{
	if (initComplete)
		GData::LockDocks = actLockDocks->isChecked();

	if (GData::LockDocks)
	{
		fsDock->setTitleBarWidget(fsDockEmptyWidget);
		iiDock->setTitleBarWidget(iiDockEmptyWidget);
		pvDock->setTitleBarWidget(pvDockEmptyWidget);
	}
	else
	{
		fsDock->setTitleBarWidget(fsDockOrigWidget);
		iiDock->setTitleBarWidget(iiDockOrigWidget);
		pvDock->setTitleBarWidget(pvDockOrigWidget);
	}
}

void Phototonic::loadShortcuts()
{
	// Add customizable key shortcut actions
	GData::actionKeys[thumbsGoTopAct->text()] = thumbsGoTopAct;
	GData::actionKeys[thumbsGoBottomAct->text()] = thumbsGoBottomAct;
	GData::actionKeys[closeImageAct->text()] = closeImageAct;
	GData::actionKeys[fullScreenAct->text()] = fullScreenAct;
	GData::actionKeys[settingsAction->text()] = settingsAction;
	GData::actionKeys[exitAction->text()] = exitAction;
	GData::actionKeys[thumbsZoomInAct->text()] = thumbsZoomInAct;
	GData::actionKeys[thumbsZoomOutAct->text()] = thumbsZoomOutAct;
	GData::actionKeys[cutAction->text()] = cutAction;
	GData::actionKeys[copyAction->text()] = copyAction;
	GData::actionKeys[nextImageAction->text()] = nextImageAction;
	GData::actionKeys[prevImageAction->text()] = prevImageAction;
	GData::actionKeys[deleteAction->text()] = deleteAction;
	GData::actionKeys[saveAction->text()] = saveAction;
	GData::actionKeys[saveAsAction->text()] = saveAsAction;
	GData::actionKeys[keepTransformAct->text()] = keepTransformAct;
	GData::actionKeys[keepZoomAct->text()] = keepZoomAct;
	GData::actionKeys[showClipboardAction->text()] = showClipboardAction;
	GData::actionKeys[copyImageAction->text()] = copyImageAction;
	GData::actionKeys[pasteImageAction->text()] = pasteImageAction;
	GData::actionKeys[renameAction->text()] = renameAction;
	GData::actionKeys[refreshAction->text()] = refreshAction;
	GData::actionKeys[pasteAction->text()] = pasteAction;
	GData::actionKeys[goBackAction->text()] = goBackAction;
	GData::actionKeys[goFrwdAction->text()] = goFrwdAction;
	GData::actionKeys[slideShowAction->text()] = slideShowAction;
	GData::actionKeys[firstImageAction->text()] = firstImageAction;
	GData::actionKeys[lastImageAction->text()] = lastImageAction;
	GData::actionKeys[randomImageAction->text()] = randomImageAction;
	GData::actionKeys[openAction->text()] = openAction;
	GData::actionKeys[zoomOutAct->text()] = zoomOutAct;
	GData::actionKeys[zoomInAct->text()] = zoomInAct;
	GData::actionKeys[resetZoomAct->text()] = resetZoomAct;
	GData::actionKeys[origZoomAct->text()] = origZoomAct;
	GData::actionKeys[rotateLeftAct->text()] = rotateLeftAct;
	GData::actionKeys[rotateRightAct->text()] = rotateRightAct;
	GData::actionKeys[freeRotateLeftAct->text()] = freeRotateLeftAct;
	GData::actionKeys[freeRotateRightAct->text()] = freeRotateRightAct;
	GData::actionKeys[flipHAct->text()] = flipHAct;
	GData::actionKeys[flipVAct->text()] = flipVAct;
	GData::actionKeys[cropAct->text()] = cropAct;
	GData::actionKeys[colorsAct->text()] = colorsAct;
	GData::actionKeys[mirrorDisabledAct->text()] = mirrorDisabledAct;
	GData::actionKeys[mirrorDualAct->text()] = mirrorDualAct;
	GData::actionKeys[mirrorTripleAct->text()] = mirrorTripleAct;
	GData::actionKeys[mirrorVDualAct->text()] = mirrorVDualAct;
	GData::actionKeys[mirrorQuadAct->text()] = mirrorQuadAct;
	GData::actionKeys[moveDownAct->text()] = moveDownAct;
	GData::actionKeys[moveUpAct->text()] = moveUpAct;
	GData::actionKeys[moveRightAct->text()] = moveRightAct;
	GData::actionKeys[moveLeftAct->text()] = moveLeftAct;
	GData::actionKeys[copyMoveAction->text()] = copyMoveAction;
	GData::actionKeys[goUpAction->text()] = goUpAction;
	GData::actionKeys[resizeAct->text()] = resizeAct;
	
	GData::appSettings->beginGroup("Shortcuts");
	QStringList groupKeys = GData::appSettings->childKeys();

	if (groupKeys.size())
	{
		for (int i = 0; i < groupKeys.size(); ++i)
		{
			if (GData::actionKeys.value(groupKeys.at(i)))
				GData::actionKeys.value(groupKeys.at(i))->setShortcut
											(GData::appSettings->value(groupKeys.at(i)).toString());
		}
	}
	else
	{
		thumbsGoTopAct->setShortcut(QKeySequence("Ctrl+Home"));
		thumbsGoBottomAct->setShortcut(QKeySequence("Ctrl+End"));
		closeImageAct->setShortcut(Qt::Key_Escape);
		fullScreenAct->setShortcut(QKeySequence("F"));
		settingsAction->setShortcut(QKeySequence("P"));
		exitAction->setShortcut(QKeySequence("Ctrl+Q"));
		cutAction->setShortcut(QKeySequence("Ctrl+X"));
		copyAction->setShortcut(QKeySequence("Ctrl+C"));
		deleteAction->setShortcut(QKeySequence("Del"));
		saveAction->setShortcut(QKeySequence("Ctrl+S"));
		copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
		pasteImageAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
		renameAction->setShortcut(QKeySequence("F2"));
		refreshAction->setShortcut(QKeySequence("F5"));
		pasteAction->setShortcut(QKeySequence("Ctrl+V"));
		goBackAction->setShortcut(QKeySequence("Alt+Left"));
		goFrwdAction->setShortcut(QKeySequence("Alt+Right"));
		goUpAction->setShortcut(QKeySequence("Alt+Up"));
		slideShowAction->setShortcut(QKeySequence("W"));
		nextImageAction->setShortcut(QKeySequence("PgDown"));
		prevImageAction->setShortcut(QKeySequence("PgUp"));
		firstImageAction->setShortcut(QKeySequence("Home"));
		lastImageAction->setShortcut(QKeySequence("End"));
		randomImageAction->setShortcut(QKeySequence("R"));
		openAction->setShortcut(QKeySequence("Return"));
		zoomOutAct->setShortcut(QKeySequence("Shift+Z"));
		zoomInAct->setShortcut(QKeySequence("Z"));
		resetZoomAct->setShortcut(QKeySequence("Ctrl+Z"));
		origZoomAct->setShortcut(QKeySequence("Alt+Z"));
		rotateLeftAct->setShortcut(QKeySequence("Ctrl+Left"));
		rotateRightAct->setShortcut(QKeySequence("Ctrl+Right"));
		freeRotateLeftAct->setShortcut(QKeySequence("Ctrl+Shift+Left"));
		freeRotateRightAct->setShortcut(QKeySequence("Ctrl+Shift+Right"));
		flipHAct->setShortcut(QKeySequence("Ctrl+Down"));
		flipVAct->setShortcut(QKeySequence("Ctrl+Up"));
		cropAct->setShortcut(QKeySequence("Ctrl+R"));
		colorsAct->setShortcut(QKeySequence("C"));
		mirrorDisabledAct->setShortcut(QKeySequence("Ctrl+1"));
		mirrorDualAct->setShortcut(QKeySequence("Ctrl+2"));
		mirrorTripleAct->setShortcut(QKeySequence("Ctrl+3"));
		mirrorVDualAct->setShortcut(QKeySequence("Ctrl+4"));
		mirrorQuadAct->setShortcut(QKeySequence("Ctrl+5"));
		moveDownAct->setShortcut(QKeySequence("Down"));
		moveUpAct->setShortcut(QKeySequence("Up"));
		moveLeftAct->setShortcut(QKeySequence("Left"));
		moveRightAct->setShortcut(QKeySequence("Right"));
		copyMoveAction->setShortcut(QKeySequence("M"));
		resizeAct->setShortcut(QKeySequence("Ctrl+L"));
	}
		
	GData::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event)
{
	thumbView->abort();
	writeSettings();
	if (!QApplication::clipboard()->image().isNull())
		QApplication::clipboard()->clear();
	event->accept();
}

void Phototonic::setStatus(QString state)
{
	stateLabel->setText("    " + state + "    ");
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			if (GData::reverseMouseBehavior)
			{
				fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				toggleFullScreen();
				event->accept();
			}
			else if (closeImageAct->isEnabled())
			{
				closeImage();
				event->accept();
			}
		}
	}
}

void Phototonic::mousePressEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::MiddleButton)
		{
			if (GData::reverseMouseBehavior && closeImageAct->isEnabled())
			{
				closeImage();
				event->accept();
			}
			else
			{
				fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				toggleFullScreen();
				event->accept();
			}
		}
		else if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(true, event->x(), event->y());
			QApplication::setOverrideCursor(Qt::ClosedHandCursor);
			event->accept();
		}
	}
}

void Phototonic::mouseReleaseEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(false, 0, 0);
			while (QApplication::overrideCursor())
				QApplication::restoreOverrideCursor();
		}
		
	}
}

void Phototonic::newImage()
{
	loadImageFile("");
}

void Phototonic::setThumbViewWidgetsVisible(bool visible)
{
	menuBar()->setVisible(visible);
	menuBar()->setDisabled(!visible);
	statusBar()->setVisible(visible);

	editToolBar->setVisible(visible? editToolBarVisible : false);
	goToolBar->setVisible(visible? goToolBarVisible : false);
	viewToolBar->setVisible(visible? viewToolBarVisible : false);

	fsDock->setVisible(visible? GData::fsDockVisible : false);
	iiDock->setVisible(visible? GData::iiDockVisible : false);
	pvDock->setVisible(visible? GData::pvDockVisible : false);
}

void Phototonic::openOp()
{
	if (QApplication::focusWidget() == fsTree)
		goSelectedDir(fsTree->getCurrentIndex());
	else if (QApplication::focusWidget() == thumbView)
	{
		QModelIndex idx;
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		if (indexesList.size() > 0)
			idx = indexesList.first();
		else
		{
			if (thumbView->thumbViewModel->rowCount() == 0)
			{
				setStatus(tr("No images"));
				return;
			}

			idx = thumbView->thumbViewModel->indexFromItem(thumbView->thumbViewModel->item(0));
			thumbView->selectionModel()->select(idx, QItemSelectionModel::Toggle);
			thumbView->setCurrentRow(0);
		}

		loadImagefromThumb(idx);
	}
	else if (QApplication::focusWidget() == filterBar)
	{
		setThumbsFilter();
	}
	else if (QApplication::focusWidget() == pathBar)
	{
		goPathBarDir();
	}
}

void Phototonic::setEditToolBarVisibility()
{
	editToolBarVisible = editToolBar->isVisible();
}

void Phototonic::setGoToolBarVisibility()
{
	goToolBarVisible = goToolBar->isVisible();
}

void Phototonic::setViewToolBarVisibility()
{
	viewToolBarVisible = viewToolBar->isVisible();
}

void Phototonic::setFsDockVisibility()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		return;

	GData::fsDockVisible = fsDock->isVisible();
}

void Phototonic::setIiDockVisibility()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		return;

	GData::iiDockVisible = iiDock->isVisible();
}

void Phototonic::setPvDockVisibility()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		return;

	GData::pvDockVisible = pvDock->isVisible();
}

void Phototonic::loadImageFile(QString imageFileName)
{
	imageView->loadImage(imageFileName);
	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		GData::appSettings->setValue("Geometry", saveGeometry());
		GData::appSettings->setValue("WindowState", saveState());
	
		stackedWidget->setCurrentIndex(imageViewIdx);
		setThumbViewWidgetsVisible(false);
		if (GData::isFullScreen == true)
		{
			shouldMaximize = isMaximized();
			showFullScreen();
			imageView->setCursorHiding(true);
		}
	}
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx)
{
	thumbView->setCurrentRow(idx.row());
	loadImageFile(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString());
	thumbView->setImageviewWindowTitle();
}

void Phototonic::loadImagefromCli()
{
	QFile imageFile(cliFileName);
	if(!imageFile.exists()) 
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to open file ") + cliFileName + tr(", file not found"));
		cliFileName = "";
		return;
	}

	loadImageFile(cliFileName);
	thumbView->setCurrentIndexByName(cliFileName);
	setWindowTitle(cliFileName + " - Phototonic");
}

void Phototonic::slideShow()
{
	if (GData::slideShowActive)
	{
		GData::slideShowActive = false;
		slideShowAction->setText(tr("Slide Show"));
		imageView->popMessage(tr("Slide show stopped"));

		SlideShowTimer->stop();
		delete SlideShowTimer;
		slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));
	}
	else
	{
		if (thumbView->thumbViewModel->rowCount() <= 0)
			return;
	
		if (stackedWidget->currentIndex() == thumbViewIdx)
		{
			QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
			if (indexesList.size() != 1)
				thumbView->setCurrentRow(0);
			else
				thumbView->setCurrentRow(indexesList.first().row());
		}
	
		GData::slideShowActive = true;

		SlideShowTimer = new QTimer(this);
		connect(SlideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowHandler()));
		SlideShowTimer->start(GData::slideShowDelay * 1000);

		slideShowAction->setText(tr("Stop Slide Show"));
		imageView->popMessage(tr("Slide show started"));
		slideShowAction->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/images/stop.png")));

		slideShowHandler();
	}
}

void Phototonic::slideShowHandler()
{
	if (GData::slideShowActive)
	{
		if (GData::slideShowRandom)
		{
			loadRandomImage();
		}
		else
		{
			int currentRow = thumbView->getCurrentRow();
			loadImageFile(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());
			thumbView->setImageviewWindowTitle();

			if (thumbView->getNextRow() > 0)
				thumbView->setCurrentRow(thumbView->getNextRow());
			else 
			{
				if (GData::wrapImageList)
					thumbView->setCurrentRow(0);
				else
					slideShow();
			}
		}
	}
}

void Phototonic::loadNextImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int nextRow = thumbView->getNextRow();
	if (nextRow < 0) 
	{
		if (GData::wrapImageList)
			nextRow = 0;
		else
			return;
	}

	loadImageFile(thumbView->thumbViewModel->item(nextRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(nextRow);
	thumbView->setImageviewWindowTitle();
}

void Phototonic::loadPrevImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int prevRow = thumbView->getPrevRow();
	if (prevRow < 0) 
	{
		if (GData::wrapImageList)
			prevRow = thumbView->getLastRow();
		else
			return;
	}
	
	loadImageFile(thumbView->thumbViewModel->item(prevRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(prevRow);
	thumbView->setImageviewWindowTitle();
}

void Phototonic::loadFirstImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	loadImageFile(thumbView->thumbViewModel->item(0)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(0);
	thumbView->setImageviewWindowTitle();
}

void Phototonic::loadLastImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int lastRow = thumbView->getLastRow();
	loadImageFile(thumbView->thumbViewModel->item(lastRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(lastRow);
	thumbView->setImageviewWindowTitle();
}

void Phototonic::loadRandomImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int randomRow = thumbView->getRandomRow();
	loadImageFile(thumbView->thumbViewModel->item(randomRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(randomRow);
	thumbView->setImageviewWindowTitle();
}

void Phototonic::scrollToLastImage()
{
	if (thumbView->thumbViewModel->rowCount() > 0) 
	{
		if (thumbView->setCurrentIndexByName(imageView->currentImageFullPath))
			thumbView->selectCurrentIndex();
	}
}

void Phototonic::selectRecentThumb()
{
	if (thumbView->thumbViewModel->rowCount() > 0) 
	{
		if (thumbView->setCurrentIndexByName(thumbView->recentThumb))
			thumbView->selectCurrentIndex();
	}
}

void Phototonic::closeImage()
{
	if (cliImageLoaded && GData::exitInsteadOfClose)
	{
		close();
		return;
	}

	setThumbViewWidgetsVisible(true);
	stackedWidget->setVisible(false);
	stackedWidget->setCurrentIndex(thumbViewIdx);
	stackedWidget->setVisible(true);

	if (isFullScreen())
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
	}
	while (QApplication::overrideCursor())
		QApplication::restoreOverrideCursor();

	if (GData::slideShowActive)
		slideShow();

	if (needThumbsRefresh)
	{
		needThumbsRefresh = false;
		refreshThumbs(true);
	}

	thumbView->setFocus(Qt::OtherFocusReason);
	thumbView->loadVisibleThumbs();
	setThumbviewWindowTitle();

	if (!needThumbsRefresh)
	{
		QTimer::singleShot(100, this, SLOT(scrollToLastImage()));
	}
}

void Phototonic::goBottom()
{
	thumbView->scrollToBottom();
}

void Phototonic::goTop()
{
	thumbView->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath)
{
	QApplication::restoreOverrideCursor();
	GData::copyOp = (keyMods == Qt::ControlModifier);
	QMessageBox msgBox;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		msgBox.critical(this, tr("Error"), tr("Can not move or copy images to this folder"));
		selectCurrentViewDir();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir)
	{
		msgBox.critical(this, tr("Error"), tr("Destination folder is same as source"));
		return;
	}

	if (dirOp)
	{
		QString dirOnly = 
			cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);

		QString question = tr("Move ") + dirOnly + tr(" to ") + destDir + "?";
		int ret = QMessageBox::question(this, tr("Move folder"), question,
							QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

		if (ret == QMessageBox::Yes)
		{
			QFile dir(cpMvDirPath);
			bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
			if (!ok)
			{
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to move folder"));
			}
			setStatus(tr("Folder moved"));
		}
	}
	else
	{
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false);
		QString state = QString((GData::copyOp? tr("Copied ") : tr("Moved ")) + QString::number(cpMvdialog->nfiles) + tr(" images"));
		setStatus(state);
		delete(cpMvdialog);
	}

	thumbView->loadVisibleThumbs();
}

void Phototonic::selectCurrentViewDir()
{
	QModelIndex idx = fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
	{
		fsTree->expand(idx);
		fsTree->setCurrentIndex(idx);
	}
}

void Phototonic::checkDirState(const QModelIndex &, int, int)
{
	if (!initComplete)
	{
		return;
	}

	if (thumbViewBusy)
	{
		thumbView->abort();
	}

	if (!QDir().exists(thumbView->currentViewDir))
	{
		thumbView->currentViewDir = "";
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::recordHistory(QString dir)
{
	if (!needHistoryRecord)
	{
		needHistoryRecord = true;
		return;
	}

	if (pathHistory.size() && dir == pathHistory.at(currentHistoryIdx))
		return;
		
	pathHistory.insert(++currentHistoryIdx, dir);

	// Need to clear irrelevant items from list
	if (currentHistoryIdx != pathHistory.size() - 1)
	{	
		goFrwdAction->setEnabled(false);
		for (int i = pathHistory.size() - 1; i > currentHistoryIdx ; --i)
		{
			pathHistory.removeAt(i);
		}
	}
}

void Phototonic::reloadThumbsSlot()
{
	if (thumbViewBusy || !initComplete)
	{	
		thumbView->abort();
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
		return;
	}

	if (thumbView->currentViewDir == "")
	{
		thumbView->currentViewDir = getSelectedPath();
		if (thumbView->currentViewDir == "")
			return;
	}

	QDir checkPath(thumbView->currentViewDir);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to open folder: ") + thumbView->currentViewDir);
		return;
	}

	thumbView->infoView->clear();
	thumbView->imagePreview->clear();
	pathBar->setText(thumbView->currentViewDir);
	recordHistory(thumbView->currentViewDir);
	if (currentHistoryIdx > 0)
		goBackAction->setEnabled(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		setThumbviewWindowTitle();
	}

	thumbViewBusy = true;
	thumbView->load();
}

void Phototonic::setThumbviewWindowTitle()
{
	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
}

void Phototonic::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = tr("Rename ") + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							tr("New name:"), QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid name entered"));
		selectCurrentViewDir();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to rename folder"));
		selectCurrentViewDir();
		return;
	}

	if (thumbView->currentViewDir == dirInfo.absoluteFilePath()) 
		fsTree->setCurrentIndex(fsModel->index(newFullPathName));
	else
		selectCurrentViewDir();
}

void Phototonic::rename()
{
	if (QApplication::focusWidget() == fsTree)
	{
		renameDir();
		return;
	}
		
	QString selectedImageFileName = thumbView->getSingleSelectionFilename();
	if (selectedImageFileName.isEmpty())
	{
		setStatus(tr("Invalid selection"));
		return;
	}

	bool ok;

	QString title = tr("Rename Image");
	QString newImageName = QInputDialog::getText(this,
									title, tr("Enter a new name for ")
									+ QFileInfo(selectedImageFileName).fileName()
									+ ":\t\t\t",
									QLineEdit::Normal,
									QFileInfo(selectedImageFileName).completeBaseName(),
									&ok);

	if (!ok)													
		return;

	if(newImageName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("No name entered"));
		return;
	}

	newImageName += "." + QFileInfo(selectedImageFileName).suffix();
	QString currnetFilePath = selectedImageFileName;
	QFile currentFile(currnetFilePath);
	QString newImageFullPath = thumbView->currentViewDir + QDir::separator() + newImageName;
	ok = currentFile.rename(newImageFullPath);

	if (ok)
	{
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageFullPath, 
																			thumbView->FileNameRole);
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName,
																			Qt::DisplayRole);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to rename image"));
	}
}

void Phototonic::deleteDir()
{
	bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = tr("Permanently delete ") + dirInfo.completeBaseName() + tr(" and all of its contents?");

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
	else
	{
		selectCurrentViewDir();
		return;
	}

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to delete folder"));
		selectCurrentViewDir();
	}

	QString state = QString(tr("Removed ") + deletePath);
	setStatus(state);

	if (thumbView->currentViewDir == deletePath) 
	{
		if (idxAbove.isValid())
			fsTree->setCurrentIndex(idxAbove);
	}
	else
		selectCurrentViewDir();
}

void Phototonic::createSubDirectory()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString newDirName = QInputDialog::getText(this, tr("New Sub folder"), 
							tr("New folder name:"), QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid name entered"));
		selectCurrentViewDir();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to create new folder"));
		selectCurrentViewDir();
		return;
	}

	setStatus(tr("Created ") + newDirName);
	fsTree->expand(selectedDirs[0]);
	selectCurrentViewDir();
}

void Phototonic::manageDir()
{
	setStatus(tr("Executing file manager..."));
	QDesktopServices::openUrl(QUrl("file:///" + getSelectedPath()));
}

QString Phototonic::getSelectedPath()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	if (selectedDirs.size() && selectedDirs[0].isValid())
	{
		QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));
		return dirInfo.absoluteFilePath();
	}
	else
		return "";
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{	
		if (event->modifiers() == Qt::ControlModifier)
		{
			if (event->delta() < 0)
				zoomOut();				
			else
				zoomIn();
		}
		else if (nextImageAction->isEnabled())
		{
			if (event->delta() < 0)
				loadNextImage();
			else
				loadPrevImage();
		}
	}
}

void Phototonic::showNewImageWarning(QWidget *parent)
{
	QMessageBox msgBox;
	msgBox.warning(parent, tr("Warning"), tr("Cannot perform action with temporary image"));
}

bool Phototonic::removeDirOp(QString dirToDelete)
{
	bool ok = true;
	QDir dir(dirToDelete);

	Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
					QDir::AllDirs | QDir::Files, QDir::DirsFirst))
	{
		if (info.isDir())
			ok = removeDirOp(info.absoluteFilePath());
		else 
			ok = QFile::remove(info.absoluteFilePath());

		if (!ok)
			return ok;
	}
	ok = dir.rmdir(dirToDelete);

	return ok;
}

void Phototonic::disableImageView()
{
	colorsAct->setEnabled(false);
	cropAct->setEnabled(false);
	resizeAct->setEnabled(false);
	closeImageAct->setEnabled(false);
	nextImageAction->setEnabled(false);
	prevImageAction->setEnabled(false);
	firstImageAction->setEnabled(false);
	lastImageAction->setEnabled(false);
	randomImageAction->setEnabled(false);
	slideShowAction->setEnabled(false);
	copyMoveAction->setEnabled(false);
	deleteAction->setEnabled(false);
}

void Phototonic::enableImageView()
{
	colorsAct->setEnabled(true);
	cropAct->setEnabled(true);
	resizeAct->setEnabled(true);
	closeImageAct->setEnabled(true);
	nextImageAction->setEnabled(true);
	prevImageAction->setEnabled(true);
	firstImageAction->setEnabled(true);
	lastImageAction->setEnabled(true);
	randomImageAction->setEnabled(true);
	slideShowAction->setEnabled(true);
	copyMoveAction->setEnabled(true);
	deleteAction->setEnabled(true);
}

