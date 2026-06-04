#ifndef CONTACT_H
#define CONTACT_H

#include <QString>
#include <QIcon>
#include <QPixmap>
#include <QVariant>
#include <QMetaType>
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

Q_DECLARE_METATYPE(Contact)

#endif
