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
#include <QApplication>

static void showHelp() {
    qInfo() << VERSION << "image viewer.";
    qInfo() << "Usage: phototonic [OPTION] [FILE... | DIRECTORY]";
    qInfo() << "  -h, --help\t\t\tshow this help and exit";
    qInfo() << "  -l, --lang=LANGUAGE\t\tstart with a specific translation";
}

int main(int argc, char *argv[]) {
    QApplication QApp(argc, argv);
    QStringList arguments = QCoreApplication::arguments();
    QString language;
    int argumentsStartAt = 1;

    if (arguments.size() == 2) {
        if (arguments.at(1)[0] == '-') {
            showHelp();
            return -1;
        }
    } else if (arguments.size() > 3) {
        if ((arguments.at(1) != "-l" && arguments.at(1) != "--lang")) {
            showHelp();
            return -1;
        }
        language = arguments.at(2);
        argumentsStartAt = 3;
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

    Phototonic phototonic(arguments, argumentsStartAt);
    phototonic.show();
    return QApp.exec();
}

