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
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    QApplication QApp(argc, argv);
    QLocale locale = QLocale::system();
    QCoreApplication::setApplicationVersion(VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(VERSION " image viewer.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QCoreApplication::translate("main", "files or directory"),
                                 QCoreApplication::translate("main", "files or directory to open"),
                                 QCoreApplication::translate("main", "[FILE...] | [DIRECTORY]"));

    QCommandLineOption langOption(QStringList() << "l" << "lang",
                                             QCoreApplication::translate("main", "start with a specific translation"),
                                             QCoreApplication::translate("main", "language"));
    parser.addOption(langOption);

    QCommandLineOption targetDirectoryOption(QStringList() << "o" << "output-directory",
            QCoreApplication::translate("main", "Copy all modified images into <directory>."),
            QCoreApplication::translate("main", "directory"));
    parser.addOption(targetDirectoryOption);

    parser.process(QApp);

    if (parser.isSet(langOption))
        locale = QLocale(parser.value(langOption));

    QTranslator qTranslator;
    qTranslator.load(locale, "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApp.installTranslator(&qTranslator);

    QTranslator qTranslatorPhototonic;
    qTranslatorPhototonic.load(locale, "phototonic", "_", ":/translations");
    QApp.installTranslator(&qTranslatorPhototonic);

    Phototonic phototonic(parser.positionalArguments(), 0);
    if (parser.isSet(targetDirectoryOption))
        phototonic.setSaveDirectory(parser.value(targetDirectoryOption));
    phototonic.show();
    return QApp.exec();
}
