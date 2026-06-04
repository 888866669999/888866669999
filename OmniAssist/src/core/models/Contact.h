#ifndef CONTACT_H
#define CONTACT_H

#include <QString>
#include <QIcon>
#include <QPixmap>
#include <QVariant>
#include "Platform.h"

struct Contact {
    QString id;
    QString name;
    QString remark;
    Platform platform;
    QIcon platformIcon;
    QPixmap avatar;
    QVariant extra;
};

#endif
