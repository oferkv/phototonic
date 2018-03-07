/*
 *  Copyright (C) 2013-2015 Ofer Kashayov <oferkv@live.com>
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

#include "Tags.h"
#include "Settings.h"
#include "ProgressDialog.h"
#include "MessageBox.h"

ImageTags::ImageTags(QWidget *parent, ThumbsViewer *thumbsViewer, MetadataCache *metadataCache) : QWidget(parent) {
    tagsTree = new QTreeWidget;
    tagsTree->setColumnCount(2);
    tagsTree->setDragEnabled(false);
    tagsTree->setSortingEnabled(true);
    tagsTree->header()->close();
    tagsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->thumbView = thumbsViewer;
    this->metadataCache = metadataCache;
    negateFilterEnabled = false;

    tabs = new QTabBar(this);
    tabs->addTab(tr("Selection"));
    tabs->addTab(tr("Filter"));
    tabs->setTabIcon(0, QIcon(":/images/tag_yellow.png"));
    tabs->setTabIcon(1, QIcon(":/images/tag_filter_off.png"));
    tabs->setExpanding(false);
    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(tabsChanged(int)));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 3, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(tabs);
    mainLayout->addWidget(tagsTree);
    setLayout(mainLayout);
    currentDisplayMode = SelectionTagsDisplay;
    dirFilteringActive = false;

    connect(tagsTree, SIGNAL(itemChanged(QTreeWidgetItem * , int)),
            this, SLOT(saveLastChangedTag(QTreeWidgetItem * , int)));
    connect(tagsTree, SIGNAL(itemClicked(QTreeWidgetItem * , int)),
            this, SLOT(tagClicked(QTreeWidgetItem * , int)));

    tagsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tagsTree, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showMenu(QPoint)));

    addToSelectionAction = new QAction(tr("Tag"), this);
    addToSelectionAction->setIcon(QIcon(":/images/tag_yellow.png"));
    connect(addToSelectionAction, SIGNAL(triggered()), this, SLOT(addTagsToSelection()));

    removeFromSelectionAction = new QAction(tr("Untag"), this);
    connect(removeFromSelectionAction, SIGNAL(triggered()), this, SLOT(removeTagsFromSelection()));

    actionAddTag = new QAction(tr("New Tag"), this);
    actionAddTag->setIcon(QIcon(":/images/new_tag.png"));
    connect(actionAddTag, SIGNAL(triggered()), this, SLOT(addNewTag()));

    removeTagAction = new QAction(tr("Delete Tag"), this);
    removeTagAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

    actionClearTagsFilter = new QAction(tr("Clear Filters"), this);
    actionClearTagsFilter->setIcon(QIcon(":/images/tag_filter_off.png"));
    connect(actionClearTagsFilter, SIGNAL(triggered()), this, SLOT(clearTagFilters()));

    negateAction = new QAction(tr("Negate"), this);
    negateAction->setCheckable(true);
    connect(negateAction, SIGNAL(triggered()), this, SLOT(negateFilter()));

    tagsMenu = new QMenu("");
    tagsMenu->addAction(addToSelectionAction);
    tagsMenu->addAction(removeFromSelectionAction);
    tagsMenu->addSeparator();
    tagsMenu->addAction(actionAddTag);
    tagsMenu->addAction(removeTagAction);
    tagsMenu->addSeparator();
    tagsMenu->addAction(actionClearTagsFilter);
    tagsMenu->addAction(negateAction);
}

void ImageTags::redrawTagTree() {
    tagsTree->resizeColumnToContents(0);
    tagsTree->sortItems(0, Qt::AscendingOrder);
}

void ImageTags::showMenu(QPoint point) {
    QTreeWidgetItem *item = tagsTree->itemAt(point);
    addToSelectionAction->setEnabled(item != NULL);
    removeFromSelectionAction->setEnabled(item != NULL);
    removeTagAction->setEnabled(item != NULL);
    tagsMenu->popup(tagsTree->viewport()->mapToGlobal(point));
}

void ImageTags::setTagIcon(QTreeWidgetItem *tagItem, TagIcons icon) {
    switch (icon) {
        case TagIconDisabled:
            tagItem->setIcon(0, QIcon(":/images/tag_grey.png"));
            break;
        case TagIconEnabled:
            tagItem->setIcon(0, QIcon(":/images/tag_yellow.png"));
            break;
        case TagIconMultiple:
            tagItem->setIcon(0, QIcon(":/images/tag_multi.png"));
            break;
        case TagIconFilterEnabled:
            tagItem->setIcon(0, QIcon(":/images/tag_filter_on.png"));
            break;
        case TagIconFilterDisabled:
            tagItem->setIcon(0, QIcon(":/images/tag_filter_off.png"));
            break;
        case TagIconFilterNegate:
            tagItem->setIcon(0, QIcon(":/images/tag_filter_negate.png"));
            break;
    }
}

void ImageTags::addTag(QString tagName, bool tagChecked) {
    QTreeWidgetItem *tagItem = new QTreeWidgetItem();
    tagItem->setText(0, tagName);
    tagItem->setCheckState(0, tagChecked ? Qt::Checked : Qt::Unchecked);
    setTagIcon(tagItem, tagChecked ? TagIconEnabled : TagIconDisabled);
    tagsTree->addTopLevelItem(tagItem);
}

bool ImageTags::writeTagsToImage(QString &imageFileName, QSet<QString> &newTags) {
    QSet<QString> imageTags;
    Exiv2::Image::AutoPtr exifImage;

    try {
        exifImage = Exiv2::ImageFactory::open(imageFileName.toStdString());
        exifImage->readMetadata();

        Exiv2::IptcData newIptcData;

        /* copy existing data */
        Exiv2::IptcData &iptcData = exifImage->iptcData();
        if (!iptcData.empty()) {
            QString key;
            Exiv2::IptcData::iterator end = iptcData.end();
            for (Exiv2::IptcData::iterator iptcIt = iptcData.begin(); iptcIt != end; ++iptcIt) {
                if (iptcIt->tagName() != "Keywords") {
                    newIptcData.add(*iptcIt);
                }
            }
        }

        /* add new tags */
        QSetIterator<QString> newTagsIt(newTags);
        while (newTagsIt.hasNext()) {
            QString tag = newTagsIt.next();
            Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::string);
            value->read(tag.toStdString());
            Exiv2::IptcKey key("Iptc.Application2.Keywords");
            newIptcData.add(key, value.get());
        }

        exifImage->setIptcData(newIptcData);
        exifImage->writeMetadata();
    }
    catch (Exiv2::Error &error) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Failed to save tags to ") + imageFileName);
        return false;
    }

    return true;
}

void ImageTags::showSelectedImagesTags() {
    static bool busy = false;
    if (busy)
        return;
    busy = true;
    QStringList selectedThumbs = thumbView->getSelectedThumbsList();

    setActiveViewMode(SelectionTagsDisplay);

    int selectedThumbsNum = selectedThumbs.size();
    QMap<QString, int> tagsCount;
    for (int i = 0; i < selectedThumbsNum; ++i) {
        QSetIterator<QString> imageTagsIter(metadataCache->getImageTags(selectedThumbs[i]));
        while (imageTagsIter.hasNext()) {
            QString imageTag = imageTagsIter.next();
            tagsCount[imageTag]++;

            if (!Settings::knownTags.contains(imageTag)) {
                addTag(imageTag, true);
                Settings::knownTags.insert(imageTag);
            }
        }
    }

    bool imagesTagged = false, imagesTaggedMixed = false;
    QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
        QString tagName = (*it)->text(0);
        int tagCountTotal = tagsCount[tagName];

        if (selectedThumbsNum == 0) {
            (*it)->setCheckState(0, Qt::Unchecked);
            (*it)->setFlags((*it)->flags() & ~Qt::ItemIsUserCheckable);
            setTagIcon(*it, TagIconDisabled);
        } else if (tagCountTotal == selectedThumbsNum) {
            (*it)->setCheckState(0, Qt::Checked);
            (*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
            setTagIcon(*it, TagIconEnabled);
            imagesTagged = true;
        } else if (tagCountTotal) {
            (*it)->setCheckState(0, Qt::PartiallyChecked);
            (*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
            setTagIcon(*it, TagIconMultiple);
            imagesTaggedMixed = true;
        } else {
            (*it)->setCheckState(0, Qt::Unchecked);
            (*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
            setTagIcon(*it, TagIconDisabled);
        }
        ++it;
    }

    if (imagesTagged) {
        tabs->setTabIcon(0, QIcon(":/images/tag_yellow.png"));
    } else if (imagesTaggedMixed) {
        tabs->setTabIcon(0, QIcon(":/images/tag_multi.png"));
    } else {
        tabs->setTabIcon(0, QIcon(":/images/tag_grey.png"));
    }

    addToSelectionAction->setEnabled(selectedThumbsNum ? true : false);
    removeFromSelectionAction->setEnabled(selectedThumbsNum ? true : false);

    redrawTagTree();
    busy = false;
}

void ImageTags::showTagsFilter() {
    static bool busy = false;
    if (busy)
        return;
    busy = true;

    setActiveViewMode(DirectoryTagsDisplay);

    QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
        QString tagName = (*it)->text(0);

        (*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
        if (imageFilteringTags.contains(tagName)) {
            (*it)->setCheckState(0, Qt::Checked);
            setTagIcon(*it, negateFilterEnabled ? TagIconFilterNegate : TagIconFilterEnabled);
        } else {
            (*it)->setCheckState(0, Qt::Unchecked);
            setTagIcon(*it, TagIconFilterDisabled);
        }
        ++it;
    }

    redrawTagTree();
    busy = false;
}

void ImageTags::populateTagsTree() {
    tagsTree->clear();
    QSetIterator<QString> knownTagsIt(Settings::knownTags);
    while (knownTagsIt.hasNext()) {
        QString tag = knownTagsIt.next();
        addTag(tag, false);
    }

    redrawTagTree();

    if (currentDisplayMode == SelectionTagsDisplay) {
        showSelectedImagesTags();
    } else {
        showTagsFilter();
    }
}

void ImageTags::setActiveViewMode(TagsDisplayMode mode) {
    currentDisplayMode = mode;
    actionAddTag->setVisible(currentDisplayMode == SelectionTagsDisplay);
    removeTagAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
    addToSelectionAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
    removeFromSelectionAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
    actionClearTagsFilter->setVisible(currentDisplayMode == DirectoryTagsDisplay);
    negateAction->setVisible(currentDisplayMode == DirectoryTagsDisplay);
}

bool ImageTags::isImageFilteredOut(QString imageFileName) {
    QSet<QString> imageTags = metadataCache->getImageTags(imageFileName);

    QSetIterator<QString> filteredTagsIt(imageFilteringTags);
    while (filteredTagsIt.hasNext()) {
        if (imageTags.contains(filteredTagsIt.next())) {
            return negateFilterEnabled;
        }
    }

    return !negateFilterEnabled;
}

void ImageTags::resetTagsState() {
    tagsTree->clear();
    metadataCache->clear();
}

QSet<QString> ImageTags::getCheckedTags(Qt::CheckState tagState) {
    QSet<QString> checkedTags;
    QTreeWidgetItemIterator it(tagsTree);

    while (*it) {
        if ((*it)->checkState(0) == tagState) {
            checkedTags.insert((*it)->text(0));
        }
        ++it;
    }

    return checkedTags;
}

void ImageTags::applyTagFiltering() {
    imageFilteringTags = getCheckedTags(Qt::Checked);
    if (imageFilteringTags.size()) {
        dirFilteringActive = true;
        if (negateFilterEnabled) {
            tabs->setTabIcon(1, QIcon(":/images/tag_filter_negate.png"));
        } else {
            tabs->setTabIcon(1, QIcon(":/images/tag_filter_on.png"));
        }
    } else {
        dirFilteringActive = false;
        tabs->setTabIcon(1, QIcon(":/images/tag_filter_off.png"));
    }

    emit reloadThumbs();
}

void ImageTags::applyUserAction(QTreeWidgetItem *item) {
    QList<QTreeWidgetItem *> tagsList;
    tagsList << item;
    applyUserAction(tagsList);
}

void ImageTags::applyUserAction(QList<QTreeWidgetItem *> tagsList) {
    int processEventsCounter = 0;
    ProgressDialog *progressDialog = new ProgressDialog(this);
    progressDialog->show();

    QStringList currentSelectedImages = thumbView->getSelectedThumbsList();
    for (int currentImage = 0; currentImage < currentSelectedImages.size(); ++currentImage) {

        QString imageName = currentSelectedImages[currentImage];
        for (int i = tagsList.size() - 1; i > -1; --i) {
            Qt::CheckState tagState = tagsList.at(i)->checkState(0);
            setTagIcon(tagsList.at(i), (tagState == Qt::Checked ? TagIconEnabled : TagIconDisabled));
            QString tagName = tagsList.at(i)->text(0);

            if (tagState == Qt::Checked) {
                progressDialog->opLabel->setText(tr("Tagging ") + imageName);
                metadataCache->addTagToImage(imageName, tagName);
            } else {
                progressDialog->opLabel->setText(tr("Untagging ") + imageName);
                metadataCache->removeTagFromImage(imageName, tagName);
            }
        }

        if (!writeTagsToImage(imageName, metadataCache->getImageTags(imageName))) {
            metadataCache->removeImage(imageName);
        }

        ++processEventsCounter;
        if (processEventsCounter > 9) {
            processEventsCounter = 0;
            QApplication::processEvents();
        }

        if (progressDialog->abortOp) {
            break;
        }
    }

    progressDialog->close();
    delete (progressDialog);
}

void ImageTags::saveLastChangedTag(QTreeWidgetItem *item, int) {
    lastChangedTagItem = item;
}

void ImageTags::tabsChanged(int index) {
    if (!index) {
        showSelectedImagesTags();
    } else {
        showTagsFilter();
    }
}

void ImageTags::tagClicked(QTreeWidgetItem *item, int) {
    if (item == lastChangedTagItem) {
        if (currentDisplayMode == DirectoryTagsDisplay) {
            applyTagFiltering();
        } else {
            applyUserAction(item);
        }
        lastChangedTagItem = 0;
    }
}

void ImageTags::removeTagsFromSelection() {
    for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
        tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Unchecked);
    }

    applyUserAction(tagsTree->selectedItems());
}

void ImageTags::addTagsToSelection() {
    for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
        tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Checked);
    }

    applyUserAction(tagsTree->selectedItems());
}

void ImageTags::clearTagFilters() {
    QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
        (*it)->setCheckState(0, Qt::Unchecked);
        ++it;
    }

    imageFilteringTags.clear();
    applyTagFiltering();
}

void ImageTags::negateFilter() {
    negateFilterEnabled = negateAction->isChecked();
    applyTagFiltering();
}

void ImageTags::addNewTag() {
    bool ok;
    QString title = tr("Add a new tag");
    QString newTagName = QInputDialog::getText(this, title, tr("Enter new tag name"),
                                               QLineEdit::Normal, "", &ok);
    if (!ok) {
        return;
    }

    if (newTagName.isEmpty()) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("No name entered"));
        return;
    }

    QSetIterator<QString> knownTagsIt(Settings::knownTags);
    while (knownTagsIt.hasNext()) {
        QString tag = knownTagsIt.next();
        if (newTagName == tag) {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), tr("Tag ") + newTagName + tr(" already exists"));
            return;
        }
    }

    addTag(newTagName, false);
    Settings::knownTags.insert(newTagName);
    redrawTagTree();
}

void ImageTags::removeTag() {
    if (!tagsTree->selectedItems().size()) {
        return;
    }

    MessageBox msgBox(this);
    msgBox.setText(tr("Delete selected tags(s)?"));
    msgBox.setWindowTitle(tr("Delete tag"));
    msgBox.setIcon(MessageBox::Warning);
    msgBox.setStandardButtons(MessageBox::Yes | MessageBox::Cancel);
    msgBox.setDefaultButton(MessageBox::Cancel);
    msgBox.setButtonText(MessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(MessageBox::Cancel, tr("Cancel"));

    if (msgBox.exec() != MessageBox::Yes) {
        return;
    }

    bool removedTagWasChecked = false;
    for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {

        QString tagName = tagsTree->selectedItems().at(i)->text(0);
        Settings::knownTags.remove(tagName);

        if (imageFilteringTags.contains(tagName)) {
            imageFilteringTags.remove(tagName);
            removedTagWasChecked = true;
        }

        tagsTree->takeTopLevelItem(tagsTree->indexOfTopLevelItem(tagsTree->selectedItems().at(i)));
    }

    if (removedTagWasChecked) {
        applyTagFiltering();
    }
}

