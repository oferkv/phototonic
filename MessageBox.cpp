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
#include "Phototonic.h"

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

void MessageBox::about() {
    QString aboutString = "<h2>" + QString(VERSION) + "</h2>"
                          + tr("<h4>Image Viewer and Organizer</h4>")
                          + "Qt v" + QT_VERSION_STR
                          + "<p><a href=\"https://github.com/oferkv/phototonic\">" + tr("Home page")
                          + "</a></p></a></p><p></p>"
                                  "<table><tr><td>Code:</td><td>Ofer Kashayov</td><td>(oferkv@gmail.com)</td></tr>"
                                  "<tr><td></td><td>Christopher Roy Bratusek</td><td>(nano@jpberlin.de)</td></tr>"
                                  "<tr><td></td><td>Krzysztof Pyrkosz</td><td>(pyrkosz@o2.pl)</td></tr>"
                                  "<tr><td></td><td>Roman Chistokhodov</td><td>(freeslave93@gmail.com)</td></tr>"
                                  "<tr><td></td><td>Thomas Lübking</td><td>(thomas.luebking@gmail.com)</td></tr>"
                                  "<tr><td></td><td>Tung Le</td><td>(https://github.com/everbot)</td></tr>"
                                  "<tr><td></td><td>Peter Mattern</td><td>(https://github.com/pmattern)</td></tr>"
                                  "<tr><td>Bosnian:</td><td>Dino Duratović</td><td>(dinomol@mail.com)</td></tr>"
                                  "<tr><td>Croatian:</td><td>Dino Duratović</td><td>(dinomol@mail.com)</td></tr>"
                                  "<tr><td>Czech:</td><td>Pavel Fric</td><td>(pavelfric@seznam.cz)</td></tr>"
                                  "<tr><td>French:</td><td>Adrien Daugabel</td><td>(adrien.d@mageialinux-online.org)</td></tr>"
                                  "<tr><td></td><td>David Geiger</td><td>(david.david@mageialinux-online.org)</td></tr>"
                                  "<tr><td></td><td>Rémi Verschelde</td><td>(akien@mageia.org)</td></tr>"
                                  "<tr><td>German:</td><td>Jonathan Hooverman</td><td>(jonathan.hooverman@gmail.com)</td></tr>"
                                  "<tr><td>Polish:</td><td>Robert Wojewódzki</td><td>(robwoj44@poczta.onet.pl)</td></tr>"
                                  "<tr><td></td><td>Krzysztof Pyrkosz</td><td>(pyrkosz@o2.pl)</td></tr>"
                                  "<tr><td>Portuguese:</td><td>Marcos M. Nascimento</td><td>(wstlmn@uol.com.br)</td></tr>"
                                  "<tr><td>Russian:</td><td>Ilya Alexandrovich</td><td>(yast4ik@gmail.com)</td></tr>"
                                  "<tr><td>Serbian:</td><td>Dino Duratović</td><td>(dinomol@mail.com)</td></tr></table>"
                                  "<p>Phototonic is licensed under the GNU General Public License version 3</p>"
                                  "<p>Copyright &copy; 2013-2018 Ofer Kashayov</p>";

    setWindowTitle(tr("About"));
    setText(aboutString);
    setIconPixmap(QIcon(":/images/phototonic.png").pixmap(64, 64));
    exec();
}
