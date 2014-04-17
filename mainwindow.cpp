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

#include "mainwindow.h"
#include "thumbview.h"
#include "dialogs.h"
#include "global.h"

Phototonic::Phototonic(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("Phototonic", "Phototonic");
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

	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), 
				this, SLOT(updateActions(QWidget*, QWidget*)));

	restoreGeometry(GData::appSettings->value("geometry").toByteArray());
	restoreState(GData::appSettings->value("MainWindowState").toByteArray());
	setWindowIcon(QIcon(":/images/phototonic.png"));

	splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(fsTree);
	splitter->addWidget(thumbView);
	splitter->restoreState(GData::appSettings->value("splitterSizes").toByteArray());

	stackedWidget = new QStackedWidget;
	stackedWidget->addWidget(splitter);
	stackedWidget->addWidget(imageView);
	setCentralWidget(stackedWidget);

	cliImageLoaded = handleArgs();
	if (cliImageLoaded)
		loadImagefromCli();

	initComplete = true;
	thumbViewBusy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;
	refreshThumbs(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
		thumbView->setFocus(Qt::OtherFocusReason);
}

bool Phototonic::handleArgs()
{
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			selectCurrentViewDir();
			return false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
			selectCurrentViewDir();
			cliFileName = cliArg.fileName();
			return true;
		}
	}
	
	return false;
}

static bool removeDirOp(QString dirToDelete)
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

void Phototonic::unsetBusy()
{	
	thumbViewBusy = false;
}

void Phototonic::createThumbView()
{
	thumbView = new ThumbView(this);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();

	connect(this, SIGNAL(abortThumbLoading()), thumbView, SLOT(abort()));
	connect(thumbView, SIGNAL(unsetBusy()), this, SLOT(unsetBusy()));
	connect(thumbView, SIGNAL(updateState(QString, QString)), this, SLOT(updateState(QString, QString)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));
}

void Phototonic::addMenuSeparator(QWidget *widget)
{
	QAction *separator = new QAction(this);
	separator->setSeparator(true);
	widget->addAction(separator);
}

void Phototonic::createImageView()
{
	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
	imageView = new ImageView(this);
	connect(saveAction, SIGNAL(triggered()), imageView, SLOT(saveImage()));
	connect(saveAsAction, SIGNAL(triggered()), imageView, SLOT(saveImageAs()));
	connect(copyImageAction, SIGNAL(triggered()), imageView, SLOT(copyImage()));
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
	imageView->addAction(flipHAct);
	imageView->addAction(flipVAct);
	imageView->addAction(cropAct);
	imageView->addAction(saveAction);
	imageView->addAction(saveAsAction);
	imageView->addAction(copyImageAction);
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
	imageView->addAction(openWithExteralApp);
	imageView->addAction(refreshAction);
	imageView->addAction(colorsAct);
	imageView->addAction(moveRightAct);
	imageView->addAction(moveLeftAct);
	imageView->addAction(moveUpAct);
	imageView->addAction(moveDownAct);

	// Actions
	imageView->ImagePopUpMenu->addAction(nextImageAction);
	imageView->ImagePopUpMenu->addAction(prevImageAction);
	imageView->ImagePopUpMenu->addAction(firstImageAction);
	imageView->ImagePopUpMenu->addAction(lastImageAction);
	imageView->ImagePopUpMenu->addAction(randomImageAction);
	imageView->ImagePopUpMenu->addAction(slideShowAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	zoomSubMenu = new QMenu("Zoom");
	zoomSubMenuAct = new QAction("Zoom", this);
	zoomSubMenuAct->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom.png")));
	zoomSubMenuAct->setMenu(zoomSubMenu);
	imageView->ImagePopUpMenu->addAction(zoomSubMenuAct);
	zoomSubMenu->addAction(zoomInAct);
	zoomSubMenu->addAction(zoomOutAct);
	zoomSubMenu->addAction(origZoomAct);
	zoomSubMenu->addAction(resetZoomAct);
	addMenuSeparator(zoomSubMenu);
	zoomSubMenu->addAction(keepZoomAct);

	transformSubMenu = new QMenu("Transform");
	transformSubMenuAct = new QAction("Transform", this);
	transformSubMenuAct->setMenu(transformSubMenu);
	imageView->ImagePopUpMenu->addAction(transformSubMenuAct);
	transformSubMenu->addAction(rotateRightAct);
	transformSubMenu->addAction(rotateLeftAct);
	transformSubMenu->addAction(flipHAct);
	transformSubMenu->addAction(flipVAct);
	transformSubMenu->addAction(cropAct);

	MirroringSubMenu = new QMenu("Mirror");
	mirrorSubMenuAct = new QAction("Mirror", this);
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
	
	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(copyImageAction);
	imageView->ImagePopUpMenu->addAction(saveAction);
	imageView->ImagePopUpMenu->addAction(saveAsAction);
	imageView->ImagePopUpMenu->addAction(deleteAction);
	imageView->ImagePopUpMenu->addAction(openWithMenuAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(fullScreenAct);
	imageView->ImagePopUpMenu->addAction(refreshAction);
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
	thumbsGoTopAct = new QAction("Top", this);
	connect(thumbsGoTopAct, SIGNAL(triggered()), this, SLOT(goTop()));

	thumbsGoBottomAct = new QAction("Bottom", this);
	connect(thumbsGoBottomAct, SIGNAL(triggered()), this, SLOT(goBottom()));

	closeImageAct = new QAction("Close Image", this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(closeImage()));

	fullScreenAct = new QAction("Full Screen", this);
	fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	
	settingsAction = new QAction("Preferences", this);
	settingsAction->setIcon(QIcon::fromTheme("document-properties", QIcon(":/images/settings.png")));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction("Exit", this);
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction("Enlarge Thumbnails", this);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
	if (thumbView->thumbHeight == 300)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction("Shrink Thumbnails", this);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
	if (thumbView->thumbHeight == 100)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction("Cut", this);
	cutAction->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png")));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutImages()));
	cutAction->setEnabled(false);

	copyAction = new QAction("Copy", this);
	copyAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png")));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyImages()));
	copyAction->setEnabled(false);
	
	deleteAction = new QAction("Delete", this);
	deleteAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	saveAction = new QAction("Save", this);
	saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

	saveAsAction = new QAction("Save As", this);
	copyImageAction = new QAction("Copy Image", this);

	renameAction = new QAction("Rename", this);
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction("Select All", this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction("About", this);
	aboutAction->setIcon(QIcon::fromTheme("help-about", QIcon(":/images/about.png")));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	// Sort actions
	actName = new QAction("Name", this);
	actTime = new QAction("Time", this);
	actSize = new QAction("Size", this);
	actType = new QAction("Type", this);
	actReverse = new QAction("Reverse", this);
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

	actName->setChecked(thumbView->thumbsSortFlags == QDir::Name || 
						thumbView->thumbsSortFlags == QDir::Reversed); 
	actTime->setChecked(thumbView->thumbsSortFlags & QDir::Time); 
	actSize->setChecked(thumbView->thumbsSortFlags & QDir::Size); 
	actType->setChecked(thumbView->thumbsSortFlags & QDir::Type); 
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	actClassic = new QAction("Classic Thumbs", this);
	actCompact = new QAction("Compact", this);
	actSquarish = new QAction("Squarish", this);
	connect(actClassic, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));
	connect(actCompact, SIGNAL(triggered()), this, SLOT(setCompactThumbs()));
	connect(actSquarish, SIGNAL(triggered()), this, SLOT(setSquarishThumbs()));
	actClassic->setCheckable(true);
	actCompact->setCheckable(true);
	actSquarish->setCheckable(true);
	actClassic->setChecked(GData::thumbsLayout == ThumbView::Classic); 
	actCompact->setChecked(GData::thumbsLayout == ThumbView::Compact); 
	actSquarish->setChecked(GData::thumbsLayout == ThumbView::Squares); 

	refreshAction = new QAction("Reload", this);
	refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

	pasteAction = new QAction("Paste Here", this);
	pasteAction->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteImages()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction("New Folder", this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));
	
	manageDirAction = new QAction("Manage", this);
	connect(manageDirAction, SIGNAL(triggered()), this, SLOT(manageDir()));

	goBackAction = new QAction("Back", this);
	goBackAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);

	goFrwdAction = new QAction("Forward", this);
	goFrwdAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);

	goUpAction = new QAction("Up", this);
	goUpAction->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/up.png")));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction("Home", this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon::fromTheme("go-home", QIcon(":/images/home.png")));

	slideShowAction = new QAction("Slide Show", this);
	connect(slideShowAction, SIGNAL(triggered()), this, SLOT(slideShow()));

	nextImageAction = new QAction("Next", this);
	nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));
	
	prevImageAction = new QAction("Previous", this);
	prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

	firstImageAction = new QAction("First", this);
	firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
	connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

	lastImageAction = new QAction("Last", this);
	lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
	connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

	randomImageAction = new QAction("Random", this);
	connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

	openAction = new QAction("Open", this);
	openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	connect(openAction, SIGNAL(triggered()), this, SLOT(openOp()));

	openWithSubMenu = new QMenu("Open With");
	openWithMenuAct = new QAction("Open With", this);
	openWithExteralApp = new QAction("", this);
	openWithMenuAct->setMenu(openWithSubMenu);
	chooseAppAct = new QAction("Choose...", this);
	openWithSubMenu->addAction(openWithExteralApp);
	openWithExteralApp->setVisible(false);
	openWithSubMenu->addSeparator();
	openWithSubMenu->addAction(chooseAppAct);
	connect(chooseAppAct, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));
	connect(openWithExteralApp, SIGNAL(triggered()), this, SLOT(runExternalApp()));

	zoomOutAct = new QAction("Zoom Out", this);
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
	zoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

	zoomInAct = new QAction("Zoom In", this);
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
	zoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

	resetZoomAct = new QAction("Reset Zoom", this);
	resetZoomAct->setIcon(QIcon::fromTheme("zoom-fit-best"));
	connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

	origZoomAct = new QAction("Original Size", this);
	origZoomAct->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
	connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

	keepZoomAct = new QAction("Keep Zoom", this);
	keepZoomAct->setCheckable(true);
	connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

	rotateLeftAct = new QAction("Rotate Left", this);
	rotateLeftAct->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
	connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

	rotateRightAct = new QAction("Rotate Right", this);
	rotateRightAct->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
	connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

	flipHAct = new QAction("Flip Horizontally", this);
	flipHAct->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
	connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));

	flipVAct = new QAction("Flip Vertically", this);
	flipVAct->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
	connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));

	cropAct = new QAction("Cropping", this);
	connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));

	colorsAct = new QAction("Colors", this);
	connect(colorsAct, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
	colorsAct->setIcon(QIcon(":/images/colors.png"));

	mirrorDisabledAct = new QAction("Disable", this);
	mirrorDualAct = new QAction("Dual", this);
	mirrorTripleAct = new QAction("Triple", this);
	mirrorVDualAct = new QAction("Dual Vertical", this);
	mirrorQuadAct = new QAction("Quad", this);

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

	keepTransformAct = new QAction("Keep Transformations", this);
	keepTransformAct->setCheckable(true);
	connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

	moveLeftAct = new QAction("Move Left", this);
	connect(moveLeftAct, SIGNAL(triggered()), this, SLOT(moveLeft()));
	moveRightAct = new QAction("Move Right", this);
	connect(moveRightAct, SIGNAL(triggered()), this, SLOT(moveRight()));
	moveUpAct = new QAction("Move Up", this);
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveUp()));
	moveDownAct = new QAction("Move Down", this);
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveDown()));

	invertSelectionAct = new QAction("Invert Selection", this);
	connect(invertSelectionAct, SIGNAL(triggered()), thumbView, SLOT(invertSelection()));
}

void Phototonic::createMenus()
{
	fileMenu = menuBar()->addMenu("File");
	fileMenu->addAction(openAction);
	fileMenu->addAction(createDirAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu("Edit");
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
	editMenu->addSeparator();
	editMenu->addAction(pasteAction);
	editMenu->addSeparator();
	editMenu->addAction(selectAllAction);
	editMenu->addAction(invertSelectionAct);
	editMenu->addSeparator();
	editMenu->addAction(settingsAction);

	goMenu = menuBar()->addMenu("Go");
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);
	goMenu->addSeparator();
	goMenu->addAction(thumbsGoTopAct);
	goMenu->addAction(thumbsGoBottomAct);

	viewMenu = menuBar()->addMenu("View");
	viewMenu->addAction(thumbsZoomInAct);
	viewMenu->addAction(thumbsZoomOutAct);
	sortMenu = viewMenu->addMenu("Sort By");
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
	sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
	viewMenu->addSeparator();

	viewMenu->addAction(slideShowAction);
	viewMenu->addSeparator();

	thumbLayoutsGroup = new QActionGroup(this);
	thumbLayoutsGroup->addAction(actClassic);
	thumbLayoutsGroup->addAction(actCompact);
	thumbLayoutsGroup->addAction(actSquarish);
	viewMenu->addActions(thumbLayoutsGroup->actions());

	viewMenu->addSeparator();
	viewMenu->addAction(refreshAction);

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu("Help");
	helpMenu->addAction(aboutAction);

	// thumbview context menu
	thumbView->addAction(openAction);
	thumbView->addAction(openWithMenuAct);
	thumbView->addAction(cutAction);
	thumbView->addAction(copyAction);
	thumbView->addAction(renameAction);
	thumbView->addAction(deleteAction);
	QAction *sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(pasteAction);
	sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(selectAllAction);
	thumbView->addAction(invertSelectionAct);
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::createToolBars()
{
	editToolBar = addToolBar("Edit");
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	editToolBar->setObjectName("Edit");

	goToolBar = addToolBar("Navigation");
	goToolBar->addAction(goBackAction);
	goToolBar->addAction(goFrwdAction);
	goToolBar->addAction(goUpAction);
	goToolBar->addAction(goHomeAction);
	goToolBar->setObjectName("Navigation");

	// path bar
	pathBar = new QLineEdit;
	pathComplete = new QCompleter(this);
	QDirModel *pathCompleteDirMod = new QDirModel;
	pathCompleteDirMod->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	pathComplete->setModel(pathCompleteDirMod);
	pathBar->setCompleter(pathComplete);
	pathBar->setMinimumWidth(400);
	pathBar->setMaximumWidth(600);
	connect(pathBar, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
	goToolBar->addWidget(pathBar);

	viewToolBar = addToolBar("View");
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);
	viewToolBar->setObjectName("View");

	viewToolBarWasVisible = GData::appSettings->value("viewToolBarWasVisible").toBool();
	editToolBarWasVisible = GData::appSettings->value("editToolBarWasVisible").toBool();
	goToolBarWasVisible = GData::appSettings->value("goToolBarWasVisible").toBool();
}

void Phototonic::createStatusBar()
{
	stateLabel = new QLabel("Initializing...");
	stateLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	infoLabel = new QLabel("");
	infoLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusBar()->addWidget(stateLabel);
	statusBar()->addWidget(infoLabel);
}

void Phototonic::createFSTree()
{
	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);

	fsTree = new FSTree(this);

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

void Phototonic::setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag)
{
	if (act->isChecked())
		thumbView->thumbsSortFlags |= SortFlag;
}

void Phototonic::sortThumbnains()
{
	thumbView->thumbsSortFlags = 0;
	setFlagsByQAction(actName, QDir::Name);
	setFlagsByQAction(actTime, QDir::Time);
	setFlagsByQAction(actSize, QDir::Size);
	setFlagsByQAction(actType, QDir::Type);
	setFlagsByQAction(actReverse, QDir::Reversed);
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

void Phototonic::refreshThumbs(bool scrollToTop)
{
	if (scrollToTop)
	{
		thumbView->setNeedScroll(true);
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
	else
	{
		thumbView->setNeedScroll(false);
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
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

void Phototonic::about()
{
	QMessageBox::about(this, "About Phototonic", "<h2>Phototonic v0.98</h2>"
							"<p>Image viewer and organizer</p>"
							"<p><a href=\"http://oferkv.github.io/phototonic/\">Home page</a></p>"
							"<p><a href=\"https://github.com/oferkv/phototonic/issues\">Reports Bugs</a></p>"
							"<p>Copyright &copy; 2013-2014 Ofer Kashayov</p>"
							"<p>Contact: oferkv@live.com</p>" "Built with Qt" QT_VERSION_STR);
}

void Phototonic::runExternalApp()
{
	QString imageFileFullPath;
	QString CurrentSelectionFilename("");

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		imageFileFullPath = externalAppPath + " \"" + imageView->currentImageFullPath + "\"";
	}
	else
	{
		CurrentSelectionFilename = thumbView->getSingleSelectionFilename();
		if (CurrentSelectionFilename.isEmpty())
		{
			updateState("Invalid selection");
			return;
		}

		imageFileFullPath = externalAppPath + " \"" + thumbView->currentViewDir
							+ QDir::separator() + CurrentSelectionFilename + "\"";
	}

	externalProcess.start(imageFileFullPath);
}

void Phototonic::updateExternalApps()
{
	if(!externalAppPath.isEmpty())
	{
		GData::actionKeys.remove(openWithExteralApp->text());
		QFileInfo fileInfo = QFileInfo(externalAppPath);
		openWithExteralApp->setVisible(true);
		openWithExteralApp->setText(fileInfo.fileName());
		openWithExteralApp->setIcon(QIcon::fromTheme(fileInfo.fileName()));
		GData::actionKeys[openWithExteralApp->text()] = openWithExteralApp;
	}
}

void Phototonic::chooseExternalApp()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Choose Application",
							thumbView->currentViewDir, "");
	if (fileName.isEmpty())
		return;
		
	QFileInfo fileInfo = QFileInfo(fileName);
	if (!fileInfo.isExecutable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Not an executable");
		return;
	}

	externalAppPath = fileName;
	openWithExteralApp->setShortcut(QKeySequence(""));
	updateExternalApps();
	runExternalApp();
}

void Phototonic::showSettings()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		imageView->setCursorOverrides(false);
	
	SettingsDialog *dialog = new SettingsDialog(this);
	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->setThumbColors();
		GData::imageZoomFactor = 1.0;

		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			imageView->resizeImage();
			newSettingsRefreshThumbs = true;
		}
		else
			refreshThumbs(false);
	}

	delete dialog;

	if (stackedWidget->currentIndex() == imageViewIdx)
		imageView->setCursorOverrides(true);
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

void Phototonic::createCopyCutFileList()
{
	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; ++tn)
	{
		QString sourceFile = thumbView->currentViewDir + QDir::separator() +
			thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->data(thumbView->FileNameRole).toString();
		GData::copyCutFileList.append(sourceFile);
	}
}

void Phototonic::cutImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = false;
	pasteAction->setEnabled(true);
}

void Phototonic::copyImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = true;
	pasteAction->setEnabled(true);
}

void Phototonic::thumbsZoomIn()
{
	if (thumbView->thumbSize < 300)
	{
		thumbView->thumbSize += 50;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbSize == 300)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::thumbsZoomOut()
{
	if (thumbView->thumbSize > 100)
	{
		thumbView->thumbSize -= 50;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbSize == 100)
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
	CropDialog *dialog = new CropDialog(this, imageView);

	imageView->setCursorOverrides(false);
	dialog->exec();
	imageView->setCursorOverrides(true);
}

void Phototonic::showColorsDialog()
{
	ColorsDialog *dialog = new ColorsDialog(this, imageView);
	imageView->setCursorOverrides(false);
	dialog->exec();
	imageView->setCursorOverrides(true);
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

void Phototonic::pasteImages()
{
	if (!copyCutCount)
		return;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Can not paste in " + destDir);
		selectCurrentViewDir();
		return;
	}

	if (thumbViewBusy)
	{
		abortThumbsLoad();
	}
	
	CpMvDialog *dialog = new CpMvDialog(this);
	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);
		
	dialog->exec(thumbView, destDir, pasteInCurrDir);
	QString state = QString((GData::copyOp? "Copied " : "Moved ") + 
								QString::number(dialog->nfiles) + " images");
	updateState(state);

	delete(dialog);

	selectCurrentViewDir();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	refreshThumbs(false);
}

void Phototonic::deleteSingleImage()
{
	bool ok;
	int ret;

	ret = QMessageBox::warning(this, "Delete image", "Permanently delete this image?",
									QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	int currentRow = thumbView->getCurrentRow();
	if (ret == QMessageBox::Yes)
	{
		ok = QFile::remove(thumbView->currentViewDir + QDir::separator() + 
			thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());

		if (ok)
		{
			 thumbView->thumbViewModel->removeRow(currentRow);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to delete image");
			return;
		}

		bool wrapImageListTmp = GData::wrapImageList;
		GData::wrapImageList = false;
		thumbView->setCurrentRow(currentRow - 1);
		if (thumbView->getNextRow() > currentRow)
			loadPrevImage();
		else
			loadNextImage();
		GData::wrapImageList = wrapImageListTmp;
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
		deleteSingleImage();
		return;
	}

	if (thumbView->selectionModel()->selectedIndexes().size() < 1)
	{
		updateState("Invalid selection");
		return;
	}

	bool ok;
	int ret = QMessageBox::warning(this, "Delete images", "Permanently delete selected images?",
										QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	QModelIndexList indexesList;
	int nfiles = 0;
	if (ret == QMessageBox::Yes)
	{

		if (thumbViewBusy)
			abortThumbsLoad();

		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->currentViewDir + QDir::separator() + 
				thumbView->thumbViewModel->item(indexesList.first().row())->data(thumbView->FileNameRole).toString());
				
			++nfiles;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, "Error", "Failed to delete image");
				return;
			}
		}
		
		QString state = QString("Deleted " + QString::number(nfiles) + " images");
		updateState(state);

		if (thumbViewBusy)
			refreshThumbs(false);
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

	if (fsTree->isExpanded(idx))
		fsTree->collapse(idx);
	else
		fsTree->expand(idx);
}

void Phototonic::goPathBarDir()
{
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid Path: " + pathBar->text());
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	selectCurrentViewDir();
	refreshThumbs(true);
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
	QString parentDir = 
			thumbView->currentViewDir.left(thumbView->currentViewDir.lastIndexOf(QDir::separator()));

	if (parentDir.size() == 0)
		parentDir = QDir::separator();
	goTo(parentDir);
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
	if (stackedWidget->currentIndex() == imageViewIdx)
		setThumbViewWidgetsVisible(true);

	if(isFullScreen())
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
	}

	QApplication::processEvents();
	GData::appSettings->setValue("geometry", saveGeometry());
	GData::appSettings->setValue("mainWindowState", saveState());
	GData::appSettings->setValue("splitterSizes", splitter->saveState());
	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbSize);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
	GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
	GData::appSettings->setValue("viewToolBarWasVisible", (bool)viewToolBar->isVisible());
	GData::appSettings->setValue("editToolBarWasVisible", (bool)editToolBar->isVisible());
	GData::appSettings->setValue("goToolBarWasVisible", (bool)goToolBar->isVisible());
	GData::appSettings->setValue("exitInsteadOfClose", (int)GData::exitInsteadOfClose);
	GData::appSettings->setValue("wrapImageList", (bool)GData::wrapImageList);
	GData::appSettings->setValue("imageZoomFactor", (float)GData::imageZoomFactor);
	GData::appSettings->setValue("shouldMaximize", (bool)isMaximized());
	GData::appSettings->setValue("defaultSaveQuality", (int)GData::defaultSaveQuality);
	GData::appSettings->setValue("noEnlargeSmallThumb", (bool)GData::noEnlargeSmallThumb);
	GData::appSettings->setValue("slideShowDelay", (int)GData::slideShowDelay);
	GData::appSettings->setValue("slideShowRandom", (bool)GData::slideShowRandom);	
	GData::appSettings->setValue("externalApp", externalAppPath);	

	/* Action shortcuts */
	GData::appSettings->beginGroup("Shortcuts");
	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		GData::appSettings->setValue(it.key(), it.value()->shortcut().toString());
	}
	GData::appSettings->endGroup();
}

void Phototonic::readSettings()
{
	initComplete = false;
	newSettingsRefreshThumbs = false;

	if (!GData::appSettings->contains("thumbsZoomVal"))
	{
		resize(800, 600);
		GData::appSettings->setValue("thumbsSortFlags", (int)0);
		GData::appSettings->setValue("thumbsZoomVal", (int)200);
		GData::appSettings->setValue("isFullScreen", (bool)false);
		GData::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
		GData::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
		GData::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
		GData::appSettings->setValue("thumbSpacing", (int)5);
		GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
		GData::appSettings->setValue("viewToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("editToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("goToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("zoomOutFlags", (int)1);
		GData::appSettings->setValue("zoomInFlags", (int)0);
		GData::appSettings->setValue("wrapImageList", (bool)false);
		GData::appSettings->setValue("exitInsteadOfClose", (int)0);
		GData::appSettings->setValue("imageZoomFactor", (float)1.0);
		GData::appSettings->setValue("defaultSaveQuality", (int)85);
		GData::appSettings->setValue("noEnlargeSmallThumb", (bool)true);
		GData::appSettings->setValue("slideShowDelay", (int)5);
		GData::appSettings->setValue("slideShowRandom", (bool)false);
	}

	GData::exitInsteadOfClose = GData::appSettings->value("exitInsteadOfClose").toBool();
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
	externalAppPath = GData::appSettings->value("externalApp").toString();

	// New config settings that need null protection
	if (!GData::slideShowDelay)
		GData::slideShowDelay = 5;
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
	GData::actionKeys[copyImageAction->text()] = copyImageAction;
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

	GData::appSettings->beginGroup("Shortcuts");
	QStringList groupKeys = GData::appSettings->childKeys();

	if (groupKeys.size())
	{
		for (int i = 0; i < groupKeys.size(); ++i)
		{
			if (GData::actionKeys.value(groupKeys.at(i)))
				GData::actionKeys.value(groupKeys.at(i))->setShortcut(GData::appSettings->value(groupKeys.at(i)).toString());
		}
	}
	else
	{
		thumbsGoTopAct->setShortcut(QKeySequence::MoveToStartOfDocument);
		thumbsGoBottomAct->setShortcut(QKeySequence::MoveToEndOfDocument);
		closeImageAct->setShortcut(Qt::Key_Escape);
		fullScreenAct->setShortcut(QKeySequence("f"));
		settingsAction->setShortcut(QKeySequence("Ctrl+P"));
		exitAction->setShortcut(QKeySequence("Ctrl+Q"));
		thumbsZoomInAct->setShortcut(QKeySequence::ZoomIn);
		thumbsZoomOutAct->setShortcut(QKeySequence::ZoomOut);
		cutAction->setShortcut(QKeySequence::Cut);
		copyAction->setShortcut(QKeySequence::Copy);
		deleteAction->setShortcut(QKeySequence::Delete);
		saveAction->setShortcut(QKeySequence::Save);
		copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
		renameAction->setShortcut(QKeySequence("F2"));
		refreshAction->setShortcut(QKeySequence("F5"));
		pasteAction->setShortcut(QKeySequence::Paste);
		goBackAction->setShortcut(QKeySequence::Back);
		goFrwdAction->setShortcut(QKeySequence::Forward);
		slideShowAction->setShortcut(QKeySequence("ctrl+w"));
		nextImageAction->setShortcut(QKeySequence::MoveToNextPage);
		prevImageAction->setShortcut(QKeySequence::MoveToPreviousPage);
		firstImageAction->setShortcut(QKeySequence::MoveToStartOfLine);
		lastImageAction->setShortcut(QKeySequence("End"));
		randomImageAction->setShortcut(QKeySequence("r"));
		openAction->setShortcut(QKeySequence::InsertParagraphSeparator);
		zoomOutAct->setShortcut(QKeySequence("-"));
		zoomInAct->setShortcut(QKeySequence("+"));
		resetZoomAct->setShortcut(QKeySequence("*"));
		origZoomAct->setShortcut(QKeySequence("/"));
		rotateLeftAct->setShortcut(QKeySequence::MoveToPreviousWord);
		rotateRightAct->setShortcut(QKeySequence::MoveToNextWord);
		flipHAct->setShortcut(QKeySequence("Ctrl+Down"));
		flipVAct->setShortcut(QKeySequence("Ctrl+Up"));
		cropAct->setShortcut(QKeySequence("R"));
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
	}
		
	GData::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event)
{
	abortThumbsLoad();
	writeSettings();
	event->accept();
}

void Phototonic::updateState(QString state, QString info)
{
	QString space("   ");
	state = space + state + space;
	stateLabel->setText(state);

	if (info.isEmpty())
		infoLabel->setVisible(false);
	else
	{
		infoLabel->setVisible(true);
		info = space + info + space;
		infoLabel->setText(info);
	}
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			closeImage();
			event->accept();
		}
	}
}

void Phototonic::mousePressEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::MiddleButton)
		{
			fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				
			toggleFullScreen();
			event->accept();
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
			QApplication::setOverrideCursor(Qt::OpenHandCursor);
		}
	}
}

void Phototonic::setThumbViewWidgetsVisible(bool visible)
{
	menuBar()->setVisible(visible);
	editToolBar->setVisible(visible? editToolBarWasVisible : false);
	goToolBar->setVisible(visible? goToolBarWasVisible : false);
	viewToolBar->setVisible(visible? viewToolBarWasVisible : false);
	statusBar()->setVisible(visible);
}

void Phototonic::openOp()
{
	if (QApplication::focusWidget() == fsTree)
		goSelectedDir(fsTree->getCurrentIndex());
	else
	{
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		if (indexesList.size() != 1)
		{
			updateState("Invalid selection");
			return;
		}

		loadImagefromThumb(indexesList.first());
	}
}

void Phototonic::loadImageFile(QString imageFileName)
{
	if (stackedWidget->currentIndex() != imageViewIdx)
	{
		setThumbViewWidgetsVisible(false);
		QApplication::setOverrideCursor(Qt::OpenHandCursor);
		if (GData::isFullScreen == true)
		{
			shouldMaximize = isMaximized();
			showFullScreen();
			imageView->setCursorHiding(true);
		}
		stackedWidget->setCurrentIndex(imageViewIdx);
	}
	
	imageView->loadImage(thumbView->currentViewDir, imageFileName);
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx)
{
	thumbView->setCurrentRow(idx.row());
	loadImageFile(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString());
}

void Phototonic::loadImagefromCli()
{
	QFile imageFile(thumbView->currentViewDir + QDir::separator() + cliFileName);
	if(!imageFile.exists()) 
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to open file \"" + cliFileName + "\", file does not exist");
		cliFileName = "";
		return;
	}

	loadImageFile(cliFileName);
	thumbView->setCurrentIndexByName(cliFileName);
}

void Phototonic::slideShow()
{
	if (GData::slideShowActive)
	{
		GData::slideShowActive = false;
		slideShowAction->setText("Slide Show");
	}
	else
	{
		if (stackedWidget->currentIndex() != imageViewIdx)
		{
			QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
			if (indexesList.size() != 1)
				thumbView->setCurrentRow(0);
			else
				thumbView->setCurrentRow(indexesList.first().row());
		}
	
		GData::slideShowActive = true;
		QTimer::singleShot(0, this, SLOT(slideShowHandler()));
		slideShowAction->setText("End Slide Show");
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
			if (thumbView->getNextRow() == thumbView->getCurrentRow())
				loadFirstImage();
			else
				loadNextImage();
		}

		QTimer::singleShot(GData::slideShowDelay * 1000, this, SLOT(slideShowHandler()));
	}
}

void Phototonic::loadNextImage()
{
	int nextRow = thumbView->getNextRow();
	if (GData::wrapImageList && nextRow == thumbView->getCurrentRow())
		nextRow = 0;

	loadImageFile(thumbView->thumbViewModel->item(nextRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(nextRow);
}

void Phototonic::loadPrevImage()
{
	int prevRow = thumbView->getPrevRow();
	if (GData::wrapImageList && prevRow == thumbView->getCurrentRow())
		prevRow = thumbView->getLastRow();
	
	loadImageFile(thumbView->thumbViewModel->item(prevRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(prevRow);
}

void Phototonic::loadFirstImage()
{
	loadImageFile(thumbView->thumbViewModel->item(0)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(0);
}

void Phototonic::loadLastImage()
{
	int lastRow = thumbView->getLastRow();
	loadImageFile(thumbView->thumbViewModel->item(lastRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(lastRow);
}

void Phototonic::loadRandomImage()
{
	int randomRow = thumbView->getRandomRow();
	loadImageFile(thumbView->thumbViewModel->item(randomRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(randomRow);
}

void Phototonic::closeImage()
{
	if (cliImageLoaded && GData::exitInsteadOfClose)
		close();

	if(isFullScreen())
	{
		imageView->setCursorHiding(false);
		showNormal();
		if (shouldMaximize)
			showMaximized();
	}
	while (QApplication::overrideCursor())
		QApplication::restoreOverrideCursor();

	setThumbViewWidgetsVisible(true);
	stackedWidget->setCurrentIndex(thumbViewIdx);
	thumbView->setCurrentIndexByName(imageView->currentImage);
	thumbView->selectCurrentIndex();

	if (newSettingsRefreshThumbs)
	{
		newSettingsRefreshThumbs = false;
		refreshThumbs(false);
	}

	if (GData::slideShowActive)
		slideShow();

	thumbView->setFocus(Qt::OtherFocusReason);
	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
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
		msgBox.critical(this, "Error", "Can not move or copy images to this folder");
		selectCurrentViewDir();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir)
	{
		msgBox.critical(this, "Error", "Destination folder is same as source");
		return;
	}

	if (thumbViewBusy)
		abortThumbsLoad();

	if (dirOp)
	{
		QString dirOnly = cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);
		QFile dir(cpMvDirPath);
		bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
		if (!ok)
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to move folder");
		}
		updateState("Folder moved");
	}
	else
	{
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false);
		QString state = QString((GData::copyOp? "Copied " : "Moved ") + QString::number(cpMvdialog->nfiles) + " images");
		updateState(state);
		delete(cpMvdialog);
	}

	if (thumbViewBusy)
		refreshThumbs(false);
}

void Phototonic::selectCurrentViewDir()
{
	QModelIndex idx = fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
	{
		fsTree->setCurrentIndex(idx);
		fsTree->expand(idx);
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
		abortThumbsLoad();
	}

	if (!QDir().exists(thumbView->currentViewDir))
	{
		thumbView->currentViewDir = "";
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::abortThumbsLoad()
{
	emit abortThumbLoading();
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
		for (int i = pathHistory.size() - 1; i > currentHistoryIdx ; i--)
		{
			pathHistory.removeAt(i);
		}
	}
}

void Phototonic::reloadThumbsSlot()
{
	if (thumbViewBusy || !initComplete)
	{	
		abortThumbsLoad();
		QTimer::singleShot(250, this, SLOT(reloadThumbsSlot()));
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
		msgBox.critical(this, "Error", "Failed to open folder: " + thumbView->currentViewDir);
		return;
	}

	pathBar->setText(thumbView->currentViewDir);
	recordHistory(thumbView->currentViewDir);
	if (currentHistoryIdx > 0)
		goBackAction->setEnabled(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		setWindowTitle(thumbView->currentViewDir + " - Phototonic");
	}

	thumbViewBusy = true;
	thumbView->load(cliFileName);
}

void Phototonic::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = "Rename " + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							"New name:", QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		selectCurrentViewDir();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to rename folder");
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
		updateState("Invalid selection");
		return;
	}

	bool ok;
	QString title = "Rename " + selectedImageFileName;
	QString newImageName = QInputDialog::getText(this, title, 
								"New name:", QLineEdit::Normal, selectedImageFileName, &ok);

	if (!ok)													
		return;

	if(newImageName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		return;
	}

	QString currnetFilePath = thumbView->currentViewDir + QDir::separator() + selectedImageFileName;
	QFile currentFile(currnetFilePath);
	ok = currentFile.rename(thumbView->currentViewDir + QDir::separator() + newImageName);

	if (ok)
	{
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, thumbView->FileNameRole);
		if (GData::thumbsLayout == ThumbView::Classic)
			thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, Qt::DisplayRole);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to rename file");
	}
}

void Phototonic::deleteDir()
{
	bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = "Permanently delete " + dirInfo.completeBaseName() + " and all of its contents?";

	int ret = QMessageBox::warning(this, "Delete folder", question,
								QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

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
		msgBox.critical(this, "Error", "Failed to delete folder");
		selectCurrentViewDir();
	}

	QString state = QString("Removed " + deletePath);
	updateState(state);

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
	QString newDirName = QInputDialog::getText(this, "New Sub folder", 
							"New folder name:", QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		selectCurrentViewDir();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to create new folder");
		selectCurrentViewDir();
		return;
	}

	updateState("Created " + newDirName);
	fsTree->expand(selectedDirs[0]);
	selectCurrentViewDir();
}

void Phototonic::manageDir()
{
	updateState("Opening file manager...");
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
		return 0;
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->delta() < 0)
			loadNextImage();
		else
			loadPrevImage();
	}
}

