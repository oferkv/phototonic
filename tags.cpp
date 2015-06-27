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

#include "tags.h"
#include "global.h"
#include "dialogs.h"

ImageTags::ImageTags(QWidget *parent) : QWidget(parent)
{
	tagsTree = new QTreeWidget;
	tagsTree->setColumnCount(2);
	tagsTree->setDragEnabled(false);
	tagsTree->setSortingEnabled(true);
	tagsTree->header()->close();
	tagsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(tagsTree);
	setLayout(mainLayout);

	currentDisplayMode = FolderTagsDisplay;
	folderFilteringActive = false;

	connect(tagsTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
				this, SLOT(saveLastChangedTag(QTreeWidgetItem *, int)));
	connect(tagsTree, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
				this, SLOT(tagClicked(QTreeWidgetItem *, int)));

	tagsTree->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(tagsTree, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showMenu(QPoint)));

	addToSelectionAction = new QAction(tr("Tag"), this);
	addToSelectionAction->setIcon(QIcon(":/images/tag_yellow.png"));
	connect(addToSelectionAction, SIGNAL(triggered()), this, SLOT(addTagsToSelection()));

	removeFromSelectionAction = new QAction(tr("Untag"), this);
	connect(removeFromSelectionAction, SIGNAL(triggered()), this, SLOT(removeTagsFromSelection()));

	clearFolderFiltersAction = new QAction(tr("Clear Filters"), this);
	clearFolderFiltersAction->setIcon(QIcon::fromTheme("edit-clear"));
	connect(clearFolderFiltersAction, SIGNAL(triggered()), this, SLOT(clearFolderFilters()));

	addTagAction = new QAction(tr("New Tag"), this);
	addTagAction->setIcon(QIcon(":/images/new_tag.png"));
	connect(addTagAction, SIGNAL(triggered()), this, SLOT(addNewTag()));

	removeTagAction = new QAction(tr("Remove Tag"), this);
	removeTagAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

	tagsMenu = new QMenu("");
	tagsMenu->addAction(addToSelectionAction);
	tagsMenu->addAction(removeFromSelectionAction);
	tagsMenu->addSeparator();
	tagsMenu->addAction(addTagAction);
	tagsMenu->addAction(removeTagAction);
	tagsMenu->addSeparator();
	tagsMenu->addAction(clearFolderFiltersAction);

	tagsRootItem = new QTreeWidgetItem();
	tagsTree->addTopLevelItem(tagsRootItem);
}

void ImageTags::redrawTree()
{
	tagsTree->expandItem(tagsRootItem);
	tagsTree->resizeColumnToContents(0);
	tagsTree->sortItems(0, Qt::AscendingOrder);
}

void ImageTags::showMenu(QPoint pt)
{
    QTreeWidgetItem *item = tagsTree->itemAt(pt);
	if (item)
    	tagsMenu->popup(tagsTree->viewport()->mapToGlobal(pt));
}

void ImageTags::setTagEnabled(QTreeWidgetItem *tagItem, Qt::CheckState status)
{
	if (status == Qt::Unchecked) {
	  	tagItem->setIcon(0, QIcon(":/images/tag_grey.png"));
  	} else if (status == Qt::Checked) {
	  	tagItem->setIcon(0, QIcon(":/images/tag_yellow.png"));
  	} else {
		tagItem->setIcon(0, QIcon(":/images/tag_multi.png"));
  	}
}

void ImageTags::addTag(QString tagName, bool tagChecked)
{
	QTreeWidgetItem *tagItem = new QTreeWidgetItem();

    tagItem->setText(0, tagName);
	tagItem->setCheckState(0, tagChecked? Qt::Checked : Qt::Unchecked);
	setTagEnabled(tagItem, tagChecked? Qt::Checked : Qt::Unchecked);

	tagsRootItem->addChild(tagItem);
}

void ImageTags::readImageFileTags(QSet<QString> &tags, const QString &imageFullPath)
{
	Exiv2::Image::AutoPtr exifImage;

	try {
		exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
		exifImage->readMetadata();
	}
	catch (Exiv2::Error &error) {
		return;
	}

	Exiv2::IptcData &iptcData = exifImage->iptcData();
	if (!iptcData.empty()) {
		QString key;
		Exiv2::IptcData::iterator end = iptcData.end();
		for (Exiv2::IptcData::iterator iptcIt = iptcData.begin(); iptcIt != end; ++iptcIt) {
			if (iptcIt->tagName() == "Keywords") {
				QString tagName = QString::fromUtf8(iptcIt->toString().c_str());
				tags.insert(tagName);
				GData::knownTags.insert(tagName);
			}
        }
    }
}

bool ImageTags::writeTagsToImage(QString &imageFileName, QSet<QString> &newTags)
{
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
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to save tags to ") + imageFileName);
		return false;
	}

	return true;
}

void ImageTags::readImageTagsToCache(const QString &imageFullPath)
{
	QSet<QString> imageTags;
	readImageFileTags(imageTags, imageFullPath);

	if (imageTags.size()) {
		cacheSetImageTags(imageFullPath, imageTags);
	}
			
    return;
}

void ImageTags::addImageTagsToCache(const QString &imageFullPath, QSet<QString> imageTags)
{
	if (imageTags.size()) {
		cacheSetImageTags(imageFullPath, imageTags);
	}
    return;
}

void ImageTags::showSelectedImagesTags(QStringList &selectedThumbs)
{
	static bool busy = false;
	if (busy)
		return;
	busy = true;

	if (currentDisplayMode != SelectionTagsDisplay || selectedThumbs.size() == 1) {
		setActiveViewMode(SelectionTagsDisplay);
		clearTagsRootItem();
	    tagsRootItem->setText(0, tr("Tags for selected images"));
		tagsRootItem->setIcon(0, QIcon::fromTheme("edit-select-all"));

		/* Show known tags */
		QSetIterator<QString> knownTagsIt(GData::knownTags);
		while (knownTagsIt.hasNext()) {
		    QString tag = knownTagsIt.next();
	        addTag(tag, imageFilteringTags.contains(tag));
	    }
	    
		redrawTree();
	}

	currentSelectedImages = selectedThumbs;
	QMap<QString, int> tagsCount;
	int totalSelected = currentSelectedImages.size();
	for (int i = 0; i < totalSelected; ++i) {
		QSetIterator<QString> imageTagsIter(cacheGetImageTags(currentSelectedImages[i]));
		while (imageTagsIter.hasNext()) {
			QString imageTag = imageTagsIter.next();
			tagsCount[imageTag]++;

			if (!GData::knownTags.contains(imageTag)) {
				addTag(imageTag, true);
				GData::knownTags.insert(imageTag);
			}
		}
	}

	QTreeWidgetItemIterator it(tagsTree);
	++it;
    while (*it) {
    	QString tagName = (*it)->text(0);
    	int tagCountTotal = tagsCount[tagName];

        if (tagCountTotal ==  totalSelected) {
        	(*it)->setCheckState(0, Qt::Checked);
        	setTagEnabled(*it, Qt::Checked);
       	} else if (tagCountTotal) {
			(*it)->setCheckState(0, Qt::PartiallyChecked);
        	setTagEnabled(*it, Qt::PartiallyChecked);
		}
       	
        ++it;
    }
	
   	busy = false;
}

void ImageTags::showFolderTags()
{
	static bool busy = false;
	if (busy)
		return;
	busy = true;
	
	clearTagsRootItem();
    tagsRootItem->setText(0, tr("Filter ") + QFileInfo(GData::currentViewDir).completeBaseName() + tr(" by tags"));
	tagsRootItem->setIcon(0, QIcon::fromTheme("folder"));

	/* Show known tags */
	QSetIterator<QString> knownTagsIt(GData::knownTags);
	while (knownTagsIt.hasNext()) {
	    QString tag = knownTagsIt.next();
        addTag(tag, imageFilteringTags.contains(tag));
    }
    
	setActiveViewMode(FolderTagsDisplay);
	redrawTree();
	busy = false;
}

void ImageTags::setActiveViewMode(TagsDisplayMode mode)
{
	currentDisplayMode = mode;
	
	if (currentDisplayMode == FolderTagsDisplay) {
		addToSelectionAction->setEnabled(false);
		removeFromSelectionAction->setEnabled(false);
		clearFolderFiltersAction->setEnabled(true);
		removeTagAction->setEnabled(true);
	} else {
		addToSelectionAction->setEnabled(true);
		removeFromSelectionAction->setEnabled(true);
		clearFolderFiltersAction->setEnabled(false);
		removeTagAction->setEnabled(false);
	}
}

bool ImageTags::isImageFilteredOut(QString imageFileName)
{
	QSet<QString> imageTags = cacheGetImageTags(imageFileName);

	QSetIterator<QString> folderCheckedTagsIt(imageFilteringTags);
	while (folderCheckedTagsIt.hasNext()) {
		QString checkedTag = folderCheckedTagsIt.next();
	    if (imageTags.contains(checkedTag)) {
			return false;
		}
	}

	return true;
}

void ImageTags::clearTagsRootItem()
{
	QTreeWidgetItem *tagItem;
	
	for (int i = tagsRootItem->childCount(); i > -1; --i) {
		tagItem = tagsRootItem->takeChild(i);
		delete tagItem;
	}
}

void ImageTags::resetTagsState()
{
	clearTagsRootItem();
	cacheClear();
	currentDisplayMode = FolderTagsDisplay;
}

QSet<QString> ImageTags::getCheckedTags()
{
	QSet<QString> checkedTags;

    QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
        if ((*it)->checkState(0))
	        checkedTags.insert((*it)->text(0));
        ++it;
    }

	return checkedTags;
}

void ImageTags::applyTagFiltering()
{
	imageFilteringTags = getCheckedTags();
	if (imageFilteringTags.size()) {
		folderFilteringActive = true;
	} else {
		folderFilteringActive = false;
	}
	
	emit reloadThumbs();
}

void ImageTags::applyUserAction(QTreeWidgetItem *item)
{
	QList<QTreeWidgetItem *> tagsList;
	tagsList << item;
	applyUserAction(tagsList);
}

void ImageTags::applyUserAction(QList<QTreeWidgetItem *> tagsList)
{
	int processEventsCounter = 0;
	ProgressDialog *dialog = new ProgressDialog(this);
	dialog->show();

	for (int i = 0; i < currentSelectedImages.size(); ++i) {

		QString imageName = currentSelectedImages[i]; 
		for (int i = tagsList.size() - 1; i > -1; --i) {
			if (tagsList.at(i) == tagsRootItem) {
				continue;
			}

			Qt::CheckState tagState = tagsList.at(i)->checkState(0);
			setTagEnabled(tagsList.at(i), tagState);
			QString tagName = tagsList.at(i)->text(0);

			if (tagState == Qt::Checked) {
				dialog->opLabel->setText(tr("Tagging ") + imageName);
				if (cacheAddTagToImage(imageName, tagName)) {
					writeTagsToImage(imageName, cacheGetImageTags(imageName));
				}
			} else {
				dialog->opLabel->setText(tr("Untagging ") + imageName);
				if (cacheRemoveTagFromImage(imageName, tagName)) {
					writeTagsToImage(imageName, cacheGetImageTags(imageName));
				}
			}
		}

		writeTagsToImage(imageName, cacheGetImageTags(imageName));

		++processEventsCounter;
		if (processEventsCounter > 9) {
			processEventsCounter = 0;
			QApplication::processEvents();
		}

		if (dialog->abortOp) {
			break;
		}
	}

	dialog->close();
	delete(dialog);
}

void ImageTags::saveLastChangedTag(QTreeWidgetItem *item, int)
{
	lastChangedTagItem = item;
}

void ImageTags::tagClicked(QTreeWidgetItem *item, int)
{
	if (item == lastChangedTagItem && item != tagsRootItem) {
		if (currentDisplayMode == FolderTagsDisplay) {

			if (imageFilteringTags.size() != getCheckedTags().size()) {
				applyTagFiltering();
			}
		} else {
			applyUserAction(item);
		}
		lastChangedTagItem = 0;
	}
}

void ImageTags::removeTagsFromSelection()
{	
	for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
		if (tagsTree->selectedItems().at(i) == tagsRootItem) {
			continue;
		}

		tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Unchecked);
	}

	applyUserAction(tagsTree->selectedItems());
}

void ImageTags::addTagsToSelection()
{	
	for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
		if (tagsTree->selectedItems().at(i) == tagsRootItem) {
			continue;
		}

		tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Checked);
	}

	applyUserAction(tagsTree->selectedItems());
}

void ImageTags::clearFolderFilters()
{	
	for (int i = 0; i < tagsRootItem->childCount(); ++i) {
		tagsRootItem->child(i)->setCheckState(0, Qt::Unchecked);
	}

	imageFilteringTags.clear();
	folderFilteringActive = false;
	emit reloadThumbs();
}

void ImageTags::addNewTag()
{	
	bool ok;
	QString title = tr("Add a new tag");
	QString newTagName = QInputDialog::getText(this, title, tr("Enter new tag name"),
												QLineEdit::Normal, "", &ok);
	if (!ok) {
		return;
	}

	if(newTagName.isEmpty()) {
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("No name entered"));
		return;
	}

	addTag(newTagName, false);
	GData::knownTags.insert(newTagName);
}

void ImageTags::removeTag()
{	
	if (!tagsTree->selectedItems().size()) {
		return;
	}

	QMessageBox msgBox;

	msgBox.setText(tr("Remove selected tags(s)?"));
	msgBox.setWindowTitle(tr("Remove tag"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setWindowIcon(QIcon(":/images/tag.png"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  

	if (msgBox.exec() != QMessageBox::Yes) {
		return;
	}

	bool removedTagWasChecked = false;
	for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
		if (tagsTree->selectedItems().at(i) == tagsRootItem) {
			continue;
		}

		QString tagName = tagsTree->selectedItems().at(i)->text(0);
		GData::knownTags.remove(tagName);

		if (imageFilteringTags.contains(tagName)) {
			imageFilteringTags.remove(tagName);      
			removedTagWasChecked = true;
		}

		tagsRootItem->takeChild(tagsRootItem->indexOfChild(tagsTree->selectedItems().at(i)));
	}

	if (removedTagWasChecked) {
		applyTagFiltering();
	}
}

void ImageTags::cacheUpdateImageTags(QString &imageFileName, QSet<QString> tags)
{
	imageTagsCache[imageFileName] = tags;
}

bool ImageTags::cacheRemoveTagFromImage(QString &imageFileName, const QString &tagName)
{
	return imageTagsCache[imageFileName].remove(tagName);
}

QSet<QString> &ImageTags::cacheGetImageTags(QString &imageFileName)
{
	return imageTagsCache[imageFileName] ;
}

void ImageTags::cacheSetImageTags(const QString &imageFileName, QSet<QString> tags)
{
	imageTagsCache.insert(imageFileName, tags);
}

bool ImageTags::cacheAddTagToImage(QString &imageFileName, QString &tagName)
{
	if (imageTagsCache[imageFileName].contains(tagName)) {
		return false;
	}
	
	imageTagsCache[imageFileName].insert(tagName);
	return true;
}

void ImageTags::cacheClear()
{
	imageTagsCache.clear();
}

