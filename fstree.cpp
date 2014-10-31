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

#include "fstree.h"

FSTree::FSTree(QWidget *parent) : QTreeView(parent)
{
	setAcceptDrops(true);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::InternalMove);

	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	setModelFlags();

	setModel(fsModel);

	for (int i = 1; i <= 3; ++i) {
		hideColumn(i);
	}
	setHeaderHidden(true);

	connect(this, SIGNAL(expanded(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));
	connect(this, SIGNAL(collapsed(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));
}

QModelIndex FSTree::getCurrentIndex()
{
	return selectedIndexes().first();
}

void FSTree::resizeTreeColumn(const QModelIndex &)
{
	resizeColumnToContents(0);
}

void FSTree::dragEnterEvent(QDragEnterEvent *event)
{
	QModelIndexList selectedDirs = selectionModel()->selectedRows();
	if (selectedDirs.size() > 0) {
		dndOrigSelection = selectedDirs[0];
		event->acceptProposedAction();
	}
}

void FSTree::dragMoveEvent(QDragMoveEvent *event)
{
	setCurrentIndex(indexAt(event->pos()));
}

void FSTree::dropEvent(QDropEvent *event)
{
	if (event->source())
	{
		QString fstreeStr="FSTree";
		bool dirOp = (event->source()->metaObject()->className() == fstreeStr);
		emit dropOp(event->keyboardModifiers(), dirOp, event->mimeData()->urls().at(0).toLocalFile());
		setCurrentIndex(dndOrigSelection);
	}
}

void FSTree::setModelFlags()
{
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	if (GData::showHiddenFiles)
		fsModel->setFilter(fsModel->filter() | QDir::Hidden);
}

