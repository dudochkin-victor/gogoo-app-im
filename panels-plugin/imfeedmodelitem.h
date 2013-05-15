/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMFEEDMODELITEM_H
#define IMFEEDMODELITEM_H

#include <actions.h>

#include <QObject>
#include <QDateTime>

class IMFeedModelItem : public QObject
{
    Q_OBJECT

public:
    IMFeedModelItem(QString accountId, QString contactName, QString contactId, QString message, QDateTime time, QString avatar, McaActions *actions, int type, QString token);
    ~IMFeedModelItem();

    QString contactName(void) const;
    QString contactId(void) const;
    QString message(void) const;
    QDateTime timestamp(void) const;
    QString avatarUrl(void) const;
    int itemType(void) const;
    McaActions *actions(void);
    QString uniqueId(void) const;
    qreal relevance(void) const;

    void setContactName(const QString &name);
    void setAvatarUrl(const QString &url);

private:
    int mItemType;
    QString mAccountId;
    QString mContactName;
    QString mContactId;
    QString mMessage;
    QDateTime mTimestamp;
    QString mAvatarUrl;
    McaActions *mActions;
    QString mUniqueId;
    qreal mRelevance;
};

#endif // IMFEEDMODELITEM_H
