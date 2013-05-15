/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imfeedmodelitem.h"
#include "imfeedmodel.h"

IMFeedModelItem::IMFeedModelItem(QString accountId, QString contactName, QString contactId, QString message, QDateTime time, QString avatar, McaActions *actions, int type, QString token)
    : mItemType(type),
      mAccountId(accountId),
      mContactName(contactName),
      mContactId(contactId),
      mMessage(message),
      mTimestamp(time),
      mAvatarUrl(avatar),
      mActions(actions),
      mUniqueId(token),
      mRelevance(0)
{
    // if it is a request, assign more relevance to it
    if (type == IMFeedModel::RequestType) {
        mRelevance = 1.0;
    }

    // use a standard no avatar picture if empty
    if (mAvatarUrl.isEmpty()) {
        mAvatarUrl = "image://meegotheme/widgets/common/avatar/avatar-default";
    }

    // if timestamp is null, use current time
    if (mTimestamp.isNull()) {
        mTimestamp = QDateTime::currentDateTime();
    }

    // if token is empty, use contact id and current time
    if (mUniqueId.isEmpty()) {
        mUniqueId = QString(mItemType + "&&" + mContactId + mTimestamp.toString(Qt::ISODate));
    }
}

IMFeedModelItem::~IMFeedModelItem()
{
}

QString IMFeedModelItem::avatarUrl() const
{
    return mAvatarUrl;
}

QString IMFeedModelItem::contactName() const
{
    return mContactName;
}

QString IMFeedModelItem::contactId() const
{
    return mContactId;
}

QString IMFeedModelItem::message() const
{
    return mMessage;
}

QDateTime IMFeedModelItem::timestamp() const
{
    return mTimestamp;
}

int IMFeedModelItem::itemType() const
{
    return mItemType;
}

QString IMFeedModelItem::uniqueId() const
{
    return mUniqueId;
}


void IMFeedModelItem::setContactName(const QString &name)
{
    mContactName = name;
}

void IMFeedModelItem::setAvatarUrl(const QString &url)
{
    mAvatarUrl = url;
}

McaActions *IMFeedModelItem::actions()
{
    return mActions;
}

qreal IMFeedModelItem::relevance(void) const
{
    return mRelevance;
}
