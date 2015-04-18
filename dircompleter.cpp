/*
 *  Copyright (C) 2015 Thomas Lübking <thomas.luebking@gmail.com>
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

#include <QDirModel>
#include "dircompleter.h"

DirCompleter::DirCompleter(QObject *parent) : QCompleter(parent)
{
	QDirModel *model = new QDirModel;
	model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	model->setLazyChildCount(true);
	setModel(model);
}

QString DirCompleter::pathFromIndex(const QModelIndex &index) const
{
	return QCompleter::pathFromIndex(index) + "/";
}

QStringList DirCompleter::splitPath(const QString &path) const
{
	if (path.startsWith("~")) {
		return QCompleter::splitPath(QString(path).replace(0, 1, QDir::homePath()));
	}
	return QCompleter::splitPath(path);
}
