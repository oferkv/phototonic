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

#ifndef TAGS_H
#define TAGS_H

#include <QtWidgets>
#include <exiv2/exiv2.hpp>
#include "thumbview.h"

class ThumbView;

enum TagsDisplayMode
{
	FolderTagsDisplay,
	SelectionTagsDisplay
};

class ImageTags : public QWidget
{
	Q_OBJECT

public:
	QMenu *tagsMenu;
	QTreeWidget *tagsTree;
	bool folderFilteringActive;
	QAction *removeTagAction;
	TagsDisplayMode currentDisplayMode;
	
	ImageTags(QWidget *parent, ThumbView *thumbView);
	void addTag(QString tagName, bool tagChecked);
	void readImageTagsToCache(const QString &imageFullPath);
	void addImageTagsToCache(const QString &imageFullPath, QSet<QString> imageFileTags);
	void showTagsFilter();
	void showSelectedImagesTags();
	void resetTagsState();
	bool isImageFilteredOut(QString imagePath);
	void removeTag();
	void populateTagsTree();

private:
	QSet<QString> imageFilteringTags;
	QMap<QString, QSet<QString> > imageTagsCache;
	QAction *addTagAction;
	QAction *addToSelectionAction;
	QAction *removeFromSelectionAction;
	QAction *clearTagsFilterAction;
	QTreeWidgetItem *lastChangedTagItem;
	ThumbView *thumbView;
	QTabBar *tabs;
	
	void readImageFileTags(QSet<QString> &tags, const QString &imageFullPath);
	bool writeTagsToImage(QString &imageFileName, QSet<QString> &tags);
	QSet<QString> getCheckedTags();
	void setTagIcon(QTreeWidgetItem *tagItem, Qt::CheckState status);
	void setActiveViewMode(TagsDisplayMode mode);
	void applyUserAction(QTreeWidgetItem *item);
	void applyUserAction(QList<QTreeWidgetItem *> tagsList);
	void redrawTree();

	void cacheUpdateImageTags(QString &imageFileName, QSet<QString> tags);
	bool cacheRemoveTagFromImage(QString &imageFileName, const QString &tagName);
	QSet<QString> &cacheGetImageTags(QString &imageFileName);
	void cacheSetImageTags(const QString &imageFileName, QSet<QString> tags);
	bool cacheAddTagToImage(QString &imageFileName, QString &tagName);
	void cacheClear();

private slots:
	void tagClicked(QTreeWidgetItem *item, int column);
	void saveLastChangedTag(QTreeWidgetItem *item, int column);
	void applyTagFiltering();
	void showMenu(QPoint pt);
	void addNewTag();
	void addTagsToSelection();
	void clearTagFilters();
	void removeTagsFromSelection();
	void tabsChanged(int index);

signals:
	void setStatus(QString state);
	void reloadThumbs();

};

#endif // TAGS_H

