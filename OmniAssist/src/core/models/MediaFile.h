#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <QString>
#include <QPixmap>

struct MediaFile {
    QString id;
    QString localPath;
    QString fileName;
    qint64 fileSize;
    QString fileType;
    QPixmap thumbnail;
};

#endif
