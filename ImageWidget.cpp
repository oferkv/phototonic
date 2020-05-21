/*
 *  Copyright (C) 2018 Shawn Rutledge <s@ecloud.org>
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

#include "ImageWidget.h"
#include <QDebug>
#include <QPainter>

ImageWidget::ImageWidget(QWidget *parent) : QWidget(parent)
{
}

bool ImageWidget::empty()
{
    return m_image.isNull();
}

QImage ImageWidget::image()
{
    return m_image;
}

void ImageWidget::setImage(const QImage &i)
{
    m_image = i;
    m_rotation = 0;
    update();
}

void ImageWidget::setRotation(qreal r)
{
    m_rotation = r;
    update();
}

QPoint ImageWidget::mapToImage(QPoint p)
{
    QPoint upperLeft;
    QPoint center(width() / 2, height() / 2);
    if (width() > m_image.width())
        upperLeft.setX(center.x() - m_image.width() / 2);
    if (height() > m_image.height())
        upperLeft.setY(center.y() - m_image.height() / 2);
    return QPoint(p.x() - upperLeft.x(), p.y() - upperLeft.y());
}

QSize ImageWidget::imageSize() const
{
    return m_image.size();
}

QSize ImageWidget::sizeHint() const
{
    return m_image.size();
}

void ImageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPoint center(width() / 2, height() / 2);
    painter.translate(center);
    painter.rotate(m_rotation);
    painter.translate(center * -1);
    QPoint upperLeft;
    if (width() > m_image.width())
        upperLeft.setX(center.x() - m_image.width() / 2);
    if (height() > m_image.height())
        upperLeft.setY(center.y() - m_image.height() / 2);
    painter.drawImage(upperLeft, m_image);
}
