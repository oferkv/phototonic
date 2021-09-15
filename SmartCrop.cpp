#include "SmartCrop.h"
#include <QDebug>
#include <QtMath>

namespace SmartCrop {

static inline qreal cie(qreal r, qreal g, qreal b) {
    return 0.5126 * b + 0.7152 * g + 0.0722 * r;
}

static inline qreal sample(const uchar *id, int p) {
    return cie(id[p], id[p + 1], id[p + 2]);
}

static void edgeDetect(const QImage &i, QImage &o) {
    const int bpl = i.bytesPerLine();//: width();
    const uchar *id = i.scanLine(0);
    const int h = i.height();

    for (int y = 0; y < h; y++) {
        uchar *od = o.scanLine(y);
        for (int x = 0; x < i.width(); x++) {
            int p = y * bpl + x * 4;
            qreal lightness;

            if (x == 0 || x >= i.width() - 1 || y == 0 || y >= h - 1) {
                lightness = sample(id, p);
            } else {
                lightness = sample(id, p) * 4. -
                        sample(id, p - bpl) -
                        sample(id, p - 4) -
                        sample(id, p + 4) -
                        sample(id, p + bpl);
            }

            od[x * 4 + 1] = qMax<int>(lightness, 0);
        }
    }
}

static qreal skinColor(const CropOptions &options, qreal r,qreal g, qreal b) {
    qreal mag = sqrt(r * r + g * g + b * b);
    if (Q_UNLIKELY(mag == 0.)) {
        return 0.;
    }
    qreal rd = r / mag - options.skinColor[0];
    qreal gd = g / mag - options.skinColor[1];
    qreal bd = b / mag - options.skinColor[2];
    qreal d = sqrt(rd * rd + gd * gd + bd * bd);
    return 1 - d;
}

static void skinDetect(const CropOptions &options, const QImage &i, QImage &o) {
    int w = i.width();
    int h = i.height();

    for (int y = 0; y < h; y++) {
        uchar *od = o.scanLine(y);
        const uchar *id = i.scanLine(y) ;
        for (int x = 0; x < w; x++) {
            int p = x * 4;
            qreal lightness = cie(id[p], id[p + 1], id[p + 2]) / 255.;
            qreal skin = skinColor(options, id[p], id[p + 1], id[p + 2]);
            bool isSkinColor = skin > options.skinThreshold;
            bool isSkinBrightness =
                    lightness >= options.skinBrightnessMin &&
                    lightness <= options.skinBrightnessMax;
            if (isSkinColor && isSkinBrightness) {
                od[p] =
                        (skin - options.skinThreshold) *
                        (255. / (1. - options.skinThreshold));
            } else {
                od[p] = 0;
            }
        }
    }
}
static qreal saturation(qreal r, qreal g, qreal b) {
    qreal maximum = qMax(r / 255., qMax(g / 255., b / 255.));
    qreal minimum = qMin(r / 255., qMin(g / 255., b / 255.));

    if (qFuzzyCompare(maximum, minimum)) {
        return 0;
    }

    qreal l = (maximum + minimum) / 2.;
    qreal d = maximum - minimum;

    return l > 0.5 ? d / (2. - maximum - minimum) : d / (maximum + minimum);
}
static void saturationDetect(const CropOptions &options, const QImage &i, QImage &o) {
    int w = i.width();
    int h = i.height();
    for (int y = 0; y < h; y++) {
        uchar *od = o.scanLine(y);
        const uchar *id = i.scanLine(y);
        for (int x = 0; x < w; x++) {
            int p = x * 4;

            qreal lightness = cie(id[p], id[p + 1], id[p + 2]) / 255.;
            qreal sat = saturation(id[p], id[p + 1], id[p + 2]);

            bool acceptableSaturation = sat > options.saturationThreshold;
            bool acceptableLightness =
                    lightness >= options.saturationBrightnessMin &&
                    lightness <= options.saturationBrightnessMax;
            if (acceptableLightness && acceptableSaturation) {
                od[p + 2] =
                        (sat - options.saturationThreshold) *
                        (255. / (1 - options.saturationThreshold));
            } else {
                od[p + 2] = 0;
            }
        }
    }
}

static QImage downSample(QImage &input, qreal factor) {
    int width = qFloor(input.width() / factor);
    int height = qFloor(input.height() / factor);
    QImage output(width, height, QImage::Format_RGBA8888);
    qreal ifactor2 = 1. / (factor * factor);
    for (int y = 0; y < height; y++) {
        uchar *data = output.scanLine(y);
        for (int x = 0; x < width; x++) {
            int i = x * 4;

            qreal r = 0;
            qreal g = 0;
            qreal b = 0;
            qreal a = 0;

            qreal mr = 0;
            qreal mg = 0;

            for (int v = 0; v < factor; v++) {
                uchar *idata = input.scanLine(y * factor + v);
                for (int u = 0; u < factor; u++) {
                    int j = (x * factor + u) * 4;
                    r += idata[j];
                    g += idata[j + 1];
                    b += idata[j + 2];
                    a += idata[j + 3];
                    mr = qMax(mr, qreal(idata[j]));
                    mg = qMax(mg, qreal(idata[j + 1]));
                }
            }
            // this is some funky magic to preserve detail a bit more for
            // skin (r) and detail (g). Saturation (b) does not get this boost.
            data[i] = r * ifactor2 * 0.5 + mr * 0.5;
            data[i + 1] = g * ifactor2 * 0.7 + mg * 0.3;
            data[i + 2] = b * ifactor2;
            data[i + 3] = a * ifactor2;
        }
    }
    return output;
}

struct ScoreResult {
};
// Gets value in the range of [0, 1] where 0 is the center of the pictures
// returns weight of rule of thirds [0, 1]
static qreal thirds(qreal x) {
    x = ((int(x - 1 / 3 + 1.0) % 2) * 0.5 - 0.5) * 16;
    return qMax(1.0 - x * x, 0.0);
}

static inline qreal importance(const CropOptions &options, const QRectF &crop, qreal x, qreal y) {
    if (
            crop.x() > x ||
            x >= crop.x() + crop.width() ||
            crop.y() > y ||
            y >= crop.y() + crop.height()
            ) {
        return options.outsideImportance;
    }
    x = (x - crop.x()) / qreal(crop.width());
    y = (y - crop.y()) / qreal(crop.height());
    qreal px = abs(0.5 - x) * 2.;
    qreal py = abs(0.5 - y) * 2.;
    // Distance from edge
    qreal dx = qMax<qreal>(px - 1.0 + options.edgeRadius, 0);
    qreal dy = qMax<qreal>(py - 1.0 + options.edgeRadius, 0);
    qreal d = (dx * dx + dy * dy) * options.edgeWeight;
    qreal s = 1.41 - sqrt(px * px + py * py);
    if (options.ruleOfThirds) {
        s += qMax(0., s + d + 0.5) * 1.2 * (thirds(px) + thirds(py));
    }
    return s + d;
}

static QList<QRectF> generateCrops(const CropOptions &options, qreal width, qreal height) {
    QList<QRectF> results;
    int minDimension = qMin(width, height);
    qreal cropWidth = options.cropWidth > 0 ? options.cropWidth : minDimension;
    qreal cropHeight = options.cropHeight > 0 ? options.cropHeight : minDimension;
    for (
         qreal scale = options.maxScale;
         scale >= options.minScale;
         scale -= options.scaleStep
         ) {
        for (qreal y = 0; y + cropHeight * scale <= height; y += options.step) {
            for (qreal x = 0; x + cropWidth * scale <= width; x += options.step) {
                results.append({x, y, cropWidth * scale, cropHeight * scale});
            }
        }
    }
    return results;
}

static float score(const CropOptions &options, const QImage &output, const QRectF &crop) {
    qreal detail = 0;
    qreal saturation = 0;
    qreal skin = 0;
    qreal boost = 0;

    qreal downSample = options.scoreDownSample;
    qreal invDownSample = 1. / downSample;
    qreal outputHeightDownSample = output.height() * downSample;
    qreal outputWidthDownSample = output.width() * downSample;

    for (int y = 0; y < outputHeightDownSample; y += downSample) {
        const uchar *od = output.scanLine(qFloor(y * invDownSample));
        for (int x = 0; x < outputWidthDownSample; x += downSample) {
            int p = (qFloor(x * invDownSample)) * 4;
            qreal i = importance(options, crop, x, y);
            qreal detail = od[p + 1] / 255.;

            skin += (od[p] / 255.) * (detail + options.skinBias) * i;
            detail += detail * i;
            saturation +=
                    (od[p + 2] / 255) * (detail + options.saturationBias) * i;
            boost += (od[p + 3] / 255.) * i;
        }
    }

    return
            (detail * options.detailWeight +
             skin * options.skinWeight +
             saturation * options.saturationWeight +
             boost * options.boostWeight) /
            (crop.width() * crop.height());
}

QRect smartCropRect(const QImage &input, CropOptions options)
{
    if (input.isNull()) {
        qWarning() << "Invalid image";
        return input.rect();
    }

    if (options.aspect) {
        options.width = options.aspect;
        options.height = 1;
    }

    QImage image = input;

    qreal scale = 1;
    qreal prescale = 1;
    if (options.width && options.height) {
        scale = qMin(
                    image.width() / options.width,
                    image.height() / options.height
                    );
        options.cropWidth = qFloor(options.width * scale);
        options.cropHeight = qFloor(options.height * scale);
        // Img = 100x100, width = 95x95, scale = 100/95, 1/scale > min
        // don't set minscale smaller than 1/scale
        // -> don't pick crops that need upscaling
        options.minScale = qMin(
                    options.maxScale,
                    qMax(1 / scale, options.minScale)
                    );

        // prescale if possible
        if (options.prescale != false) {
            prescale = qMin(qMax(256. / image.width(), 256. / image.height()), 1.);
            if (prescale < 1) {
                image = image.scaled(image.width() * prescale, image.height () * prescale);
                options.cropWidth = qFloor(options.cropWidth * prescale);
                options.cropHeight = qFloor(options.cropHeight * prescale);
            } else {
                prescale = 1;
            }
        }
    }
    image = image.convertToFormat(QImage::Format_RGBA8888);

    QImage filtered = image.copy();
    if (filtered.isNull()) {
        qWarning() << "Failed to copy image!";
        return input.rect();
    }

    edgeDetect(image, filtered);
    skinDetect(options, image, filtered);
    saturationDetect(options, image, filtered);
    QImage toScore = downSample(filtered, options.scoreDownSample);

    QList<QRectF> crops = generateCrops(options, image.width(), image.height());

    QRectF topCrop;
    qreal topScore = -1;
    for (const QRectF &crop : crops) {
        float scr = score(options, toScore, crop);
        if (scr > topScore || topCrop.isEmpty()) {
            topCrop = crop;
            topScore = scr;
        }
    }

    return QRectF(topCrop.x() / prescale, topCrop.y() / prescale, topCrop.width() / prescale, topCrop.height() / prescale).toAlignedRect();
}

QImage crop(const QImage &input, CropOptions options)
{
    if (input.isNull()) {
        qWarning() << "Invalid image";
        return input;
    }
    return input.copy(smartCropRect(input, options));

}

} // namespace SmartCrop
