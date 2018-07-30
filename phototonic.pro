#
#  Copyright (C) 2013-2018 Ofer Kashayov <oferkv@live.com>
#  This file is part of Phototonic Image Viewer.
#
#  Phototonic is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Phototonic is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = app
TARGET = phototonic
INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
win32-g++ {
MINGWEXIVPATH = $$PWD/mingw

LIBS += -L$$MINGWEXIVPATH/lib/ -lexiv2 -lexpat -lz

INCLUDEPATH += $$MINGWEXIVPATH/include
DEPENDPATH += $$MINGWEXIVPATH/include

PRE_TARGETDEPS += $$MINGWEXIVPATH/lib/libexiv2.a $$MINGWEXIVPATH/lib/libexpat.a $$MINGWEXIVPATH/lib/libz.a
}
else: LIBS += -L/usr/local/lib -lexiv2
QT += widgets
QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)
CONFIG += c++11

HEADERS += Phototonic.h ThumbsViewer.h ImageViewer.h CropRubberband.h SettingsDialog.h Settings.h InfoViewer.h \
			FileSystemTree.h Bookmarks.h DirCompleter.h Tags.h MetadataCache.h ShortcutsTable.h CopyMoveDialog.h \
			CopyMoveToDialog.h CropDialog.h ProgressDialog.h ColorsDialog.h ResizeDialog.h ExternalAppsDialog.h \
			ImagePreview.h ImageWidget.h FileSystemModel.h FileListWidget.h RenameDialog.h Trashcan.h MessageBox.h \
			GuideWidget.h

SOURCES += main.cpp Phototonic.cpp ThumbsViewer.cpp ImageViewer.cpp CropRubberband.cpp SettingsDialog.cpp \
			Settings.cpp InfoViewer.cpp FileSystemTree.cpp Bookmarks.cpp DirCompleter.cpp Tags.cpp \
			MetadataCache.cpp ShortcutsTable.cpp CopyMoveDialog.cpp CopyMoveToDialog.cpp CropDialog.cpp \
			ProgressDialog.cpp ExternalAppsDialog.cpp ColorsDialog.cpp ResizeDialog.cpp ImagePreview.cpp \
			ImageWidget.cpp FileSystemModel.cpp FileListWidget.cpp RenameDialog.cpp Trashcan.cpp MessageBox.cpp \
			GuideWidget.cpp

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
		translations/phototonic_sr.ts \
		translations/phototonic_pt.ts
