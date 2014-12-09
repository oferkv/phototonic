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

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication QApp(argc, argv);
   	if (QCoreApplication::arguments().size() > 2)
   	{
		qDebug() << QObject::tr("Usage: phototonic [FILE or DIRECTORY]...");
   		return -1;
	}

	QTranslator qtTranslator;

	qtTranslator.load("qt_" + QLocale::system().name(),
					QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	QApp.installTranslator(&qtTranslator);

	QTranslator phototonicTranslator;
	phototonicTranslator.load(":/translations/phototonic_" + QLocale::system().name());
	QApp.installTranslator(&phototonicTranslator);

    Phototonic phototonic;
    phototonic.show();
    return QApp.exec();
}

