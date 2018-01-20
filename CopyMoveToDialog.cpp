/*
 *  Copyright (C) 2013-2014 Ofer Kashayov - oferkv@live.com
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

#include "CopyMoveToDialog.h"
#include "Settings.h"

void CopyMoveToDialog::selection(const QItemSelection &, const QItemSelection &) {
    if (pathsTable->selectionModel()->selectedRows().size() > 0) {
        destinationLabel->setText(tr("Destination:") + " " +
                                  pathsTableModel->item(
                                          pathsTable->selectionModel()->selectedRows().at(0).row())->text());
    }
}

void CopyMoveToDialog::pathDoubleClick(const QModelIndex &) {
    copyOrMove();
}

void CopyMoveToDialog::savePaths() {
    Settings::bookmarkPaths.clear();
    for (int i = 0; i < pathsTableModel->rowCount(); ++i) {
        Settings::bookmarkPaths.insert
                (pathsTableModel->itemFromIndex(pathsTableModel->index(i, 0))->text());
    }
}

void CopyMoveToDialog::copyOrMove() {
    savePaths();

    QModelIndexList indexesList;
    if ((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
        selectedPath = pathsTableModel->itemFromIndex(indexesList.first())->text();
        accept();
    } else {
        reject();
    }
}

void CopyMoveToDialog::justClose() {
    savePaths();
    reject();
}

void CopyMoveToDialog::add() {
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Directory"), currentPath,
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dirName.isEmpty()) {
        return;
    }

    QStandardItem *item = new QStandardItem(QIcon(":/images/bookmarks.png"), dirName);
    pathsTableModel->insertRow(pathsTableModel->rowCount(), item);

    pathsTable->selectionModel()->clearSelection();
    pathsTable->selectionModel()->select(pathsTableModel->index(pathsTableModel->rowCount() - 1, 0),
                                         QItemSelectionModel::Select);
}

void CopyMoveToDialog::remove() {
    QModelIndexList indexesList;
    if ((indexesList = pathsTable->selectionModel()->selectedIndexes()).size()) {
        pathsTableModel->removeRow(indexesList.first().row());
    }
}

CopyMoveToDialog::CopyMoveToDialog(QWidget *parent, QString thumbsPath, bool move) : QDialog(parent) {
    copyOp = !move;
    if (move) {
        setWindowTitle(tr("Move to..."));
        setWindowIcon(QIcon::fromTheme("go-next"));
    } else {
        setWindowTitle(tr("Copy to..."));
        setWindowIcon(QIcon::fromTheme("edit-copy"));
    }

    resize(350, 250);
    currentPath = thumbsPath;

    pathsTable = new QTableView(this);
    pathsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    pathsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pathsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pathsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pathsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    pathsTableModel = new QStandardItemModel(this);
    pathsTable->setModel(pathsTableModel);
    pathsTable->verticalHeader()->setVisible(false);
    pathsTable->horizontalHeader()->setVisible(false);
    pathsTable->verticalHeader()->setDefaultSectionSize(pathsTable->verticalHeader()->
            minimumSectionSize());
    pathsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    pathsTable->setShowGrid(false);

    connect(pathsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(selection(QItemSelection, QItemSelection)));
    connect(pathsTable, SIGNAL(doubleClicked(
                                       const QModelIndex &)),
            this, SLOT(pathDoubleClick(
                               const QModelIndex &)));

    QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Browse..."));
    connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
    QPushButton *removeButton = new QPushButton(tr("Remove"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
    addRemoveHbox->addWidget(removeButton, 0, Qt::AlignLeft);
    addRemoveHbox->addStretch(1);
    addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);

    QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(justClose()));

    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    okButton->setDefault(true);

    connect(okButton, SIGNAL(clicked()), this, SLOT(copyOrMove()));

    buttonsHbox->addStretch(1);
    buttonsHbox->addWidget(cancelButton, 0, Qt::AlignRight);
    buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

    destinationLabel = new QLabel(tr("Destination:"));
    QFrame *line = new QFrame(this);
    line->setObjectName(QString::fromUtf8("line"));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *mainVbox = new QVBoxLayout;
    mainVbox->addWidget(pathsTable);
    mainVbox->addLayout(addRemoveHbox);
    mainVbox->addWidget(line);
    mainVbox->addWidget(destinationLabel);
    mainVbox->addLayout(buttonsHbox);
    setLayout(mainVbox);

    // Load paths list
    QSetIterator<QString> it(Settings::bookmarkPaths);
    while (it.hasNext()) {
        QStandardItem *item = new QStandardItem(QIcon(":/images/bookmarks.png"), it.next());
        pathsTableModel->insertRow(pathsTableModel->rowCount(), item);
    }
    pathsTableModel->sort(0);
}
