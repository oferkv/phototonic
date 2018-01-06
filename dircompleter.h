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

#ifndef DIRCOMPLETER_H
#define DIRCOMPLETER_H

#include <QCompleter>

class DirCompleter : public QCompleter {
Q_OBJECT
public:
    DirCompleter(QObject *parent = 0);

    QString pathFromIndex(const QModelIndex &index) const;

public slots:

    QStringList splitPath(const QString &path) const;
};

#endif // DIRCOMPLETER_H
