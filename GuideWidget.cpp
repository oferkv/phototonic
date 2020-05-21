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

#include "GuideWidget.h"
#include <QAction>
#include <QMouseEvent>
#include <QPainter>

static const int RuleWidgetWidth = 5; // determines the active area for dragging

QAction *GuideWidget::m_deleteAction(new QAction(GuideWidget::tr("Remove guide")));

GuideWidget::GuideWidget(QWidget *parent, bool vertical) : QWidget(parent), m_vertical(vertical)
{
    resizeToParent();
    setCursor(vertical ? Qt::SplitHCursor : Qt::SplitVCursor);
    addAction(m_deleteAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(m_deleteAction, &QAction::triggered, this, &QObject::deleteLater);
}

void GuideWidget::resizeToParent()
{
    if (m_vertical)
        resize(5, parentWidget()->height());
    else
        resize(parentWidget()->width(), 6);
}

int GuideWidget::halfThickness()
{
    return RuleWidgetWidth / 2;
}

void GuideWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->modifiers() != Qt::NoModifier) {
        QWidget::mouseMoveEvent(event);
        return; // don't interfere with crop selection etc.
    }
    if (m_vertical)
        move(x() + event->pos().x() - halfThickness(), 0);
    else
        move(0, y() + event->pos().y() - halfThickness());
}

void GuideWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::cyan);
    if (m_vertical)
        painter.drawLine(halfThickness(), 0, halfThickness(), height());
    else
        painter.drawLine(0, halfThickness(), width(), halfThickness());
}
