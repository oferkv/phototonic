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
#include "global.h"
#include "mdcache.h"

void MetadataCache::updateImageTags(QString &imageFileName, QSet<QString> tags)
{
	imageTagsCache[imageFileName].tags = tags;
}

bool MetadataCache::removeTagFromImage(QString &imageFileName, const QString &tagName)
{
	return imageTagsCache[imageFileName].tags.remove(tagName);
}

void MetadataCache::removeImage(QString &imageFileName)
{
	imageTagsCache.remove(imageFileName);
}

QSet<QString>& MetadataCache::getImageTags(QString &imageFileName)
{
	return imageTagsCache[imageFileName].tags ;
}

int MetadataCache::getImageOrientation(QString &imageFileName)
{
	if (imageTagsCache.contains(imageFileName)|| loadImageMetadata(imageFileName)) {
		return imageTagsCache[imageFileName].orientation;
	} 

	return 0;
}

void MetadataCache::setImageTags(const QString &imageFileName, QSet<QString> tags)
{
	ImageMetadata imageMetadata;

	imageMetadata.tags = tags;
	imageTagsCache.insert(imageFileName, imageMetadata);
}

void MetadataCache::addTagToImage(QString &imageFileName, QString &tagName)
{
	if (imageTagsCache[imageFileName].tags.contains(tagName)) {
		return;
	}
	
	imageTagsCache[imageFileName].tags.insert(tagName);
}

void MetadataCache::clear()
{
	imageTagsCache.clear();
}

bool MetadataCache::loadImageMetadata(const QString &imageFullPath)
{
	Exiv2::Image::AutoPtr exifImage;
	QSet<QString> tags;
	long orientation = 0;

	try {
		exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
		exifImage->readMetadata();

		Exiv2::ExifData &exifData = exifImage->exifData();
		if (!exifData.empty()) {
			orientation = exifData["Exif.Image.Orientation"].value().toLong();
		}

		Exiv2::IptcData &iptcData = exifImage->iptcData();
		if (!iptcData.empty()) {
			QString key;
			Exiv2::IptcData::iterator end = iptcData.end();
			for (Exiv2::IptcData::iterator iptcIt = iptcData.begin(); iptcIt != end; ++iptcIt) {
				if (iptcIt->tagName() == "Keywords") {
					QString tagName = QString::fromUtf8(iptcIt->toString().c_str());
					tags.insert(tagName);
					GData::knownTags.insert(tagName);
				}
	        }
	    }
    }

    catch (Exiv2::Error &error) {
		return false;
	}

	ImageMetadata imageMetadata;

	if (tags.size()) {
		imageMetadata.tags = tags;
	}

	if (orientation) {
		imageMetadata.orientation = orientation;
	}

	if (tags.size() || orientation) {
		imageTagsCache.insert(imageFullPath, imageMetadata);
	}

	return true;
}

