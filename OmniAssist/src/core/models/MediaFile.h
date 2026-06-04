#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <QString>
#include <QPixmap>
#include <QMetaType>

struct MediaFile {
    QString id;
    QString localPath;
    QString fileName;
    qint64 fileSize;
    QString fileType;
    QPixmap thumbnail;
};

Q_DECLARE_METATYPE(MediaFile)

#endif
