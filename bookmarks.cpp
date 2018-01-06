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

BookMarks::BookMarks(QWidget *parent) : QTreeWidget(parent) {
    setAcceptDrops(true);
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DropOnly);

    connect(this, SIGNAL(expanded(
                                 const QModelIndex &)),
            this, SLOT(resizeTreeColumn(
                               const QModelIndex &)));
    connect(this, SIGNAL(collapsed(
                                 const QModelIndex &)),
            this, SLOT(resizeTreeColumn(
                               const QModelIndex &)));

    setColumnCount(1);
    setHeaderHidden(true);
    reloadBookmarks();
}

void BookMarks::reloadBookmarks() {
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

void BookMarks::resizeTreeColumn(const QModelIndex &) {
    resizeColumnToContents(0);
}

void BookMarks::removeBookmark() {
    if (selectedItems().size() == 1) {
        GData::bookmarkPaths.remove(selectedItems().at(0)->toolTip(0));
        reloadBookmarks();
    }
}

void BookMarks::dragEnterEvent(QDragEnterEvent *event) {
    QModelIndexList selectedDirs = selectionModel()->selectedRows();

    if (selectedDirs.size() > 0) {
        dndOrigSelection = selectedDirs[0];
    }
    event->acceptProposedAction();
}

void BookMarks::dragMoveEvent(QDragMoveEvent *event) {
    setCurrentIndex(indexAt(event->pos()));
}

void BookMarks::dropEvent(QDropEvent *event) {
    if (event->source()) {
        QString fstreeStr("FSTree");
        bool dirOp = (event->source()->metaObject()->className() == fstreeStr);
        emit dropOp(event->keyboardModifiers(), dirOp, event->mimeData()->urls().at(0).toLocalFile());
    }
}

