/*
 *  Copyright (C) 2013-2015 Ofer Kashayov <oferkv@live.com>
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

#include <exiv2/exiv2.hpp>
#include "Settings.h"
#include "MetadataCache.h"

void MetadataCache::updateImageTags(QString &imageFileName, QSet<QString> tags) {
    cache[imageFileName].tags = tags;
}

bool MetadataCache::removeTagFromImage(QString &imageFileName, const QString &tagName) {
    return cache[imageFileName].tags.remove(tagName);
}

void MetadataCache::removeImage(QString &imageFileName) {
    cache.remove(imageFileName);
}

QSet<QString> &MetadataCache::getImageTags(QString &imageFileName) {
    return cache[imageFileName].tags;
}

long MetadataCache::getImageOrientation(QString &imageFileName) {
    if (cache.contains(imageFileName) || loadImageMetadata(imageFileName)) {
        return cache[imageFileName].orientation;
    }

    return 0;
}

void MetadataCache::setImageTags(const QString &imageFileName, QSet<QString> tags) {
    ImageMetadata imageMetadata;

    imageMetadata.tags = tags;
    cache.insert(imageFileName, imageMetadata);
}

void MetadataCache::addTagToImage(QString &imageFileName, QString &tagName) {
    if (cache[imageFileName].tags.contains(tagName)) {
        return;
    }

    cache[imageFileName].tags.insert(tagName);
}

void MetadataCache::clear() {
    cache.clear();
}

bool MetadataCache::loadImageMetadata(const QString &imageFullPath) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    Exiv2::Image::AutoPtr exifImage;
#pragma clang diagnostic pop

    QSet<QString> tags;
    long orientation = 0;

    try {
        exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
        exifImage->readMetadata();
    } catch (Exiv2::Error &error) {
        qWarning() << "Error loading image for reading metadata" << error.what();
        return false;
    }

    if (!exifImage->good()) {
        return false;
    }

    if (exifImage->supportsMetadata(Exiv2::mdExif)) try {
        Exiv2::ExifData::const_iterator it = Exiv2::orientation(exifImage->exifData());
        if (it != exifImage->exifData().end()) {
            orientation = it->toLong();
        }
    } catch (Exiv2::Error &error) {
        qWarning() << "Failed to read Exif metadata" << error.what();
    }

    if (exifImage->supportsMetadata(Exiv2::mdIptc)) try {
        Exiv2::IptcData &iptcData = exifImage->iptcData();
        if (!iptcData.empty()) {
            QString key;
            Exiv2::IptcData::iterator end = iptcData.end();

            // Finds the first ID, but we need to loop over the rest in case there are more
            Exiv2::IptcData::iterator iptcIt = iptcData.findId(Exiv2::IptcDataSets::Keywords);
            for (; iptcIt != end; ++iptcIt) {
                if (iptcIt->tag() != Exiv2::IptcDataSets::Keywords) {
                    continue;
                }

                QString tagName = QString::fromUtf8(iptcIt->toString().c_str());
                tags.insert(tagName);
                Settings::knownTags.insert(tagName);
            }
        }
    } catch (Exiv2::Error &error) {
        qWarning() << "Failed to read Iptc metadata";
    }

    ImageMetadata imageMetadata;
    if (tags.size()) {
        imageMetadata.tags = tags;
    }

    if (orientation) {
        imageMetadata.orientation = orientation;
    }

    if (tags.size() || orientation) {
        cache.insert(imageFullPath, imageMetadata);
    }

    return true;
}

