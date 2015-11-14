/*
 *  Copyright (C) 2013-2015 Ofer Kashayov <oferkv@live.com>
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
#include <ostream>
#include <QApplication>

static void showHelp()
{
	std::cout << VERSION << ", image viewer and organizer." << std::endl;
	std::cout << "Usage: phototonic [OPTION...] [FILE | DIRECTORY]" << std::endl << std::endl;
	std::cout << "  -h, --help\t\t\tshow this help" << std::endl;
	std::cout << "  -l, --lang=LANGUAGE\t\tuse a specific language" << std::endl;
 	std::cout << std::endl << "Report bugs to oferkv@gmail.com" << std::endl;
}

int main(int argc, char *argv[])
{
    QApplication QApp(argc, argv);
	QStringList args = QCoreApplication::arguments();
	QString fileOrDirectory, language;

	if (args.size() == 2) {
		if (args.at(1)[0] == '-') {
			showHelp();
	   		return -1;
		}
		fileOrDirectory	= args.at(1);
	} else if (args.size() == 3 || args.size() == 4) {
		if ((args.at(1) != "-l" && args.at(1) != "--lang")) {
			showHelp();
	   		return -1;
		}
		language = args.at(2);

		if (args.size() == 4) {
			fileOrDirectory = args.at(3);
		}
	}

	if (!language.size()) {
		language = QLocale::system().name();
	}

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + language, 	QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	QApp.installTranslator(&qtTranslator);

	QTranslator phototonicTranslator;
	phototonicTranslator.load(":/translations/phototonic_" + language);
	QApp.installTranslator(&phototonicTranslator);

    Phototonic phototonic(fileOrDirectory);
    phototonic.show();
    return QApp.exec();
}

