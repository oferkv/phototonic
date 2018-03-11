# Phototonic Image Viewer

[![Build Status](https://travis-ci.org/oferkv/phototonic.svg?branch=master)](https://travis-ci.org/oferkv/phototonic)

### About

Phototonic is an image viewer and organizer built with Qt and Exiv2, released under GNU General Public License v3.

### Features
+ Support for common image formats and GIF animation
+ Supports tagging images, and filtering images by tags (IPTC)
+ Browse thumbnails recursively down a folder tree
+ Dynamic thumbnails loading
+ Image transformation and color manipulation
+ Display image information and metadata
+ Does not depend on any desktop environment

### Updates:

##### 28 Feb 2018 - v2.0
+ Move to Trash
+ File List support
+ Bug fixes

##### 15 Jan 2018
+ Back after a long break
+ Code cleanup and removal of useless features
+ Lots of bug fixes
+ Added Remove Image Metadata action
+ Enhanced keyboard settings
+ Enhanced image info

##### 12 Nov 2015 - v1.7.1
+ Changes to the way layouts are being switched, now faster and more efficient
+ Fixed issue with not reading image tags correctly when exif data was missing from image
+ Added Negate option to image tags filtering
+ Docks can now be nested to create more customized layouts
+ Some enhancements to Tags user experience and icons
+ Fixed issue with limited zoom functionality
+ Better error handling when reading corrupted images
+ New translations added

##### 8 Aug 2015 - v1.6.17
+ Image tags improvements and bug fixes
+ Changes to default key mapping
+ Small fixes to image extensions
+ Fixed issue with thumb label appearing after rename when labels are not displayed
+ Improvements to image feedback
+ Some dialog usability fixes
+ Added Negativity settings per color channel
+ Fixed colors manipulations for images with alpha channel and non animated GIF images
+ Other Bug fixes

##### Optional Dependencies
+ qt5-imageformats (TIFF and TGA support)
+ qt5-svg (SVG support)

##### Quick Build Instructions on Linux
```
$ tar -zxvf phototonic.tar.gz
$ cd phototonic
$ qmake
$ make
$ make install
$ sudo make install
```

##### Building on Windows
Building on Windows is only supported with mingw at the moment (the source code is probably compatible with msvc, but this was not tested yet).
First get the exiv2 library. Binary version is available from http://www.exiv2.org/download.html (download mingw version) or build it manually.

Note that Qt libraries must be built against the same major mingw version as exiv2 is built against (i.e. Qt built with mingw 5 and higher won't be compatible with exiv2 built with mingw 4.9).
Currently exiv2 binary package for mingw is built with mingw 4.9 therefore the latest compatible Qt version available in binary is 5.6.3 (available via Qt Maintenance Tool).

If using the binary package from exiv2 website, unpack the `mingw` directory to the root of the repository (only mingw/lib and mingw/include are essential).
Then build phototonic as usual - via qmake + mingw32-make in the console, or via QtCreator (remember to choose the compatible Qt Kit).
