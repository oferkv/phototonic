TEMPLATE = app
TARGET = phototonic
INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lexiv2
QT += widgets

QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

# Input
HEADERS += dialogs.h mainwindow.h thumbview.h imageview.h global.h infoview.h
SOURCES += dialogs.cpp main.cpp mainwindow.cpp thumbview.cpp imageview.cpp global.cpp infoview.cpp
RESOURCES += phototonic.qrc

target.path = /usr/bin/

icon.files = images/phototonic.png
icon.path = /usr/share/icons/hicolor/48x48/apps

icon2.files = images/phototonic.png
icon2.path = /usr/share/pixmaps

desktop.files = phototonic.desktop
desktop.path = /usr/share/applications

INSTALLS += target icon icon2 desktop

TRANSLATIONS = 	translations/phototonic_pl.ts \
		translations/phototonic_de.ts \
		translations/phototonic_ru.ts \
		translations/phototonic_cs.ts 


