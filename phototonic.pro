TEMPLATE = app
TARGET = phototonic
INCLUDEPATH += .

# Input
HEADERS += dialogs.h mainwindow.h thumbview.h imageview.h global.h
SOURCES += dialogs.cpp main.cpp mainwindow.cpp thumbview.cpp imageview.cpp global.cpp
RESOURCES += phototonic.qrc

target.path = /usr/bin/
INSTALLS += target

