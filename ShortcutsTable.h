/*
 *  Copyright (C) 2018 Ofer Kashayov <oferkv@live.com>
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

#ifndef SHORTCUTS_TABLE_H
#define SHORTCUTS_TABLE_H

#include <QtWidgets>

class ShortcutsTable : public QTableView {
    Q_OBJECT

public:
    ShortcutsTable();

    void addRow(QString action, QString description, QString shortcut);

    void refreshShortcuts();

public slots:
    void setFilter(QString filter);

    void showShortcutPopupMenu(QPoint point);

    void clearSelectedShortcut();

protected:
    void keyPressEvent(QKeyEvent *keyEvent);

private:
    bool confirmOverwriteShortcut(QString action, QString shortcut);

    QStandardItemModel *keysModel;
    QModelIndex selectedEntry;
    QMenu *shortcutsMenu;
    QAction *clearAction;

    QString shortcutsFilter;
};

#endif // SHORTCUTS_TABLE_H
