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

#ifndef COPY_MOVE_TO_DIALOG_H
#define COPY_MOVE_TO_DIALOG_H

#include <QtWidgets/qdialog.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

class CopyMoveToDialog : public QDialog {
Q_OBJECT

public:
    CopyMoveToDialog(QWidget *parent, QString thumbsPath, bool move);

    QString selectedPath;
    bool copyOp;

private slots:

    void copyOrMove();

    void justClose();

    void add();

    void remove();

    void selection(const QItemSelection &, const QItemSelection &);

    void pathDoubleClick(const QModelIndex &idx);

private:
    QTableView *pathsTable;
    QStandardItemModel *pathsTableModel;
    QString currentPath;
    QLabel *destinationLabel;

    void savePaths();
};

#endif // COPY_MOVE_TO_DIALOG_H