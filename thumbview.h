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

#ifndef THUMBVIEW_H
#define THUMBVIEW_H

#include <QListView>
#include <QList>
#include <QDir>
#include <QDebug>
#include <QStandardItem>
#include <QTreeView>
#include "global.h"

class QListView;
class QStandardItemModel;

class ThumbView : public QListView
{
    Q_OBJECT

signals:
	void unsetBusy();
	void updateState(QString state);

public slots:
	void abort();
	void updateIndex();
	void handleSelectionChanged(const QItemSelection& selection);

public:
	ThumbView(QWidget *parent);
	void load(QString &cliImageName);
	void setNeedScroll(bool needScroll)
	{
		m_needScroll = needScroll;
	}
	void addNewThumb(QString &imageFileName);
	void setThumbColors();
	void setCurrentIndexByName(QString &FileName);
	void selectCurrentIndex();
	int getNextRow();
	int getPrevRow();
	int getLastRow();
	void setCurrentRow(int row);

	QDir *thumbsDir;
	QList<QStandardItem*> *thumbList;
	QList<bool> *thumbIsLoaded;
	QStandardItemModel *thumbViewModel;
	QString currentViewDir;
	QDir::SortFlags thumbsSortFlags;
	int thumbSize;
	int thumbWidth;
	int thumbHeight;

	enum UserRoles
	{
		FileNameRole = Qt::UserRole + 1,
		SortRole
	};

	enum ThumbnailLayouts
	{
		Classic,
		Compact,
		Squares
	};

protected:
    void startDrag(Qt::DropActions);
	void wheelEvent(QWheelEvent *event);
	
private:
	QFileInfo thumbFileInfo;
	QFileInfoList thumbFileInfoList;
	QImage emptyImg;
	QModelIndex currentIndex;
	
	bool abortOp;
	int newIndex;
	bool thumbLoaderActive;
	bool m_needScroll;
	int currentRow;

	void initThumbs();
	void loadThumbs();
	int getFirstVisibleItem();
	bool isItemVisible(QModelIndex idx);
};

class FSTree : public QTreeView
{
    Q_OBJECT

public:
	FSTree(QWidget *parent);
	~FSTree();

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

signals:
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);

private:
	QModelIndex dndOrigSelection;
	
};

#endif // THUMBVIEW_H

