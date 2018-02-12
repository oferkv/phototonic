#ifndef TRASHCAN_H
#define TRASHCAN_H

#include <QString>

namespace Trash {
    typedef enum
    {
        Success,
        Error,
        NeedsUserInput
    } Result;

    typedef enum
    {
        NoOptions = 0,
        ForceDeletionToHomeTrash = 1
    } Options;

    Trash::Result moveToTrash(const QString &filePath, QString &error, Options trashOptions = NoOptions);
}

#endif // TRASHCAN_H
