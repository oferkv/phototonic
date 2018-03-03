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

#include "MessageBox.h"

MessageBox::MessageBox(QWidget *parent) : QMessageBox(parent) {
    setWindowIcon(QIcon(":/images/phototonic.png"));
}

void MessageBox::critical(const QString &title, const QString &message) {
    setWindowTitle(title);
    setText(message);
    setIcon(MessageBox::Critical);
    exec();
}

void MessageBox::warning(const QString &title, const QString &message) {
    setWindowTitle(title);
    setText(message);
    setIcon(MessageBox::Warning);
    exec();
}

void MessageBox::about(const QString &aboutMessage) {
    setWindowTitle(tr("About"));
    setText(aboutMessage);
    setIconPixmap(QIcon(":/images/phototonic.png").pixmap(64, 64));
    exec();
}
