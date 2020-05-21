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

#include "InfoViewer.h"
#include "ThumbsViewer.h"

InfoView::InfoView(QWidget *parent) : QWidget(parent) {

    infoViewerTable = new QTableView();
    infoViewerTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    infoViewerTable->verticalHeader()->setVisible(false);
    infoViewerTable->verticalHeader()->setDefaultSectionSize(infoViewerTable->verticalHeader()->minimumSectionSize());
    infoViewerTable->horizontalHeader()->setVisible(false);
    infoViewerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    infoViewerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    infoViewerTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    infoViewerTable->setTabKeyNavigation(false);
    infoViewerTable->setShowGrid(false);

    imageInfoModel = new QStandardItemModel(this);
    infoViewerTable->setModel(imageInfoModel);

    // Menu
    QAction *copyAction = new QAction(tr("Copy"), this);
    infoViewerTable->connect(copyAction, SIGNAL(triggered()), this, SLOT(copyEntry()));
    infoMenu = new QMenu("");
    infoMenu->addAction(copyAction);
    infoViewerTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(infoViewerTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showInfoViewMenu(QPoint)));

    QVBoxLayout *infoViewerLayout = new QVBoxLayout;

    // Filter items
    filterLineEdit = new QLineEdit;
    connect(filterLineEdit, SIGNAL(textChanged(
                                           const QString&)), this, SLOT(filterItems()));
    filterLineEdit->setClearButtonEnabled(true);
    filterLineEdit->setPlaceholderText(tr("Filter Items"));
    infoViewerLayout->addWidget(filterLineEdit);

    infoViewerLayout->addWidget(infoViewerTable);
    infoViewerLayout->setContentsMargins(2, 2, 2, 2);
    infoViewerLayout->setSpacing(2);

    setLayout(infoViewerLayout);
}

void InfoView::showInfoViewMenu(QPoint pt) {
    selectedEntry = infoViewerTable->indexAt(pt);
    if (selectedEntry.isValid()) {
        infoMenu->popup(infoViewerTable->viewport()->mapToGlobal(pt));
    }
}

void InfoView::clear() {
    imageInfoModel->clear();
}

void InfoView::addEntry(QString &key, QString &value) {
    if (!filterLineEdit->text().isEmpty() && !key.toLower().contains(filterLineEdit->text().toLower())) {
        return;
    }

    int atRow = imageInfoModel->rowCount();
    QStandardItem *itemKey = new QStandardItem(key);
    imageInfoModel->insertRow(atRow, itemKey);
    if (!value.isEmpty()) {
        QStandardItem *itemVal = new QStandardItem(value);
        itemVal->setToolTip(value);
        imageInfoModel->setItem(atRow, 1, itemVal);
    }
}

void InfoView::addTitleEntry(QString title) {
    int atRow = imageInfoModel->rowCount();
    QStandardItem *itemKey = new QStandardItem(title);
    imageInfoModel->insertRow(atRow, itemKey);

    QFont boldFont;
    boldFont.setBold(true);
    itemKey->setData(boldFont, Qt::FontRole);
}

void InfoView::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);

    emit updateInfo();
}

void InfoView::copyEntry() {
    if (selectedEntry.isValid()) {
        QApplication::clipboard()->setText(imageInfoModel->itemFromIndex(selectedEntry)->toolTip());
    }
}

void InfoView::filterItems() {
    emit updateInfo();
}
