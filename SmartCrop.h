#pragma once

// Algorithm from https://github.com/jwagner/smartcrop.js/

#include <QImage>

namespace SmartCrop {

struct CropOptions {
    CropOptions(const QSize &targetSize) : width(targetSize.width()), height(targetSize.height()) {}
    CropOptions() = default;

    qreal width = 0;
    qreal height= 0;
    qreal aspect= 0;
    qreal cropWidth= 0;
    qreal cropHeight= 0;
    qreal detailWeight= 0.2;
    qreal skinColor[3] = {0.78, 0.57, 0.44};
    qreal skinBias= 0.01;
    qreal skinBrightnessMin= 0.2;
    qreal skinBrightnessMax= 0.9;
    qreal skinThreshold= 0.8;
    qreal skinWeight= 1.8;
    qreal saturationBrightnessMin= 0.05;
    qreal saturationBrightnessMax= 0.9;
    qreal saturationThreshold= 0.4;
    qreal saturationBias= 0.2;
    qreal saturationWeight= 0.1;
    // Step * minscale rounded down to the next power of two should be good
    qreal scoreDownSample= 8;
    qreal step= 8;
    qreal scaleStep= 0.1;
    qreal minScale= 1.0;
    qreal maxScale= 1.0;
    qreal edgeRadius= 0.4;
    qreal edgeWeight= -20.0;
    qreal outsideImportance= -0.5;
    qreal boostWeight= 100.0;
    bool ruleOfThirds= true;
    bool prescale= true;
};

QImage crop(const QImage &input, CropOptions options);
QRect smartCropRect(const QImage &input, CropOptions options);
}
