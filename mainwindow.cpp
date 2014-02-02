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

#include <QtGui>
#include "mainwindow.h"
#include "thumbview.h"
#include "dialogs.h"
#include "global.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("Phototonic", "Phototonic");
	initComplete = false;

	createThumbView();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createFSTree();
	createImageView();

    restoreGeometry(GData::appSettings->value("geometry").toByteArray());
    restoreState(GData::appSettings->value("MainWindowState").toByteArray());
   	connect(qApp, SIGNAL(focusChanged(QWidget* , QWidget*)), this, SLOT(updateActions(QWidget*, QWidget*)));
	setWindowTitle("Phototonic");
	setWindowIcon(QIcon(":/images/phototonic.png"));

	splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(fsTree);
	splitter->addWidget(thumbView);
    splitter->restoreState(GData::appSettings->value("splitterSizes").toByteArray());

    stackedWidget = new QStackedWidget;
	stackedWidget->addWidget(splitter);
	stackedWidget->addWidget(imageView);
    setCentralWidget(stackedWidget);

	if (argIsImageFile())
		loadImagefromCli(cliFileName);

    // Load current folder
   	initComplete = true;
    thumbViewBusy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;
	refreshThumbs(true);
}

MainWindow::~MainWindow()
{

}

bool MainWindow::argIsImageFile()
{
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			restoreCurrentIdx();
			return false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
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
 
	Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
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

void MainWindow::unsetBusy()
{	
	thumbViewBusy = false;
}

void MainWindow::createThumbView()
{
	int thumbSize = GData::appSettings->value("thumbsZoomVal").toInt();

	thumbView = new ThumbView(this, thumbSize);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();

	connect(this, SIGNAL(abortThumbLoading()), thumbView, SLOT(abort()));
	connect(thumbView, SIGNAL(unsetBusy()), this, SLOT(unsetBusy()));
	connect(thumbView, SIGNAL(updateState(QString)), this, SLOT(updateState(QString)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));
}

void MainWindow::createImageView()
{
	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
   	imageView = new ImageView(this);
   	imageView->setVisible(false);

	GData::zoomOutFlags = GData::appSettings->value("zoomOutFlags").toInt();
	if (!GData::zoomOutFlags)
		GData::zoomOutFlags = ImageView::WidthNHeight;
	GData::zoomInFlags = GData::appSettings->value("zoomInFlags").toInt();
	if (!GData::zoomInFlags)
		GData::zoomInFlags = ImageView::Disable;

  	imageView->addAction(closeImageAct);
  	imageView->addAction(fullScreenAct);
	QAction *sep = new QAction(this);
	sep->setSeparator(true);
	imageView->addAction(sep);
   	imageView->addAction(settingsAction);
	sep = new QAction(this);
	sep->setSeparator(true);
	imageView->addAction(sep);
	imageView->addAction(exitAction);

	imageView->setContextMenuPolicy(Qt::ActionsContextMenu);
	GData::isFullScreen = GData::appSettings->value("isFullScreen").toBool();
	fullScreenAct->setChecked(GData::isFullScreen); 
}

void MainWindow::createActions()
{
	closeImageAct = new QAction(tr("Close Image"), this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(closeImage()));
	closeImageAct->setShortcut(Qt::Key_Escape);

	fullScreenAct = new QAction(tr("Full Screen"), this);
    fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	fullScreenAct->setShortcut(tr("f"));
	
	settingsAction = new QAction(tr("Preferences"), this);
	settingsAction->setIcon(QIcon(":/images/settings.png"));
	settingsAction->setShortcut(QKeySequence(tr("F10")));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction(tr("E&xit"), this);
	exitAction->setShortcut(tr("Ctrl+Q"));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction(tr("Zoom In"), this);
	thumbsZoomInAct->setShortcut(QKeySequence::ZoomIn);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon(":/images/zoom_in.png"));
	if (thumbView->thumbHeight == 400)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction(tr("Zoom Out"), this);
	thumbsZoomOutAct->setShortcut(QKeySequence::ZoomOut);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon(":/images/zoom_out.png"));
	if (thumbView->thumbHeight == 100)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction(tr("Cut"), this);
	cutAction->setShortcut(QKeySequence::Cut);
   	cutAction->setIcon(QIcon(":/images/cut.png"));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutImages()));
	cutAction->setEnabled(false);

	copyAction = new QAction(tr("Copy"), this);
	copyAction->setShortcut(QKeySequence::Copy);
	copyAction->setIcon(QIcon(":/images/copy.png"));
   	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyImages()));
	copyAction->setEnabled(false);
	
	deleteAction = new QAction(tr("Delete"), this);
	deleteAction->setShortcut(QKeySequence::Delete);
	deleteAction->setIcon(QIcon(":/images/delete.png"));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	copyToAction = new QAction(tr("Copy to"), this);
	moveToAction = new QAction(tr("Move to"), this);

	renameAction = new QAction(tr("Rename"), this);
	renameAction->setShortcut(QKeySequence(tr("F2")));
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction(tr("Select All"), this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction(tr("&About"), this);
	aboutAction->setIcon(QIcon(":/images/about.png"));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	aboutQtAction = new QAction(tr("About &Qt"), this);
	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

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
	connect(actName, SIGNAL(triggered()), this, SLOT(setSortFlags()));
	connect(actTime, SIGNAL(triggered()), this, SLOT(setSortFlags()));
	connect(actSize, SIGNAL(triggered()), this, SLOT(setSortFlags()));
	connect(actType, SIGNAL(triggered()), this, SLOT(setSortFlags()));
	connect(actReverse, SIGNAL(triggered()), this, SLOT(setSortFlags()));

	actName->setChecked(thumbView->thumbsSortFlags == QDir::Name || thumbView->thumbsSortFlags == QDir::Reversed); 
	actTime->setChecked(thumbView->thumbsSortFlags & QDir::Time); 
	actSize->setChecked(thumbView->thumbsSortFlags & QDir::Size); 
	actType->setChecked(thumbView->thumbsSortFlags & QDir::Type); 
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	refreshAction = new QAction(tr("Refresh"), this);
	refreshAction->setShortcut(QKeySequence(tr("F5")));
	refreshAction->setIcon(QIcon(":/images/refresh.png"));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshThumbs()));

	pasteAction = new QAction(tr("&Paste Here"), this);
	pasteAction->setShortcut(QKeySequence::Paste);
   	pasteAction->setIcon(QIcon(":/images/paste.png"));    
   	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteImages()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction(tr("&New Folder"), this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon(":/images/new_folder.png"));
	
	manageDirAction = new QAction(tr("&Manage"), this);
	connect(manageDirAction, SIGNAL(triggered()), this, SLOT(manageDir()));

	goBackAction = new QAction(tr("&Back"), this);
	goBackAction->setIcon(QIcon(":/images/back.png"));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);
	goBackAction->setShortcut(QKeySequence::Back);
	// goBackAction->setToolTip("Back");

	goFrwdAction = new QAction(tr("&Forward"), this);
	goFrwdAction->setIcon(QIcon(":/images/next.png"));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);
	goFrwdAction->setShortcut(QKeySequence::Forward);

	goUpAction = new QAction(tr("&Up"), this);
	goUpAction->setIcon(QIcon(":/images/up.png"));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction(tr("&Home"), this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon(":/images/home.png"));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(createDirAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
    editMenu->addSeparator();
	editMenu->addAction(pasteAction);
    editMenu->addSeparator();
   	editMenu->addAction(selectAllAction);

    goMenu = menuBar()->addMenu(tr("&Go"));
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);

	viewMenu = menuBar()->addMenu(tr("&View"));
   	viewMenu->addAction(thumbsZoomInAct);
   	viewMenu->addAction(thumbsZoomOutAct);
    sortMenu = viewMenu->addMenu(tr("&Sort By"));
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
    sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
    viewMenu->addSeparator();
   	viewMenu->addAction(refreshAction);
    viewMenu->addSeparator();
   	viewMenu->addAction(settingsAction);

	menuBar()->addSeparator();
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(aboutQtAction);

    // thumbview context menu
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
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void MainWindow::createToolBars()
{
	editToolBar = addToolBar(tr("Edit"));
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	editToolBar->setObjectName("Edit");

	goToolBar = addToolBar(tr("Navigation"));
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

	viewToolBar = addToolBar(tr("View"));
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);
	viewToolBar->setObjectName("View");
}

void MainWindow::createStatusBar()
{
    stateLabel = new QLabel("Initializing...");
	statusBar()->addWidget(stateLabel);
}

void MainWindow::createFSTree()
{
	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);

	fsTree = new FSTree(this);

	QAction *sep1 = new QAction(this);
	QAction *sep2 = new QAction(this);
	sep1->setSeparator(true);
	sep2->setSeparator(true);

	// Context menu
	fsTree->addAction(createDirAction);
	fsTree->addAction(renameAction);
	fsTree->addAction(deleteAction);
	fsTree->addAction(sep1);
	fsTree->addAction(pasteAction);
	fsTree->addAction(sep2);
	fsTree->addAction(manageDirAction);
	fsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

	fsTree->setDragEnabled(true);
	fsTree->setAcceptDrops(true);
	fsTree->setDragDropMode(QAbstractItemView::InternalMove);

	
	fsTree->setModel(fsModel);
	for (int i = 1; i <= 3; i++)
		fsTree->hideColumn(i);
	fsTree->setHeaderHidden(true);
    connect(fsTree, SIGNAL(clicked(const QModelIndex&)),
    			this, SLOT(goSelectedDir(const QModelIndex &)));

	connect(fsModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
    			this, SLOT(checkDirState(const QModelIndex &, int, int)));

	connect(fsTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));
	QModelIndex idx = fsModel->index(QDir::currentPath()); 
	fsTree->setCurrentIndex(idx);
}

void MainWindow::setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag)
{
	if (act->isChecked())
		thumbView->thumbsSortFlags |= SortFlag;
}

void MainWindow::setSortFlags()
{
	thumbView->thumbsSortFlags = 0;
	setFlagsByQAction(actName, QDir::Name);
	setFlagsByQAction(actTime, QDir::Time);
	setFlagsByQAction(actSize, QDir::Size);
	setFlagsByQAction(actType, QDir::Type);
	setFlagsByQAction(actReverse, QDir::Reversed);
	refreshThumbs(false);
}

void MainWindow::refreshThumbs()
{
	refreshThumbs(false);
}

void MainWindow::refreshThumbs(bool scrollToTop)
{
	if (scrollToTop)
	{
		thumbView->setNeedScroll(true);
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
	else
	{
		thumbView->setNeedScroll(false);
		QTimer::singleShot(100, this, SLOT(reloadThumbsSlot()));
		QTimer::singleShot(200, thumbView, SLOT(updateIndex()));
	}
}

void MainWindow::about()
{
	QMessageBox::about(this, tr("About Phototonic"), tr("<h2>Phototonic v0.1</h2>"
											"<p>Copyright &copy; 2013 Ofer Kashayov"));
}

void MainWindow::showSettings()
{
	SettingsDialog *dialog = new SettingsDialog(this);

	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->setThumbColors();

		if (stackedWidget->currentIndex() == imageViewIdx)
			imageView->resizeImage();

		refreshThumbs(true);
	}

	delete dialog;
}

void MainWindow::toggleFullScreen()
{
	if (fullScreenAct->isChecked())
	{
		showFullScreen();
		GData::isFullScreen = true;
	}
	else
	{
		showNormal();
		GData::isFullScreen = false;
	}
}

void MainWindow::selectAllThumbs()
{
	thumbView->selectAll();
}

void MainWindow::createCopyCutFileList()
{
	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; tn++)
	{
		QString sourceFile = thumbView->currentViewDir + QDir::separator() +
			thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->data(thumbView->FileNameRole).toString();
		GData::copyCutFileList.append(sourceFile);
	}
}

void MainWindow::cutImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = false;
	pasteAction->setEnabled(true);
}

void MainWindow::copyImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = true;
	pasteAction->setEnabled(true);
}

void MainWindow::thumbsZoomIn()
{
	if (thumbView->thumbHeight < 400)
	{
		thumbView->thumbHeight += 50;
		thumbView->thumbWidth = thumbView->thumbHeight * GData::thumbAspect;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbHeight == 400)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void MainWindow::thumbsZoomOut()
{
	if (thumbView->thumbHeight > 100)
	{
		thumbView->thumbHeight -= 50;
		thumbView->thumbWidth = thumbView->thumbHeight * GData::thumbAspect;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbHeight == 100)
			thumbsZoomOutAct->setEnabled(false);
		refreshThumbs(false);
	}
}

bool MainWindow::isValidPath(QString &path)
{
	QDir checkPath(path);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		return false;
	}
	return true;
}

void MainWindow::pasteImages()
{
	if (!copyCutCount)
		return;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Can not paste in " + destDir);
		restoreCurrentIdx();
		return;
	}

	if (thumbViewBusy)
	{
		abortThumbsLoad();
	}
	
	CpMvDialog *dialog = new CpMvDialog(this);
	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);
		
	dialog->exec(thumbView, destDir, pasteInCurrDir, thumbViewBusy);
	QString state = QString((GData::copyOp? "Copied " : "Moved ") + QString::number(dialog->nfiles) + " images");
	updateState(state);

	delete(dialog);

	restoreCurrentIdx();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	if (thumbViewBusy)
		refreshThumbs(false);
}

void MainWindow::deleteOp()
{
	if (QApplication::focusWidget() == fsTree)
	{
		deleteDir();
		return;
	}

	int ret = QMessageBox::warning(this, tr("Delete images"), "Permanently delete selected images?",
										QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	if (ret == QMessageBox::Yes)
	{
		bool ok;
		if (thumbViewBusy)
		{
			abortThumbsLoad();
		}

		QModelIndexList indexesList;
		int nfiles = 0;
		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->currentViewDir + QDir::separator() + 
				thumbView->thumbViewModel->item(indexesList.first().row())->data(thumbView->FileNameRole).toString());
				
			nfiles++;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, "Error", "failed to delete image");
				return;
			}
		}
		QString state = QString("Deleted " + QString::number(nfiles) + " images");
		updateState(state);

		if (thumbViewBusy)
			refreshThumbs(false);
	}
}

void MainWindow::goTo(QString path)
{
	thumbView->setNeedScroll(true);
	fsTree->setCurrentIndex(fsModel->index(path));
	thumbView->currentViewDir = path;
	refreshThumbs(false);
}

void MainWindow::goSelectedDir(const QModelIndex&)
{
	thumbView->setNeedScroll(true);
	thumbView->currentViewDir = getSelectedPath();
	refreshThumbs(true);
}

void MainWindow::goPathBarDir()
{
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid Path: " + thumbView->currentViewDir);
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	restoreCurrentIdx();
	refreshThumbs(false);
}

void MainWindow::goBack()
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

void MainWindow::goForward()
{

	if (currentHistoryIdx < pathHistory.size() - 1)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(++currentHistoryIdx));
		if (currentHistoryIdx == (pathHistory.size() - 1))
			goFrwdAction->setEnabled(false);
	}
}

void MainWindow::goUp()
{
	QString parentDir = 
			thumbView->currentViewDir.left(thumbView->currentViewDir.lastIndexOf(QDir::separator()));

	if (parentDir.size() == 0)
		parentDir = QDir::separator();
	goTo(parentDir);
}

void MainWindow::goHome()
{
	goTo(QDir::homePath());
}

void MainWindow::setCopyCutActions(bool setEnabled)
{
	cutAction->setEnabled(setEnabled);
	copyAction->setEnabled(setEnabled);
}

void MainWindow::changeActionsBySelection(const QItemSelection&, const QItemSelection&)
{
	if (thumbView->selectionModel()->selectedIndexes().size())
		setCopyCutActions(true);
	else
		setCopyCutActions(false);
}

void MainWindow::updateActions(QWidget*, QWidget *selectedWidget)
{
	if (selectedWidget == fsTree)
		setCopyCutActions(false);
	else if (selectedWidget == thumbView)
	{
		if (thumbView->selectionModel()->selectedIndexes().size())
			setCopyCutActions(true);
	}
}

void MainWindow::writeSettings()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		setThumbViewWidgetsVisible(true);

	showNormal();
    GData::appSettings->setValue("geometry", saveGeometry());
	GData::appSettings->setValue("MainWindowState", saveState());
	GData::appSettings->setValue("splitterSizes", splitter->saveState());
	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbHeight);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("showThumbNames", (bool)GData::thumbsCompactLayout);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	abortThumbsLoad();
	writeSettings();
	event->accept();
}

void MainWindow::updateState(QString state)
{
    stateLabel->setText(state);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
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

void MainWindow::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MiddleButton)
	{
		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				
			toggleFullScreen();
	        event->accept();
		}
	}
}

void MainWindow::setThumbViewWidgetsVisible(bool visible)
{
	static bool viewToolBarWasVisible;
	static bool editToolBarWasVisible;
	static bool goToolBarWasVisible;

	if (!visible)
	{
		viewToolBarWasVisible = viewToolBar->isVisible();
		editToolBarWasVisible = editToolBar->isVisible();
		goToolBarWasVisible = goToolBar->isVisible();
	}

	menuBar()->setVisible(visible);
	editToolBar->setVisible(visible? editToolBarWasVisible : false);
	goToolBar->setVisible(visible? goToolBarWasVisible : false);
	viewToolBar->setVisible(visible? viewToolBarWasVisible : false);
	statusBar()->setVisible(visible);
}

void MainWindow::loadImagefromThumb(const QModelIndex &idx)
{
    currentImage = thumbView->currentViewDir;
    currentImage += QDir::separator();
	currentImage += thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString();
	
	imageView->loadImage(currentImage);
	setThumbViewWidgetsVisible(false);
	if (GData::isFullScreen == true)
		showFullScreen();
	stackedWidget->setCurrentIndex(imageViewIdx);
	setWindowTitle(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString() + " - Phototonic");
}

void MainWindow::loadImagefromCli(const QString &imageFileName)
{
    currentImage = thumbView->currentViewDir;
    currentImage += QDir::separator();
	currentImage += imageFileName;
	
	imageView->loadImage(currentImage);
	setThumbViewWidgetsVisible(false);
	if (GData::isFullScreen == true)
		showFullScreen();
	stackedWidget->setCurrentIndex(imageViewIdx);
	setWindowTitle(imageFileName + " - Phototonic");
}

void MainWindow::closeImage()
{
	if(isFullScreen())
	showNormal();
	setThumbViewWidgetsVisible(true);
	stackedWidget->setCurrentIndex(thumbViewIdx);
	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
}

void MainWindow::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath)
{
	QApplication::restoreOverrideCursor();
	GData::copyOp = (keyMods == Qt::ControlModifier);
	QMessageBox msgBox;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		msgBox.critical(this, "Error", "Can not move or copy images to this folder");
		restoreCurrentIdx();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir)
	{
		msgBox.critical(this, "Error", "Destination folder is same as source");
		return;
	}

	if (thumbViewBusy)
	{
		abortThumbsLoad();
	}

	if (dirOp)
	{
		QString dirOnly = cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);
		QFile dir(cpMvDirPath);
		bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
		if (!ok)
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "failed to move folder");
		}
		updateState("Folder moved");
	}
	else
	{
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false, thumbViewBusy);
		QString state = QString((GData::copyOp? "Copied " : "Moved ") + QString::number(cpMvdialog->nfiles) + " images");
		updateState(state);
		delete(cpMvdialog);
	}

	if (thumbViewBusy)
		refreshThumbs(false);
}

void MainWindow::restoreCurrentIdx()
{
	QModelIndex idx = fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
		fsTree->setCurrentIndex(idx);
}

void MainWindow::checkDirState(const QModelIndex &, int, int)
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
		QTimer::singleShot(500, this, SLOT(reloadThumbsSlot()));
	}
}

void MainWindow::abortThumbsLoad()
{
	emit abortThumbLoading();
}

void MainWindow::recordHistory(QString dir)
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

void MainWindow::reloadThumbsSlot()
{
	if (thumbViewBusy || !initComplete)
	{	
		abortThumbsLoad();
		QTimer::singleShot(10, this, SLOT(reloadThumbsSlot()));
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

	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
	thumbViewBusy = true;
	thumbView->load();
}

void MainWindow::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = "Rename " + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							tr("New name:"), QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		restoreCurrentIdx();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		restoreCurrentIdx();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to rename folder");
		restoreCurrentIdx();
		return;
	}

	if (thumbView->currentViewDir == dirInfo.absoluteFilePath()) 
		fsTree->setCurrentIndex(fsModel->index(newFullPathName));
	else
		restoreCurrentIdx();
}

void MainWindow::rename()
{
	if (QApplication::focusWidget() == fsTree)
	{
		renameDir();
		return;
	}
		
	QModelIndexList indexesList;
	indexesList = thumbView->selectionModel()->selectedIndexes();

	if (indexesList.size() != 1)
		return;

	bool ok;
	QString renameImageName = thumbView->thumbViewModel->item(indexesList.first().row())->data(thumbView->FileNameRole).toString();
	QString title = "Rename " + renameImageName;
	QString newImageName = QInputDialog::getText(this, title, 
										tr("New name:"), QLineEdit::Normal, renameImageName, &ok);

	if (!ok)													
		return;

	if(newImageName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		return;
	}

	QString currnetFilePath = thumbView->currentViewDir + QDir::separator() + renameImageName;
	QFile currentFile(currnetFilePath);
	ok = currentFile.rename(thumbView->currentViewDir + QDir::separator() + newImageName);

	if (ok)
	{
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, thumbView->FileNameRole);
		if (!GData::thumbsCompactLayout)
			thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, Qt::DisplayRole);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to rename file");
	}
}

void MainWindow::deleteDir()
{
    bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = "Permanently delete " + dirInfo.completeBaseName() + " and all of its contents?";

	int ret = QMessageBox::warning(this, tr("Delete folder"), question,
								QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	if (ret == QMessageBox::Yes)
		ok = removeDirOp(deletePath);
	else
	{
		restoreCurrentIdx();
		return;
	}

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to delete folder");
		restoreCurrentIdx();
	}

	QString state = QString("Removed " + deletePath);
	updateState(state);

	if (thumbView->currentViewDir == deletePath) 
	{
		if (idxAbove.isValid())
			fsTree->setCurrentIndex(idxAbove);
	}
	else
		restoreCurrentIdx();
}

void MainWindow::createSubDirectory()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString newDirName = QInputDialog::getText(this, "New Sub folder", 
							tr("New folder name:"), QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		restoreCurrentIdx();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		restoreCurrentIdx();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to create new folder");
		restoreCurrentIdx();
		return;
	}

	updateState("Created " + newDirName);
	fsTree->expand(selectedDirs[0]);
	restoreCurrentIdx();
}

void MainWindow::manageDir()
{
	updateState("Opening file manager...");
	QDesktopServices::openUrl(QUrl("file:///" + getSelectedPath()));
}

QString MainWindow::getSelectedPath()
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

