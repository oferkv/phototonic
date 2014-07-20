TEMPLATE = app
TARGET = phototonic
INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lexiv2
QT += widgets

# Input
HEADERS += dialogs.h mainwindow.h thumbview.h imageview.h global.h infoview.h
SOURCES += dialogs.cpp main.cpp mainwindow.cpp thumbview.cpp imageview.cpp global.cpp infoview.cpp
RESOURCES += phototonic.qrc

target.path = /usr/bin/

icon.files = images/phototonic.png
icon.path = /usr/share/pixmaps

desktop.files = phototonic.desktop
desktop.path = /usr/share/applications

INSTALLS += target icon desktop

TRANSLATIONS = 	translations/phototonic_pl.ts \
				translations/phototonic_de.ts

