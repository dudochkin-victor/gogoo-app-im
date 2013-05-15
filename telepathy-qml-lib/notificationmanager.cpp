/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "notificationmanager.h"
#include <QDebug>
#include <QApplication>
#include <MNotification>

NotificationItem::~NotificationItem()
{
    // no need to delete the pointer, QSharedPointer will take care of it
}

NotificationManager::NotificationManager(QObject *parent) :
    QObject(parent), mChatActive(0), mApplicationActive(true)
{
    mAppWindow = QApplication::topLevelWidgets()[0];
}

QString NotificationManager::currentAccount() const
{
    return mCurrentAccount;
}

QString NotificationManager::currentContact() const
{
    return mCurrentContact;
}

bool NotificationManager::chatActive() const
{
    return mChatActive > 0;
}

bool NotificationManager::applicationActive() const
{
    return mApplicationActive;
}

void NotificationManager::setCurrentAccount(const QString &account)
{
    mCurrentAccount = account;
    qDebug() << "NOTIFICATION: current account is" << account;
    processNotifications();
}

void NotificationManager::setCurrentContact(const QString &contact)
{
    mCurrentContact = contact;
    qDebug() << "NOTIFICATION: current contact is" << contact;
    processNotifications();
}

void NotificationManager::setChatActive(bool active)
{
    if (active)
        mChatActive++;
    else
        mChatActive--;
    qDebug() << "NOTIFICATION: chat is active:" << (active!=0);
    processNotifications();
}

void NotificationManager::setApplicationActive(bool active)
{
    mApplicationActive = active;
    qDebug() << "NOTIFICATION: application is active:" << active;
    processNotifications();
}

void NotificationManager::notifyPendingMessage(const QString &accountId,
                                               const QString &contactId,
                                               const QString &contactAlias,
                                               const QDateTime &time,
                                               const QString &message)
{
    qDebug() << "Notifying a message from contact" << contactId;
    placeNotification(NotificationItem::PendingChatMessage,
                      accountId,
                      contactId,
                      contactAlias,
                      time,
                      message);
}

void NotificationManager::notifyMissedCall(const QString &accountId,
                                           const QString &contactId,
                                           const QString &contactAlias,
                                           const QDateTime &time)
{
    qDebug() << "Notifying a missed call from contact" << contactId;
    placeNotification(NotificationItem::MissedCall,
                      accountId,
                      contactId,
                      contactAlias,
                      time);
}

void NotificationManager::notifyMissedVideoCall(const QString &accountId,
                                                const QString &contactId,
                                                const QString &contactAlias,
                                                const QDateTime &time)
{
    qDebug() << "Notifying a missed video call from contact" << contactId;
    placeNotification(NotificationItem::MissedVideoCall,
                      accountId,
                      contactId,
                      contactAlias,
                      time);
}

void NotificationManager::notifyIncomingFileTransfer(const QString &accountId,
                                                     const QString &contactId,
                                                     const QString &contactAlias,
                                                     const QDateTime &time,
                                                     const QString &fileName)
{
    qDebug() << "Notifying an incoming file transfer from contact" << contactId;

    placeNotification(NotificationItem::IncomingFileTransfer,
                      accountId,
                      contactId,
                      contactAlias,
                      time,
                      tr("%1 is sending you the file %2").arg(contactAlias, fileName));
}


void NotificationManager::processNotifications()
{
    // if the application is not active, we should notify everything
    // so not removing anything from the list
    if (!mApplicationActive) {
        return;
    }

    // if there is an active chat, we should remove all notifications
    // of the contact in the chat
    if (mChatActive) {
        // Remove all notifications for the contact we are chatting to
        QList<NotificationItem>::iterator it = mNotifications.begin();
        while (it != mNotifications.end()) {
            if ((*it).accountId == mCurrentAccount &&
                    (*it).contactId == mCurrentContact) {
                (*it).item->remove();
                it = mNotifications.erase(it);
            } else {
                ++it;
            }
        }

        return;
    }

    // if there is no active chat, we should remove notifications from the current account
    if (!mCurrentAccount.isEmpty()) {
        QList<NotificationItem>::iterator it = mNotifications.begin();
        while (it != mNotifications.end()) {
            if ((*it).accountId == mCurrentAccount) {
                (*it).item->remove();
                it = mNotifications.erase(it);
            } else {
                ++it;
            }
        }
        return;
    }

    // that's all folks
}


void NotificationManager::placeNotification(NotificationItem::NotificationType type,
                                            const QString &accountId,
                                            const QString &contactId,
                                            const QString &contactAlias,
                                            const QDateTime &time,
                                            const QString &message)
{
    int eventCount = 0;
    // TODO: check how we are supposed to use the time in the notifications
    Q_UNUSED(time)

    // remove previous notifications from that contact
    QList<NotificationItem>::iterator it = mNotifications.begin();
    while (it != mNotifications.end()) {
        if ((*it).accountId == accountId &&
                (*it).contactId == contactId) {
            eventCount = (*it).item->count();

            (*it).item->remove();
            it = mNotifications.erase(it);
        } else {
            ++it;
        }
    }

    // increate the event counter
    eventCount++;

    // if the application is currently active, we have to meet all the conditions
    // for displaying the notification
    if (mApplicationActive) {
        // if there is an active chat, we should not place notifications of that chat
        if (mChatActive &&
            accountId == mCurrentAccount &&
            contactId == mCurrentContact) {
            return;
        }
        // now if there is no chat active, we should only place
        // notifications of other accounts
        if (!mChatActive &&
            accountId == mCurrentAccount) {
            return;
        }
    }

    // check all the conditions before setting up a notification
    NotificationItem notification;
    notification.type = type;
    notification.accountId = accountId;
    notification.contactId = contactId;
    notification.message = message;
    notification.item = QSharedPointer<MNotification>(new MNotification(MNotification::ImEvent,
                                                                        contactAlias,
                                                                        message));

    notification.item->setCount(eventCount);

    // NOTE: The current theme can be found in the /meego/ux/theme gconf key, where
    //       the full path to the theme is /usr/share/themes/NAME.  I am only 'fixing'
    //       this to at least fall back to a known theme directory 
    QString icon("/usr/share/themes/meego-ux-theme/1024-600-10/images/im/");
    switch (type) {
    case NotificationItem::IncomingFileTransfer:
    case NotificationItem::PendingChatMessage:
        notification.item->setImage(icon + "icn_previouschats.png");
        break;
    case NotificationItem::MissedCall:
        notification.item->setImage(icon + "icn_missedaudiocall.png");
        break;
    case NotificationItem::MissedVideoCall:
        notification.item->setImage(icon + "icn_missedvideocall.pnh");
        break;
    }

    notification.item->publish();
    mNotifications.append(notification);

}

void NotificationManager::clear()
{
    foreach (NotificationItem notification, mNotifications) {
        notification.item->remove();
    }

    mNotifications.clear();
}
