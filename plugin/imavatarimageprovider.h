/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMAVATARIMAGEPROVIDER_H
#define IMAVATARIMAGEPROVIDER_H

#include <TelepathyQt4Yell/Models/AvatarImageProvider>

class IMAvatarImageProvider : public Tpy::AvatarImageProvider
{
public:
    explicit IMAvatarImageProvider(const Tp::AccountManagerPtr &am);

    static void registerProvider(QDeclarativeEngine *engine, const Tp::AccountManagerPtr &am);

    virtual QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

#endif // IMAVATARIMAGEPROVIDER_H
