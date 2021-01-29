/*
 *  Copyright (C) 2013-2018 Ofer Kashayov <oferkv@live.com>
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

#include "FileSystemModel.h"
#include "IconProvider.h"

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
    m_iconProvider = new IconProvider;
    setIconProvider(m_iconProvider);
}

FileSystemModel::~FileSystemModel()
{
    delete m_iconProvider;
}

bool FileSystemModel::hasChildren(const QModelIndex &parent) const {
    if (parent.column() > 0) {
        return false;
    }

    if (!parent.isValid()) {
        return true;
    }

    if (parent.flags() & Qt::ItemNeverHasChildren) {
        return false;
    }

    return QDirIterator(filePath(parent), filter() | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags).hasNext();
}
