/*
 *  Copyright (C) 2013-2014 Ofer Kashayov - oferkv@live.com
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

#include "ImageViewer.h"
#include "Phototonic.h"
#include "MessageBox.h"

#define CLIPBOARD_IMAGE_NAME "clipboard.png"
#define ROUND(x) ((int) ((x) + 0.5))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

ImageViewer::ImageViewer(QWidget *parent, MetadataCache *metadataCache) : QWidget(parent) {
    this->phototonic = (Phototonic *) parent;
    this->metadataCache = metadataCache;
    cursorIsHidden = false;
    moveImageLocked = false;
    mirrorLayout = LayNone;
    imageLabel = new QLabel;
    imageLabel->setScaledContents(true);
    isAnimation = false;
    animation = nullptr;

    scrollArea = new QScrollArea;
    scrollArea->setContentsMargins(0, 0, 0, 0);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->verticalScrollBar()->blockSignals(true);
    scrollArea->horizontalScrollBar()->blockSignals(true);
    scrollArea->setFrameStyle(0);
    scrollArea->setWidget(imageLabel);
    scrollArea->setWidgetResizable(true);
    setBackgroundColor();

    QVBoxLayout *scrollLayout = new QVBoxLayout;
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(scrollArea);
    this->setLayout(scrollLayout);

    imageInfoLabel = new QLabel(this);
    imageInfoLabel->setVisible(Settings::showImageName);
    imageInfoLabel->setMargin(3);
    imageInfoLabel->move(10, 10);
    imageInfoLabel->setStyleSheet("QLabel { background-color : black; color : white; border-radius: 3px} ");

    feedbackLabel = new QLabel(this);
    feedbackLabel->setVisible(false);
    feedbackLabel->setMargin(3);
    feedbackLabel->setStyleSheet("QLabel { background-color : black; color : white; border-radius: 3px} ");

    QGraphicsOpacityEffect *infoEffect = new QGraphicsOpacityEffect;
    infoEffect->setOpacity(0.5);
    imageInfoLabel->setGraphicsEffect(infoEffect);
    QGraphicsOpacityEffect *feedbackEffect = new QGraphicsOpacityEffect;
    feedbackEffect->setOpacity(0.5);
    feedbackLabel->setGraphicsEffect(feedbackEffect);

    mouseMovementTimer = new QTimer(this);
    connect(mouseMovementTimer, SIGNAL(timeout()), this, SLOT(monitorCursorState()));

    Settings::cropLeft = Settings::cropTop = Settings::cropWidth = Settings::cropHeight = 0;
    Settings::cropLeftPercent = Settings::cropTopPercent = Settings::cropWidthPercent = Settings::cropHeightPercent = 0;

    Settings::hueVal = 0;
    Settings::saturationVal = 100;
    Settings::lightnessVal = 100;
    Settings::hueRedChannel = true;
    Settings::hueGreenChannel = true;
    Settings::hueBlueChannel = true;

    Settings::contrastVal = 78;
    Settings::brightVal = 100;

    Settings::dialogLastX = Settings::dialogLastY = 0;

    newImage = false;
    cropRubberBand = 0;
}

static unsigned int getHeightByWidth(int imgWidth, int imgHeight, int newWidth) {
    float aspect;
    aspect = (float) imgWidth / (float) newWidth;
    return (imgHeight / aspect);
}

static unsigned int getWidthByHeight(int imgHeight, int imgWidth, int newHeight) {
    float aspect;
    aspect = (float) imgHeight / (float) newHeight;
    return (imgWidth / aspect);
}

static inline int calcZoom(int size) {
    return size * Settings::imageZoomFactor;
}

void ImageViewer::resizeImage() {
    static bool busy = false;
    if (busy || (!imageLabel->pixmap() && !animation)) {
        return;
    }
    busy = true;

    int imageViewWidth = this->size().width();
    int imageViewHeight = this->size().height();
    QSize imageSize = isAnimation ? animation->currentPixmap().size() : imageLabel->pixmap()->size();

    if (tempDisableResize) {
        imageSize.scale(imageSize.width(), imageSize.height(), Qt::KeepAspectRatio);
    } else {
        switch (Settings::zoomInFlags) {
            case Disable:
                if (imageSize.width() <= imageViewWidth && imageSize.height() <= imageViewHeight) {
                    imageSize.scale(calcZoom(imageSize.width()),
                                    calcZoom(imageSize.height()),
                                    Qt::KeepAspectRatio);
                }
                break;

            case WidthAndHeight:
                if (imageSize.width() <= imageViewWidth && imageSize.height() <= imageViewHeight) {
                    imageSize.scale(calcZoom(imageViewWidth),
                                    calcZoom(imageViewHeight),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Width:
                if (imageSize.width() <= imageViewWidth) {
                    imageSize.scale(calcZoom(imageViewWidth),
                                    calcZoom(getHeightByWidth(imageSize.width(),
                                                              imageSize.height(),
                                                              imageViewWidth)),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Height:
                if (imageSize.height() <= imageViewHeight) {
                    imageSize.scale(calcZoom(getWidthByHeight(imageSize.height(),
                                                              imageSize.width(),
                                                              imageViewHeight)),
                                    calcZoom(imageViewHeight),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Disprop:
                int newWidth = imageSize.width(), newHeight = imageSize.height();
                if (newWidth <= imageViewWidth) {
                    newWidth = imageViewWidth;
                }
                if (newHeight <= imageViewHeight) {
                    newHeight = imageViewHeight;
                }
                imageSize.scale(calcZoom(newWidth), calcZoom(newHeight), Qt::IgnoreAspectRatio);
                break;
        }

        switch (Settings::zoomOutFlags) {
            case Disable:
                if (imageSize.width() >= imageViewWidth || imageSize.height() >= imageViewHeight) {
                    imageSize.scale(calcZoom(imageSize.width()),
                                    calcZoom(imageSize.height()),
                                    Qt::KeepAspectRatio);
                }
                break;

            case WidthAndHeight:
                if (imageSize.width() >= imageViewWidth || imageSize.height() >= imageViewHeight) {
                    imageSize.scale(calcZoom(imageViewWidth),
                                    calcZoom(imageViewHeight),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Width:
                if (imageSize.width() >= imageViewWidth) {
                    imageSize.scale(calcZoom(imageViewWidth),
                                    calcZoom(getHeightByWidth(imageSize.width(),
                                                              imageSize.height(),
                                                              imageViewWidth)),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Height:
                if (imageSize.height() >= imageViewHeight) {
                    imageSize.scale(calcZoom(getWidthByHeight(imageSize.height(),
                                                              imageSize.width(),
                                                              imageViewHeight)),
                                    calcZoom(imageViewHeight),
                                    Qt::KeepAspectRatio);
                }
                break;

            case Disprop:
                int newWidth = imageSize.width(), newHeight = imageSize.height();
                if (newWidth >= imageViewWidth) {
                    newWidth = imageViewWidth;
                }
                if (newHeight >= imageViewHeight) {
                    newHeight = imageViewHeight;
                }
                imageSize.scale(calcZoom(newWidth), calcZoom(newHeight), Qt::IgnoreAspectRatio);
                break;
        }
    }

    imageLabel->setFixedSize(imageSize);
    imageLabel->adjustSize();
    centerImage(imageSize);
    busy = false;
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    resizeImage();
}

void ImageViewer::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    resizeImage();
}

void ImageViewer::centerImage(QSize &imgSize) {
    int newX = (this->size().width() - imgSize.width()) / 2;
    int newY = (this->size().height() - imgSize.height()) / 2;

    if (newX != imageLabel->pos().x() || newY != imageLabel->pos().y()) {
        imageLabel->move(newX, newY);
    }
}

void ImageViewer::rotateByExifRotation(QImage &image, QString &imageFullPath) {
    QTransform trans;
    long orientation = metadataCache->getImageOrientation(imageFullPath);

    switch (orientation) {
        case 1:
            break;
        case 2:
            image = image.mirrored(true, false);
            break;
        case 3:
            trans.rotate(180);
            image = image.transformed(trans, Qt::SmoothTransformation);
            break;
        case 4:
            image = image.mirrored(false, true);
            break;
        case 5:
            trans.rotate(90);
            image = image.transformed(trans, Qt::SmoothTransformation);
            image = image.mirrored(true, false);
            break;
        case 6:
            trans.rotate(90);
            image = image.transformed(trans, Qt::SmoothTransformation);
            break;
        case 7:
            trans.rotate(90);
            image = image.transformed(trans, Qt::SmoothTransformation);
            image = image.mirrored(false, true);
            break;
        case 8:
            trans.rotate(270);
            image = image.transformed(trans, Qt::SmoothTransformation);
            break;
        default:
            break;
    }
}

void ImageViewer::transform() {
    if (Settings::exifRotationEnabled) {
        rotateByExifRotation(viewerImage, viewerImageFullPath);
    }

    if (Settings::rotation) {
        QTransform trans;
        trans.rotate(Settings::rotation);
        viewerImage = viewerImage.transformed(trans, Qt::SmoothTransformation);
    }

    if (Settings::flipH || Settings::flipV) {
        viewerImage = viewerImage.mirrored(Settings::flipH, Settings::flipV);
    }

    int cropLeftPercentPixels = 0, cropTopPercentPixels = 0, cropWidthPercentPixels = 0, cropHeightPercentPixels = 0;
    bool croppingOn = false;
    if (Settings::cropLeftPercent || Settings::cropTopPercent
        || Settings::cropWidthPercent || Settings::cropHeightPercent) {
        croppingOn = true;
        cropLeftPercentPixels = (viewerImage.width() * Settings::cropLeftPercent) / 100;
        cropTopPercentPixels = (viewerImage.height() * Settings::cropTopPercent) / 100;
        cropWidthPercentPixels = (viewerImage.width() * Settings::cropWidthPercent) / 100;
        cropHeightPercentPixels = (viewerImage.height() * Settings::cropHeightPercent) / 100;
    }

    if (Settings::cropLeft || Settings::cropTop || Settings::cropWidth || Settings::cropHeight) {
        viewerImage = viewerImage.copy(
                Settings::cropLeft + cropLeftPercentPixels,
                Settings::cropTop + cropTopPercentPixels,
                viewerImage.width() - Settings::cropLeft - Settings::cropWidth - cropLeftPercentPixels -
                cropWidthPercentPixels,
                viewerImage.height() - Settings::cropTop - Settings::cropHeight - cropTopPercentPixels -
                cropHeightPercentPixels);
    } else {
        if (croppingOn) {
            viewerImage = viewerImage.copy(
                    cropLeftPercentPixels,
                    cropTopPercentPixels,
                    viewerImage.width() - cropLeftPercentPixels - cropWidthPercentPixels,
                    viewerImage.height() - cropTopPercentPixels - cropHeightPercentPixels);
        }
    }
}

void ImageViewer::mirror() {
    switch (mirrorLayout) {
        case LayDual: {
            mirrorImage = QImage(viewerImage.width() * 2, viewerImage.height(),
                                 QImage::Format_ARGB32);
            QPainter painter(&mirrorImage);
            painter.drawImage(0, 0, viewerImage);
            painter.drawImage(viewerImage.width(), 0, viewerImage.mirrored(true, false));
            break;
        }

        case LayTriple: {
            mirrorImage = QImage(viewerImage.width() * 3, viewerImage.height(),
                                 QImage::Format_ARGB32);
            QPainter painter(&mirrorImage);
            painter.drawImage(0, 0, viewerImage);
            painter.drawImage(viewerImage.width(), 0, viewerImage.mirrored(true, false));
            painter.drawImage(viewerImage.width() * 2, 0, viewerImage.mirrored(false, false));
            break;
        }

        case LayQuad: {
            mirrorImage = QImage(viewerImage.width() * 2, viewerImage.height() * 2,
                                 QImage::Format_ARGB32);
            QPainter painter(&mirrorImage);
            painter.drawImage(0, 0, viewerImage);
            painter.drawImage(viewerImage.width(), 0, viewerImage.mirrored(true, false));
            painter.drawImage(0, viewerImage.height(), viewerImage.mirrored(false, true));
            painter.drawImage(viewerImage.width(), viewerImage.height(),
                              viewerImage.mirrored(true, true));
            break;
        }

        case LayVDual: {
            mirrorImage = QImage(viewerImage.width(), viewerImage.height() * 2,
                                 QImage::Format_ARGB32);
            QPainter painter(&mirrorImage);
            painter.drawImage(0, 0, viewerImage);
            painter.drawImage(0, viewerImage.height(), viewerImage.mirrored(false, true));
            break;
        }
    }

    viewerImage = mirrorImage;
}

static inline int bound0To255(int val) {
    return ((val > 255) ? 255 : (val < 0) ? 0 : val);
}

static inline int hslValue(double n1, double n2, double hue) {
    double value;

    if (hue > 255) {
        hue -= 255;
    } else if (hue < 0) {
        hue += 255;
    }

    if (hue < 42.5) {
        value = n1 + (n2 - n1) * (hue / 42.5);
    } else if (hue < 127.5) {
        value = n2;
    } else if (hue < 170) {
        value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
    } else {
        value = n1;
    }

    return ROUND(value * 255.0);
}

void rgbToHsl(int r, int g, int b, unsigned char *hue, unsigned char *sat, unsigned char *light) {
    double h, s, l;
    int min, max;
    int delta;

    if (r > g) {
        max = MAX(r, b);
        min = MIN(g, b);
    } else {
        max = MAX(g, b);
        min = MIN(r, b);
    }

    l = (max + min) / 2.0;

    if (max == min) {
        s = 0.0;
        h = 0.0;
    } else {
        delta = (max - min);

        if (l < 128) {
            s = 255 * (double) delta / (double) (max + min);
        } else {
            s = 255 * (double) delta / (double) (511 - max - min);
        }

        if (r == max) {
            h = (g - b) / (double) delta;
        } else if (g == max) {
            h = 2 + (b - r) / (double) delta;
        } else {
            h = 4 + (r - g) / (double) delta;
        }

        h = h * 42.5;
        if (h < 0) {
            h += 255;
        } else if (h > 255) {
            h -= 255;
        }
    }

    *hue = ROUND(h);
    *sat = ROUND(s);
    *light = ROUND(l);
}

void hslToRgb(double h, double s, double l,
              unsigned char *red, unsigned char *green, unsigned char *blue) {
    if (s == 0) {
        /* achromatic case */
        *red = l;
        *green = l;
        *blue = l;
    } else {
        double m1, m2;

        if (l < 128)
            m2 = (l * (255 + s)) / 65025.0;
        else
            m2 = (l + s - (l * s) / 255.0) / 255.0;

        m1 = (l / 127.5) - m2;

        /* chromatic case */
        *red = hslValue(m1, m2, h + 85);
        *green = hslValue(m1, m2, h);
        *blue = hslValue(m1, m2, h - 85);
    }
}

void ImageViewer::colorize() {
    int y, x;
    unsigned char hr, hg, hb;
    int r, g, b;
    QRgb *line;
    unsigned char h, s, l;
    static unsigned char contrastTransform[256];
    static unsigned char brightTransform[256];
    bool hasAlpha = viewerImage.hasAlphaChannel();

    if (viewerImage.colorCount()) {
        viewerImage = viewerImage.convertToFormat(QImage::Format_RGB32);
    }

    int i;
    float contrast = ((float) Settings::contrastVal / 100.0);
    float brightness = ((float) Settings::brightVal / 100.0);

    for (i = 0; i < 256; ++i) {
        if (i < (int) (128.0f + 128.0f * tan(contrast)) && i > (int) (128.0f - 128.0f * tan(contrast))) {
            contrastTransform[i] = (i - 128) / tan(contrast) + 128;
        } else if (i >= (int) (128.0f + 128.0f * tan(contrast))) {
            contrastTransform[i] = 255;
        } else {
            contrastTransform[i] = 0;
        }
    }

    for (i = 0; i < 256; ++i) {
        brightTransform[i] = MIN(255, (int) ((255.0 * pow(i / 255.0, 1.0 / brightness)) + 0.5));
    }

    for (y = 0; y < viewerImage.height(); ++y) {

        line = (QRgb *) viewerImage.scanLine(y);
        for (x = 0; x < viewerImage.width(); ++x) {
            r = Settings::rNegateEnabled ? bound0To255(255 - qRed(line[x])) : qRed(line[x]);
            g = Settings::gNegateEnabled ? bound0To255(255 - qGreen(line[x])) : qGreen(line[x]);
            b = Settings::bNegateEnabled ? bound0To255(255 - qBlue(line[x])) : qBlue(line[x]);

            r = bound0To255((r * (Settings::redVal + 100)) / 100);
            g = bound0To255((g * (Settings::greenVal + 100)) / 100);
            b = bound0To255((b * (Settings::blueVal + 100)) / 100);

            r = bound0To255(brightTransform[r]);
            g = bound0To255(brightTransform[g]);
            b = bound0To255(brightTransform[b]);

            r = bound0To255(contrastTransform[r]);
            g = bound0To255(contrastTransform[g]);
            b = bound0To255(contrastTransform[b]);

            rgbToHsl(r, g, b, &h, &s, &l);
            h = Settings::colorizeEnabled ? Settings::hueVal : h + Settings::hueVal;
            s = bound0To255(((s * Settings::saturationVal) / 100));
            l = bound0To255(((l * Settings::lightnessVal) / 100));
            hslToRgb(h, s, l, &hr, &hg, &hb);

            r = Settings::hueRedChannel ? hr : qRed(line[x]);
            g = Settings::hueGreenChannel ? hg : qGreen(line[x]);
            b = Settings::hueBlueChannel ? hb : qBlue(line[x]);

            if (hasAlpha) {
                line[x] = qRgba(r, g, b, qAlpha(line[x]));
            } else {
                line[x] = qRgb(r, g, b);
            }
        }
    }
}

void ImageViewer::refresh() {
    if (isAnimation) {
        return;
    }

    if (Settings::scaledWidth) {
        viewerImage = origImage.scaled(Settings::scaledWidth, Settings::scaledHeight,
                                       Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        viewerImage = origImage;
    }

    transform();

    if (Settings::colorsActive || Settings::keepTransform) {
        colorize();
    }

    if (mirrorLayout) {
        mirror();
    }

    viewerPixmap = QPixmap::fromImage(viewerImage);
    imageLabel->setPixmap(viewerPixmap);
    resizeImage();
}

QImage createImageWithOverlay(const QImage &baseImage, const QImage &overlayImage, int x, int y) {
    QImage imageWithOverlay = QImage(overlayImage.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&imageWithOverlay);

    QImage scaledImage = baseImage.scaled(overlayImage.width(), overlayImage.height(),
                                          Qt::KeepAspectRatio, Qt::SmoothTransformation);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(imageWithOverlay.rect(), Qt::transparent);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(x, y, scaledImage);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, overlayImage);

    painter.end();

    return imageWithOverlay;
}

void ImageViewer::reload() {
    isAnimation = false;
    if (Settings::showImageName) {
        if (viewerImageFullPath.left(1) == ":") {
            setInfo("No Image");
        } else if (viewerImageFullPath.isEmpty()) {
            setInfo("Clipboard");
        } else {
            setInfo(QFileInfo(viewerImageFullPath).fileName());
        }
    }

    if (!Settings::keepTransform) {
        Settings::cropLeftPercent = Settings::cropTopPercent = Settings::cropWidthPercent = Settings::cropHeightPercent = 0;
        Settings::rotation = 0;
        Settings::flipH = Settings::flipV = false;
    }
    Settings::scaledWidth = Settings::scaledHeight = 0;
    Settings::cropLeft = Settings::cropTop = Settings::cropWidth = Settings::cropHeight = 0;

    if (newImage || viewerImageFullPath.isEmpty()) {
        newImage = true;
        viewerImageFullPath = CLIPBOARD_IMAGE_NAME;
        origImage.load(":/images/no_image.png");
        viewerImage = origImage;
        viewerPixmap = QPixmap::fromImage(viewerImage);
        imageLabel->setPixmap(viewerPixmap);
        pasteImage();
        return;
    }

    QImageReader imageReader(viewerImageFullPath);
    if (Settings::enableAnimations && imageReader.supportsAnimation()) {
        if (animation) {
            delete animation;
        }
        animation = new QMovie(viewerImageFullPath);

        if (animation->frameCount() > 1) {
            isAnimation = true;
            imageLabel->setMovie(animation);
            animation->start();
            resizeImage();
            return;
        }
    }

    if (imageReader.size().isValid() && imageReader.read(&origImage)) {
        viewerImage = origImage;
        transform();
        if (Settings::colorsActive || Settings::keepTransform) {
            colorize();
        }
        if (mirrorLayout) {
            mirror();
        }
        viewerPixmap = QPixmap::fromImage(viewerImage);
    } else {
        viewerPixmap = QIcon::fromTheme("image-missing",
                                        QIcon(":/images/error_image.png")).pixmap(BAD_IMAGE_SIZE, BAD_IMAGE_SIZE);
        setInfo(imageReader.errorString());
    }

    imageLabel->setPixmap(viewerPixmap);
    resizeImage();
    if (Settings::setWindowIcon) {
        phototonic->setWindowIcon(viewerPixmap.scaled(WINDOW_ICON_SIZE, WINDOW_ICON_SIZE,
                                                      Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ImageViewer::setInfo(QString infoString) {
    imageInfoLabel->setText(infoString);
    imageInfoLabel->adjustSize();
}

void ImageViewer::unsetFeedback() {
    feedbackLabel->clear();
    feedbackLabel->setVisible(false);
}

void ImageViewer::setFeedback(QString feedbackString) {
    feedbackLabel->setText(feedbackString);
    feedbackLabel->setVisible(true);

    int margin = imageInfoLabel->isVisible() ? (imageInfoLabel->height() + 15) : 10;
    feedbackLabel->move(10, margin);

    feedbackLabel->adjustSize();
    QTimer::singleShot(3000, this, SLOT(unsetFeedback()));
}

void ImageViewer::loadImage(QString imageFileName) {
    newImage = false;
    tempDisableResize = false;
    viewerImageFullPath = imageFileName;

    if (!Settings::keepZoomFactor) {
        Settings::imageZoomFactor = 1.0;
    }

    QApplication::processEvents();
    reload();
}

void ImageViewer::clearImage() {
    origImage.load(":/images/no_image.png");
    viewerImage = origImage;
    viewerPixmap = QPixmap::fromImage(viewerImage);
    imageLabel->setPixmap(viewerPixmap);
}

void ImageViewer::monitorCursorState() {
    static QPoint lastPos;

    if (QCursor::pos() != lastPos) {
        lastPos = QCursor::pos();
        if (cursorIsHidden) {
            QApplication::restoreOverrideCursor();
            cursorIsHidden = false;
        }
    } else {
        if (!cursorIsHidden) {
            QApplication::setOverrideCursor(Qt::BlankCursor);
            cursorIsHidden = true;
        }
    }
}

void ImageViewer::setCursorHiding(bool hide) {
    if (hide) {
        mouseMovementTimer->start(500);
    } else {
        mouseMovementTimer->stop();
        if (cursorIsHidden) {
            QApplication::restoreOverrideCursor();
            cursorIsHidden = false;
        }
    }
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event) {
    QWidget::mouseDoubleClickEvent(event);
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() == Qt::ControlModifier) {
            cropOrigin = event->pos();
            if (!cropRubberBand) {
                cropRubberBand = new CropRubberBand(this);
            }
            cropRubberBand->show();
            cropRubberBand->setGeometry(QRect(cropOrigin, event->pos()).normalized());
        } else {
            if (cropRubberBand) {
                cropRubberBand->hide();
            }
        }

        setMouseMoveData(true, event->x(), event->y());
        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setMouseMoveData(false, 0, 0);
        while (QApplication::overrideCursor()) {
            QApplication::restoreOverrideCursor();
        }

        if (cropRubberBand && cropRubberBand->isVisible()) {
            setFeedback(tr("Selection size: ")
                        + QString::number(cropRubberBand->width())
                        + "x"
                        + QString::number(cropRubberBand->height()));
        }
    }

    QWidget::mouseReleaseEvent(event);
}

void ImageViewer::cropToSelection() {
    if (cropRubberBand && cropRubberBand->isVisible()) {

        QPoint bandTopLeft = mapToGlobal(cropRubberBand->geometry().topLeft());
        QPoint bandBottomRight = mapToGlobal(cropRubberBand->geometry().bottomRight());

        bandTopLeft = imageLabel->mapFromGlobal(bandTopLeft);
        bandBottomRight = imageLabel->mapFromGlobal(bandBottomRight);

        double scaledX = imageLabel->rect().width();
        double scaledY = imageLabel->rect().height();
        scaledX = viewerPixmap.width() / scaledX;
        scaledY = viewerPixmap.height() / scaledY;

        bandTopLeft.setX(int(bandTopLeft.x() * scaledX));
        bandTopLeft.setY(int(bandTopLeft.y() * scaledY));
        bandBottomRight.setX(int(bandBottomRight.x() * scaledX));
        bandBottomRight.setY(int(bandBottomRight.y() * scaledY));

        int cropLeft = bandTopLeft.x();
        int cropTop = bandTopLeft.y();
        int cropWidth = viewerPixmap.width() - bandBottomRight.x();
        int cropHeight = viewerPixmap.height() - bandBottomRight.y();

        if (cropLeft > 0) {
            Settings::cropLeft += cropLeft;
        }
        if (cropTop > 0) {
            Settings::cropTop += cropTop;
        }
        if (cropWidth > 0) {
            Settings::cropWidth += cropWidth;
        }
        if (cropHeight > 0) {
            Settings::cropHeight += cropHeight;
        }

        cropRubberBand->hide();
        refresh();
    } else {
        MessageBox messageBox(this);
        messageBox.warning(tr("No selection for cropping"),
                           tr("To make a selection, hold down the Ctrl key and select a region using the mouse."));
    }
}

void ImageViewer::setMouseMoveData(bool lockMove, int lMouseX, int lMouseY) {
    moveImageLocked = lockMove;
    mouseX = lMouseX;
    mouseY = lMouseY;
    layoutX = imageLabel->pos().x();
    layoutY = imageLabel->pos().y();
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
    if (event->modifiers() == Qt::ControlModifier) {
        if (cropRubberBand && cropRubberBand->isVisible()) {
            cropRubberBand->setGeometry(QRect(cropOrigin, event->pos()).normalized());
        }
    } else {
        if (moveImageLocked) {
            int newX = layoutX + (event->pos().x() - mouseX);
            int newY = layoutY + (event->pos().y() - mouseY);
            bool needToMove = false;

            if (imageLabel->size().width() > size().width()) {
                if (newX > 0) {
                    newX = 0;
                } else if (newX < (size().width() - imageLabel->size().width())) {
                    newX = (size().width() - imageLabel->size().width());
                }
                needToMove = true;
            } else {
                newX = layoutX;
            }

            if (imageLabel->size().height() > size().height()) {
                if (newY > 0) {
                    newY = 0;
                } else if (newY < (size().height() - imageLabel->size().height())) {
                    newY = (size().height() - imageLabel->size().height());
                }
                needToMove = true;
            } else {
                newY = layoutY;
            }

            if (needToMove) {
                imageLabel->move(newX, newY);
            }
        }
    }
}

void ImageViewer::keyMoveEvent(int direction) {
    int newX = layoutX = imageLabel->pos().x();
    int newY = layoutY = imageLabel->pos().y();
    bool needToMove = false;

    switch (direction) {
        case MoveLeft:
            newX += 50;
            break;
        case MoveRight:
            newX -= 50;
            break;
        case MoveUp:
            newY += 50;
            break;
        case MoveDown:
            newY -= 50;
            break;
    }

    if (imageLabel->size().width() > size().width()) {
        if (newX > 0) {
            newX = 0;
        } else if (newX < (size().width() - imageLabel->size().width())) {
            newX = (size().width() - imageLabel->size().width());
        }
        needToMove = true;
    } else {
        newX = layoutX;
    }

    if (imageLabel->size().height() > size().height()) {
        if (newY > 0) {
            newY = 0;
        } else if (newY < (size().height() - imageLabel->size().height())) {
            newY = (size().height() - imageLabel->size().height());
        }
        needToMove = true;
    } else {
        newY = layoutY;
    }

    if (needToMove) {
        int i;

        switch (direction) {
            case MoveLeft:
                for (i = imageLabel->pos().x(); i <= newX; ++i)
                    imageLabel->move(newX, newY);
                break;
            case MoveRight:
                for (i = imageLabel->pos().x(); i >= newX; --i)
                    imageLabel->move(newX, newY);
                break;
            case MoveUp:
                for (i = imageLabel->pos().y(); i <= newY; ++i)
                    imageLabel->move(newX, newY);
                break;
            case MoveDown:
                for (i = imageLabel->pos().y(); i >= newY; --i)
                    imageLabel->move(newX, newY);
                break;
        }
    }
}

void ImageViewer::saveImage() {
    Exiv2::Image::AutoPtr image;
    bool exifError = false;

    if (newImage) {
        saveImageAs();
        return;
    }

    setFeedback(tr("Saving..."));

    try {
        image = Exiv2::ImageFactory::open(viewerImageFullPath.toStdString());
        image->readMetadata();
    }
    catch (Exiv2::Error &error) {
        exifError = true;
    }

    QImageReader imageReader(viewerImageFullPath);
    if (!viewerPixmap.save(viewerImageFullPath, imageReader.format().toUpper(), Settings::defaultSaveQuality)) {
        MessageBox msgBox(this);
        msgBox.critical(tr("Error"), tr("Failed to save image."));
        return;
    }

    if (!exifError) {
        try {
            image->writeMetadata();
        }
        catch (Exiv2::Error &error) {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), tr("Failed to save Exif metadata."));
        }
    }

    reload();
    setFeedback(tr("Image saved."));
}

void ImageViewer::saveImageAs() {
    Exiv2::Image::AutoPtr exifImage;
    Exiv2::Image::AutoPtr newExifImage;
    bool exifError = false;

    setCursorHiding(false);

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save image as"),
                                                    viewerImageFullPath,
                                                    tr("Images") +
                                                    " (*.jpg *.jpeg *.png *.bmp *.tif *.tiff *.ppm *.pgm *.pbm *.xbm *.xpm *.cur *.ico *.icns *.wbmp *.webp)");

    if (!fileName.isEmpty()) {
        try {
            exifImage = Exiv2::ImageFactory::open(viewerImageFullPath.toStdString());
            exifImage->readMetadata();
        }
        catch (Exiv2::Error &error) {
            exifError = true;
        }


        if (!viewerPixmap.save(fileName, 0, Settings::defaultSaveQuality)) {
            MessageBox msgBox(this);
            msgBox.critical(tr("Error"), tr("Failed to save image."));
        } else {
            if (!exifError) {
                try {
                    newExifImage = Exiv2::ImageFactory::open(fileName.toStdString());
                    newExifImage->setMetadata(*exifImage);
                    newExifImage->writeMetadata();
                }
                catch (Exiv2::Error &error) {
                    exifError = true;
                }
            }

            setFeedback(tr("Image saved."));
        }
    }
    if (phototonic->isFullScreen()) {
        setCursorHiding(true);
    }
}

void ImageViewer::contextMenuEvent(QContextMenuEvent *) {
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    contextMenuPosition = QCursor::pos() - window()->geometry().topLeft();
    ImagePopUpMenu->exec(contextMenuPosition);
}

int ImageViewer::getImageWidthPreCropped() {
    return origImage.width();
}

int ImageViewer::getImageHeightPreCropped() {
    return origImage.height();
}

bool ImageViewer::isNewImage() {
    return newImage;
}

void ImageViewer::copyImage() {
    QApplication::clipboard()->setImage(viewerImage);
}

void ImageViewer::pasteImage() {
    if (isAnimation) {
        return;
    }

    if (!QApplication::clipboard()->image().isNull()) {
        origImage = QApplication::clipboard()->image();
        refresh();
    }
    phototonic->setWindowTitle(tr("Clipboard") + " - Phototonic");
    if (Settings::setWindowIcon) {
        phototonic->setWindowIcon(phototonic->getDefaultWindowIcon());
    }
}

void ImageViewer::setBackgroundColor() {
    QString bgColor = "background: rgb(%1, %2, %3); ";
    bgColor = bgColor.arg(Settings::viewerBackgroundColor.red())
            .arg(Settings::viewerBackgroundColor.green())
            .arg(Settings::viewerBackgroundColor.blue());

    QString styleSheet = "QWidget { " + bgColor + " }";
    scrollArea->setStyleSheet(styleSheet);
}

QPoint ImageViewer::getContextMenuPosition() {
    return contextMenuPosition;
}

