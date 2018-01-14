/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
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

#ifndef IMAGE_PREVIEW_H
#define IMAGE_PREVIEW_H

#include <QtWidgets>

class ImagePreview : public QWidget {
Q_OBJECT

public:
    ImagePreview(QWidget *parent);

    void loadImage(QString imageFileName);

    void resizeImagePreview();

    void setBackgroundColor();

    void clear();

    QScrollArea *scrollArea;

protected:
    void resizeEvent(QResizeEvent *event);

private:
    QLabel *imageLabel;
    QImageReader imageReader;
    QPixmap previewPixmap;

};

#endif // IMAGE_PREVIEW_H
