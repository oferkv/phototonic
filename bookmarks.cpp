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

#include "bookmarks.h"

BookMarks::BookMarks(QWidget *parent) : QTreeWidget(parent)
{
	connect(this, SIGNAL(expanded(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));
	connect(this, SIGNAL(collapsed(const QModelIndex &)),
								this, SLOT(resizeTreeColumn(const QModelIndex &)));

	setColumnCount(1);
	setHeaderHidden(true);
	reloadBookmarks();
}

void BookMarks::reloadBookmarks()
{
	clear();
	QSetIterator<QString> it(GData::bookmarkPaths);
	while (it.hasNext()) {
		QString itemPath = it.next();
	    QTreeWidgetItem *item = new QTreeWidgetItem(this);
	    item->setText(0, QFileInfo(itemPath).fileName());
	   	item->setIcon(0, QIcon(":/images/bookmarks.png"));
	   	item->setToolTip(0, itemPath);
	    insertTopLevelItem(0, item);
	}
}

void BookMarks::resizeTreeColumn(const QModelIndex &)
{
	resizeColumnToContents(0);
}

