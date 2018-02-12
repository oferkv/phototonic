/*
 *  Copyright (C) 2018 Roman Chistokhodov <freeslave93@gmail.com>
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

#ifndef TRASHCAN_H
#define TRASHCAN_H

#include <QString>

namespace Trash {
    typedef enum
    {
        Success,
        Error,
        NeedsUserInput
    } Result;

    typedef enum
    {
        NoOptions = 0,
        ForceDeletionToHomeTrash = 1
    } Options;

    Trash::Result moveToTrash(const QString &filePath, QString &error, Options trashOptions = NoOptions);
}

#endif // TRASHCAN_H
