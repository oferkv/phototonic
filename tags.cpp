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

ImageTags::ImageTags(QWidget *parent, ThumbView *thumbView) : QWidget(parent)
{
	tagsTree = new QTreeWidget;
	tagsTree->setColumnCount(2);
	tagsTree->setDragEnabled(false);
	tagsTree->setSortingEnabled(true);
	tagsTree->header()->close();
	tagsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
	this->thumbView = thumbView;

	tabs = new QTabBar(this);
	tabs->addTab("Set Tags");
  	tabs->addTab("Filter");
  	tabs->setTabIcon(0, QIcon(":/images/tag_blue.png"));
  	tabs->setTabIcon(1, QIcon(":/images/filter_off.png"));
	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(tabsChanged(int)));

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 3, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(tabs);
	mainLayout->addWidget(tagsTree);
	setLayout(mainLayout);
	currentDisplayMode = SelectionTagsDisplay;
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

	addTagAction = new QAction(tr("New Tag"), this);
	addTagAction->setIcon(QIcon(":/images/new_tag.png"));
	connect(addTagAction, SIGNAL(triggered()), this, SLOT(addNewTag()));

	removeTagAction = new QAction(tr("Remove Tag"), this);
	removeTagAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

	clearTagsFilterAction = new QAction(tr("Clear Filters"), this);
	clearTagsFilterAction->setIcon(QIcon::fromTheme("edit-clear"));
	connect(clearTagsFilterAction, SIGNAL(triggered()), this, SLOT(clearTagFilters()));

	tagsMenu = new QMenu("");
	tagsMenu->addAction(addToSelectionAction);
	tagsMenu->addAction(removeFromSelectionAction);
	tagsMenu->addSeparator();
	tagsMenu->addAction(addTagAction);
	tagsMenu->addAction(removeTagAction);
	tagsMenu->addSeparator();
	tagsMenu->addAction(clearTagsFilterAction);
}

void ImageTags::redrawTree()
{
	tagsTree->resizeColumnToContents(0);
	tagsTree->sortItems(0, Qt::AscendingOrder);
}

void ImageTags::showMenu(QPoint pt)
{
    QTreeWidgetItem *item = tagsTree->itemAt(pt);
	addToSelectionAction->setEnabled(item? true : false);
	removeFromSelectionAction->setEnabled(item? true : false);
	removeTagAction->setEnabled(item? true : false);
    tagsMenu->popup(tagsTree->viewport()->mapToGlobal(pt));
}

void ImageTags::setTagIcon(QTreeWidgetItem *tagItem, Qt::CheckState status)
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
	setTagIcon(tagItem, tagChecked? Qt::Checked : Qt::Unchecked);
	tagsTree->addTopLevelItem(tagItem);
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

void ImageTags::showSelectedImagesTags()
{
	static bool busy = false;
	if (busy)
		return;
	busy = true;
	QStringList selectedThumbs = thumbView->getSelectedThumbsList();

	setActiveViewMode(SelectionTagsDisplay);

	int selectedThumbsNum = selectedThumbs.size();
	QMap<QString, int> tagsCount;
	for (int i = 0; i < selectedThumbsNum; ++i) {
		QSetIterator<QString> imageTagsIter(cacheGetImageTags(selectedThumbs[i]));
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
    while (*it) {
    	QString tagName = (*it)->text(0);
    	int tagCountTotal = tagsCount[tagName];

        if (selectedThumbsNum == 0) {
        	(*it)->setCheckState(0, Qt::Unchecked);
        	(*it)->setFlags((*it)->flags() & ~Qt::ItemIsUserCheckable);
        	setTagIcon(*it, Qt::Unchecked);
		} else if (tagCountTotal ==  selectedThumbsNum) {
        	(*it)->setCheckState(0, Qt::Checked);
			(*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
        	setTagIcon(*it, Qt::Checked);
       	} else if (tagCountTotal) {
			(*it)->setCheckState(0, Qt::PartiallyChecked);
			(*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
        	setTagIcon(*it, Qt::PartiallyChecked);
		} else {
        	(*it)->setCheckState(0, Qt::Unchecked);
			(*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
        	setTagIcon(*it, Qt::Unchecked);
		}
        ++it;
    }

	addToSelectionAction->setEnabled(selectedThumbsNum? true : false);
	removeFromSelectionAction->setEnabled(selectedThumbsNum? true : false);

	redrawTree();
   	busy = false;
}

void ImageTags::showTagsFilter()
{
	static bool busy = false;
	if (busy)
		return;
	busy = true;

	setActiveViewMode(FolderTagsDisplay);

	QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
    	QString tagName = (*it)->text(0);

		(*it)->setFlags((*it)->flags() | Qt::ItemIsUserCheckable);
        if (imageFilteringTags.contains(tagName)) {
        	(*it)->setCheckState(0, Qt::Checked);
        	setTagIcon(*it, Qt::Checked);
		} else {
        	(*it)->setCheckState(0, Qt::Unchecked);
        	setTagIcon(*it, Qt::Unchecked);
       	} 
        ++it;
    }

	redrawTree();
	busy = false;
}

void ImageTags::populateTagsTree()
{
	tagsTree->clear();
	QSetIterator<QString> knownTagsIt(GData::knownTags);
	while (knownTagsIt.hasNext()) {
	    QString tag = knownTagsIt.next();
        addTag(tag, false);
    }

	redrawTree();
}

void ImageTags::setActiveViewMode(TagsDisplayMode mode)
{
	currentDisplayMode = mode;
	addTagAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
	removeTagAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
	addToSelectionAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
	removeFromSelectionAction->setVisible(currentDisplayMode == SelectionTagsDisplay);
	clearTagsFilterAction->setVisible(currentDisplayMode == FolderTagsDisplay);
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

void ImageTags::resetTagsState()
{
	tagsTree->clear();
	cacheClear();
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
	 	tabs->setTabIcon(1, QIcon(":/images/filter_on.png"));
	} else {
		folderFilteringActive = false;
	 	tabs->setTabIcon(1, QIcon(":/images/filter_off.png"));
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

	QStringList currentSelectedImages = thumbView->getSelectedThumbsList();
	for (int i = 0; i < currentSelectedImages.size(); ++i) {

		QString imageName = currentSelectedImages[i]; 
		for (int i = tagsList.size() - 1; i > -1; --i) {
			Qt::CheckState tagState = tagsList.at(i)->checkState(0);
			setTagIcon(tagsList.at(i), tagState);
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

void ImageTags::tabsChanged(int index)
{
	if (!index) {
		showSelectedImagesTags();
	} else {
		showTagsFilter();
	}
}

void ImageTags::tagClicked(QTreeWidgetItem *item, int)
{
	if (item == lastChangedTagItem) {
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
		tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Unchecked);
	}

	applyUserAction(tagsTree->selectedItems());
}

void ImageTags::addTagsToSelection()
{	
	for (int i = tagsTree->selectedItems().size() - 1; i > -1; --i) {
		tagsTree->selectedItems().at(i)->setCheckState(0, Qt::Checked);
	}

	applyUserAction(tagsTree->selectedItems());
}

void ImageTags::clearTagFilters()
{	
    QTreeWidgetItemIterator it(tagsTree);
    while (*it) {
        (*it)->setCheckState(0, Qt::Unchecked);
        ++it;
    }

	imageFilteringTags.clear();
	applyTagFiltering();
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

	QSetIterator<QString> knownTagsIt(GData::knownTags);
	while (knownTagsIt.hasNext()) {
	    QString tag = knownTagsIt.next();
	    if (newTagName == tag) {
			QMessageBox msgBox;
			msgBox.critical(this, tr("Error"), tr("Tag ") + newTagName + tr(" already exists"));
			return;
        }
    }

	addTag(newTagName, false);
	GData::knownTags.insert(newTagName);
	redrawTree();
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

		QString tagName = tagsTree->selectedItems().at(i)->text(0);
		GData::knownTags.remove(tagName);

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

