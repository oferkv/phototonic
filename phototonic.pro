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
HEADERS += Dialogs.h Phototonic.h ThumbsViewer.h ImageViewer.h CropRubberband.h Settings.h InfoViewer.h \
			FileSystemTree.h Bookmarks.h DirCompleter.h Tags.h MetadataCache.h ShortcutsTable.h
SOURCES += Dialogs.cpp main.cpp Phototonic.cpp ThumbsViewer.cpp ImageViewer.cpp CropRubberband.cpp \
			Settings.cpp InfoViewer.cpp FileSystemTree.cpp Bookmarks.cpp DirCompleter.cpp Tags.cpp \
			MetadataCache.cpp ShortcutsTable.cpp
RESOURCES += phototonic.qrc

target.path = /usr/bin/

icon.files = images/phototonic.png
icon.path = /usr/share/icons/hicolor/48x48/apps

icon16.files = images/icon16/phototonic.png
icon16.path = /usr/share/icons/hicolor/16x16/apps

iconPixmaps.files = images/icon16/phototonic.png
iconPixmaps.path = /usr/share/pixmaps

desktop.files = phototonic.desktop
desktop.path = /usr/share/applications

INSTALLS += target icon icon16 iconPixmaps desktop

TRANSLATIONS = 	translations/phototonic_en.ts \
		translations/phototonic_pl.ts \
		translations/phototonic_de.ts \
		translations/phototonic_ru.ts \
		translations/phototonic_cs.ts \
		translations/phototonic_fr.ts \
		translations/phototonic_bs.ts \
		translations/phototonic_hr.ts \
		translations/phototonic_sr.ts

