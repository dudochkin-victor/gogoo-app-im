/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMGROUPCHATMODELITEM_H
#define IMGROUPCHATMODELITEM_H

#include <TelepathyQt4/Account>
#include <TelepathyQt4/TextChannel>

class IMGroupChatModelItem : public QObject
{
    Q_OBJECT
public:
    explicit IMGroupChatModelItem(const QString &accountId, const Tp::TextChannelPtr &channel);

    Q_INVOKABLE virtual QVariant data(int role) const;

    Tp::TextChannelPtr channel() const;
    QString accountId() const;

Q_SIGNALS:
    void changed(IMGroupChatModelItem *);

public Q_SLOTS:
    void onChanged();

private Q_SLOTS:
    void onGroupMembersChanged();

private:
    QString mAccountId;
    Tp::TextChannelPtr mTextChannel;

};

#endif
