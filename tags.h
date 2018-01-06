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
#include "mdcache.h"

class ThumbView;

enum TagsDisplayMode {
    FolderTagsDisplay,
    SelectionTagsDisplay
};

enum TagIcons {
    TagIconDisabled,
    TagIconEnabled,
    TagIconMultiple,
    TagIconFilterDisabled,
    TagIconFilterEnabled,
    TagIconFilterNegate
};

class ImageTags : public QWidget {
Q_OBJECT

public:
    ImageTags(QWidget *parent, ThumbView *thumbView, MetadataCache *mdCache);

    void addTag(QString tagName, bool tagChecked);

    void readImageTagsToCache(const QString &imageFullPath);

    void showTagsFilter();

    void showSelectedImagesTags();

    void resetTagsState();

    bool isImageFilteredOut(QString imagePath);

    void removeTag();

    void populateTagsTree();

    QMenu *tagsMenu;
    QTreeWidget *tagsTree;
    bool folderFilteringActive;
    QAction *removeTagAction;
    TagsDisplayMode currentDisplayMode;

private:
    void readImageFileTags(QSet<QString> &tags, const QString &imageFullPath);

    bool writeTagsToImage(QString &imageFileName, QSet<QString> &tags);

    QSet<QString> getCheckedTags(Qt::CheckState tagState);

    void setTagIcon(QTreeWidgetItem *tagItem, TagIcons icon);

    void setActiveViewMode(TagsDisplayMode mode);

    void applyUserAction(QTreeWidgetItem *item);

    void applyUserAction(QList<QTreeWidgetItem *> tagsList);

    void redrawTree();

    QSet<QString> imageFilteringTags;
    QAction *addTagAction;
    QAction *addToSelectionAction;
    QAction *removeFromSelectionAction;
    QAction *clearTagsFilterAction;
    QAction *negateAction;
    QTreeWidgetItem *lastChangedTagItem;
    ThumbView *thumbView;
    QTabBar *tabs;
    MetadataCache *mdCache;
    bool negateFilterEnabled;

private slots:

    void tagClicked(QTreeWidgetItem *item, int column);

    void saveLastChangedTag(QTreeWidgetItem *item, int column);

    void applyTagFiltering();

    void showMenu(QPoint pt);

    void addNewTag();

    void addTagsToSelection();

    void clearTagFilters();

    void negateFilter();

    void removeTagsFromSelection();

    void tabsChanged(int index);

signals:

    void setStatus(QString state);

    void reloadThumbs();

};

#endif // TAGS_H

