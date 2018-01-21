/*
 *  Copyright (C) 2013-2018 Ofer Kashayov <oferkv@live.com>
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

#include "FileListWidget.h"

FileListWidget::FileListWidget(QWidget *parent) : QTreeWidget(parent) {
    setAcceptDrops(true);
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DropOnly);
    setColumnCount(1);
    setHeaderHidden(true);
    addFileListEntry();
    setMaximumHeight((int) (QFontMetrics(font()).height() * 1.6));
}

void FileListWidget::addFileListEntry() {
    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setText(0, "File List");
    item->setIcon(0, QIcon::fromTheme("edit-select-all"));
    insertTopLevelItem(0, item);
}

void FileListWidget::resizeTreeColumn(const QModelIndex &) {
    resizeColumnToContents(0);
}

void FileListWidget::dragEnterEvent(QDragEnterEvent *event) {
    QModelIndexList selectedDirs = selectionModel()->selectedRows();

    if (selectedDirs.size() > 0) {
        dndOrigSelection = selectedDirs[0];
    }
    event->acceptProposedAction();
}

void FileListWidget::dragMoveEvent(QDragMoveEvent *event) {
    setCurrentIndex(indexAt(event->pos()));
}

void FileListWidget::dropEvent(QDropEvent *event) {
    if (event->source()) {
        QString fileSystemTreeStr("FileSystemTree");
        bool dirOp = (event->source()->metaObject()->className() == fileSystemTreeStr);
        emit dropOp(event->keyboardModifiers(), dirOp, event->mimeData()->urls().at(0).toLocalFile());
    }
}

