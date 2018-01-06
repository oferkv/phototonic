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

#include "Phototonic.h"
#include <ostream>
#include <QApplication>

using namespace std;

static void showHelp() {
    cout << VERSION << ", image viewer and organizer." << endl;
    cout << "Usage: phototonic [OPTION...] [FILE | DIRECTORY]" << endl << endl;
    cout << "  -h, --help\t\t\tshow this help" << endl;
    cout << "  -l, --lang=LANGUAGE\t\tuse a specific language" << endl;
    cout << endl << "Report bugs to oferkv@gmail.com" << endl;
}

int main(int argc, char *argv[]) {
    QApplication QApp(argc, argv);
    QStringList args = QCoreApplication::arguments();
    QString fileOrDirectory, language;

    if (args.size() == 2) {
        if (args.at(1)[0] == '-') {
            showHelp();
            return -1;
        }
        fileOrDirectory = args.at(1);
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

    QTranslator qTranslator;
    qTranslator.load("qt_" + language, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApp.installTranslator(&qTranslator);

    QTranslator qTranslatorPhototonic;
    qTranslatorPhototonic.load(":/translations/phototonic_" + language);
    QApp.installTranslator(&qTranslatorPhototonic);

    Phototonic phototonic(fileOrDirectory);
    phototonic.show();
    return QApp.exec();
}

