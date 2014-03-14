TEMPLATE = app
TARGET = phototonic
INCLUDEPATH += .

# Input
HEADERS += dialogs.h mainwindow.h thumbview.h imageview.h global.h
SOURCES += dialogs.cpp main.cpp mainwindow.cpp thumbview.cpp imageview.cpp global.cpp
RESOURCES += phototonic.qrc

target.path = /usr/bin/

icon.files = images/phototonic.png
icon.path = /usr/share/pixmaps

desktop.files = phototonic.desktop
desktop.path = /usr/share/applications

INSTALLS += target icon desktop
