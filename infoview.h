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

#ifndef INFOVIEW_H
#define INFOVIEW_H

#include <QtWidgets>

class InfoView : public QTableView
{
	Q_OBJECT

public:
	InfoView(QWidget *parent);
	void clear();
	void addEntry(QString &key, QString &value);
	void addTitleEntry(QString title);

public slots:
	void showInfoViewMenu(QPoint pt);
	void copyEntry();

private:
	QStandardItemModel *infoModel;
	QModelIndex selectedEntry;
	QMenu *infoMenu;
	QAction *copyAction;

};

#endif // INFOVIEW_H
