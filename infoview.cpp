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

#include "infoview.h"

InfoView::InfoView(QWidget *parent) : QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	verticalHeader()->setVisible(false);
	verticalHeader()->setDefaultSectionSize(verticalHeader()->minimumSectionSize());
	horizontalHeader()->setVisible(false);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	setShowGrid(false);

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setTabKeyNavigation(false);

	infoModel = new QStandardItemModel(this);
	setModel(infoModel);

	// InfoView menu
	infoMenu = new QMenu("");
	copyAction = new QAction(tr("Copy"), this);
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyEntry()));
	infoMenu->addAction(copyAction);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showInfoViewMenu(QPoint)));
}

void InfoView::showInfoViewMenu(QPoint pt)
{
    selectedEntry = indexAt(pt);
	if (selectedEntry.isValid())
    	infoMenu->popup(viewport()->mapToGlobal(pt));
}
 
void InfoView::clear()
{
	infoModel->clear();
}

void InfoView::addEntry(QString &key, QString &value)
{
	int atRow = infoModel->rowCount();
	QStandardItem *itemKey = new QStandardItem(key);
	infoModel->insertRow(atRow, itemKey);
	if (!value.isEmpty()) {
		QStandardItem *itemVal = new QStandardItem(value);
		itemVal->setToolTip(value);
		infoModel->setItem(atRow, 1, itemVal);
	}
}

void InfoView::addTitleEntry(QString title)
{
	int atRow = infoModel->rowCount();
	QStandardItem *itemKey = new QStandardItem(title);
	infoModel->insertRow(atRow, itemKey);

	QFont boldFont;
	boldFont.setBold(true);
    itemKey->setData(boldFont, Qt::FontRole);
}

void InfoView::copyEntry()
{
	if (selectedEntry.isValid())
		QApplication::clipboard()->setText(infoModel->itemFromIndex(selectedEntry)->toolTip());
}

