#include <QtGlobal>
#include "Trashcan.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_DARWIN)

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTextStream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

static Trash::Result moveToTrashDir(const QString& filePath, const QDir& trashDir, QString& error)
{
    const QDir trashInfoDir = QDir(trashDir.filePath("info"));
    const QDir trashFilesDir = QDir(trashDir.filePath("files"));
    if (trashInfoDir.mkpath(".") && trashFilesDir.mkpath(".")) {
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        QString infoFileName = fileName + ".trashinfo";
        int fd;
        const int flag = O_CREAT | O_WRONLY | O_EXCL;
        const int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        for (unsigned int n = 2; trashFilesDir.exists(fileName) ||
             ((fd = open(trashInfoDir.filePath(infoFileName).toUtf8().data(), flag, mode)) == -1 && errno == EEXIST); ++n) {
            fileName = QString("%1.%2.%3").arg(fileInfo.baseName(), QString::number(n), fileInfo.completeSuffix());
            infoFileName = fileName + ".trashinfo";
        }
        if (fd == -1) {
            error = strerror(errno);
            return Trash::Error;
        }
        const QString moveHere = trashFilesDir.filePath(fileName);
        const QString deletionDate = QDateTime::currentDateTime().toString(Qt::ISODate);
        QString escapedPath = filePath;
        escapedPath.replace('\\', "\\").replace('\n', "\\n").replace('\r', "\\r").replace('\t', "\\t");
        QFile infoFile;
        if (infoFile.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
            QTextStream out(&infoFile);
            out << "[Trash Info]\nPath=" << escapedPath << "\nDeletionDate=" << deletionDate << '\n';
        } else {
            error = infoFile.errorString();
            return Trash::Error;
        }


        if (QDir().rename(filePath, moveHere)) {
            return Trash::Success;
        } else {
            error = QString("Could not rename %1 to %2").arg(filePath, moveHere);
            return Trash::Error;
        }
    } else {
        error = "Could not set up trash subdirectories";
        return Trash::Error;
    }
}

Trash::Result Trash::moveToTrash(const QString &path, QString &error, Trash::Options trashOptions)
{
    if (path.isEmpty()) {
        error = "Path is empty";
        return Trash::Error;
    }
    const QString filePath = QFileInfo(path).absoluteFilePath();
    const QStorageInfo filePathStorage(filePath);
    if (!filePathStorage.isValid()) {
        error = "Could not get device of the file being trashed";
        return Trash::Error;
    }
    const QString homeDataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QDir homeDataDirectory(homeDataLocation);
    if (homeDataLocation.isEmpty() || !homeDataDirectory.exists()) {
        error = "Could not get home data folder";
        return Trash::Error;
    }

    if (QStorageInfo(homeDataLocation) == filePathStorage || trashOptions == Trash::ForceDeletionToHomeTrash) {
        const QDir homeTrashDirectory = QDir(homeDataDirectory.filePath("Trash"));
        if (homeTrashDirectory.mkpath(".")) {
            return moveToTrashDir(filePath, homeTrashDirectory, error);
        } else {
            error = "Could not ensure that home trash directory exists";
            return Trash::Error;
        }
    } else {
        const QDir topdir = QDir(filePathStorage.rootPath());
        const QDir topdirTrash = QDir(topdir.filePath(".Trash"));
        struct stat trashStat;
        if (lstat(topdirTrash.path().toUtf8().data(), &trashStat) == 0) {
            // should be a directory, not link, and have sticky bit
            if (S_ISDIR(trashStat.st_mode) && !S_ISLNK(trashStat.st_mode) && (trashStat.st_mode & S_ISVTX)) {
                const QString subdir = QString::number(getuid());
                if (topdirTrash.mkpath(subdir)) {
                    return moveToTrashDir(filePath, QDir(topdirTrash.filePath(subdir)), error);
                }
            }
        }
        // if we're still here, $topdir/.Trash does not exist or failed some check
        QDir topdirUserTrash = QDir(topdir.filePath(QString(".Trash-%1").arg(getuid())));
        if (topdirUserTrash.mkpath(".")) {
            return moveToTrashDir(filePath, topdirUserTrash, error);
        }
        error = "Could not find trash directory for the disk where the file resides";
        return Trash::NeedsUserInput;
    }
}

#else

Trash::Result Trash::moveToTrash(const QString &path, QString &error, Trash::Options trashOptions)
{
    error = "Putting files into trashcan is not supported for this platform yet";
    return Trash::Error;
}

#endif
