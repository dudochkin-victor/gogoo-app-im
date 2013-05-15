/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imavatarimageprovider.h"
#include <TelepathyQt4/AccountManager>

IMAvatarImageProvider::IMAvatarImageProvider(const Tp::AccountManagerPtr &am) :
    Tpy::AvatarImageProvider(am)
{
}

void IMAvatarImageProvider::registerProvider(QDeclarativeEngine *engine, const Tp::AccountManagerPtr &am)
{
    engine->addImageProvider(QString::fromLatin1("avatars"), new IMAvatarImageProvider(am));
}

QImage IMAvatarImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString realId = id.split("?")[0];
    return Tpy::AvatarImageProvider::requestImage(realId, size, requestedSize);
}
