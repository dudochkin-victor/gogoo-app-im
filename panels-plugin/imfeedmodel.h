/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef __imfeedmodel_h
#define __imfeedmodel_h

#include <feedmodel.h>

#include "imfeedmodelitem.h"
#include "../telepathy-qml-lib/panelschannelobserver.h"
#include "../telepathy-qml-lib/notificationmanager.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/ReceivedMessage>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/Types>

#include <TelepathyQt4Yell/CallChannel>

#include <QDBusServiceWatcher>

class IMFeedModel: public McaFeedModel
{
    Q_OBJECT

public:
    explicit IMFeedModel(PanelsChannelObserver *observer, Tp::AccountPtr account, QObject *parent = NULL);
    ~IMFeedModel();

    // You need to override rowCount to provide the number of data items you
    //   are currently exposing through the model. See QAbstractItemModel
    //   documentation.
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    // Although the feed model is a list model, meaning there is only one
    //   column of data, there are a number of roles defined in the
    //   feedmodel.h header file. You will need to consider which of
    //   them you need to provide in your implementation.
    // Your data should be provided
    QVariant data(const QModelIndex &index, int role) const;

    Tp::AccountPtr account(void) const;

    enum FeedType {
        MessageType = 0,
        RequestType,
        InformationType,
        CallType,
        FileTransferType
    };

Q_SIGNALS:
    void acceptContact(Tp::AccountPtr account, QString contactId);
    void rejectContact(Tp::AccountPtr account, QString contactId);
    void applicationRunningChanged(bool running);

public Q_SLOTS:
    void onPresencePublicationRequested(const Tp::Contacts &contacts);

protected slots:

    void onNewTextChannel(const Tp::AccountPtr &account, const Tp::TextChannelPtr &textChannel);
    void onNewCallChannel(const Tp::AccountPtr &account, const Tpy::CallChannelPtr &callChannel);
    void onNewFileTransferChannel(const Tp::AccountPtr &account, const Tp::IncomingFileTransferChannelPtr &fileTransferChannel);
    void onMessageReceived(const Tp::ReceivedMessage &message);

    void onInformationReceived();
    void onContactUpgraded(Tp::PendingOperation *op);

    // There are three standard actions currently defined. You should always
    //   provide an McaActions object through the CommonActionsRole. You must
    //   at least handle the action "default". This will be called if the user
    //   clicks on your item. You should then launch the application to view
    //   this item in detail. The uniqueid given is the one you provided for
    //   the item in RequiredUniqueIdRole.
    // For normal data items, you provide the type "content" in
    //   RequiredTypeRole. You must then handle the "default" action.
    // For a data item associated with a picture or set of pictures, such as
    //   a photo upload event on a social networking site, if you can provide
    //   a thumbnail of the picture(s), instead use the "picture" type in
    //   RequiredTypeRole and a file:// URL for GenericPictureUrlRole. Again
    //   you must handle the "default" action.
    // The third defined type is "request", which can be used for presenting
    //   a decision to the user like a friend request. You can choose to
    //   provide localized text for two buttons with GenericAcceptTextRole and
    //   GenericRejectTextRole, or leave them blank to get our default text
    //   "Accept" or "Reject" (localized). When you provide a request, you
    //   must be ready to handle "accept" and "reject" actions. You may
    //   optionally handle the "default" action for these items as well, which
    //   may occur if the user clicks outside the button area.
    // You should test the action string and ignore any unknown ones.
    void performAction(QString action, QString uniqueid);
    void onItemChanged(int row);
    void onTextChannelReady(Tp::PendingOperation *op);
    void onCallChannelReady(Tp::PendingOperation *op);
    void onFileTransferChannelReady(Tp::PendingOperation *op);
    void onAllKnownContactsChanged(const Tp::Contacts &contactsAdded,
                                   const Tp::Contacts &contactsRemoved,
                                   const Tp::Channel::GroupMemberChangeDetails &details);
    void onPublishStateChanged(Tp::Contact::PresenceState state);
    void onServiceRegistered();
    void onServiceUnregistered();

protected:
    void insertItem(IMFeedModelItem *item);
    void removeItem(IMFeedModelItem *item);
    void createNewChannelItem(const Tp::ChannelPtr &channel, const FeedType &type);

private:
    PanelsChannelObserver *mObserver;
    Tp::AccountPtr mAccount;
    QString mAccountId;
    QString mServiceIcon;
    QString mServiceName;
    QList<IMFeedModelItem *> mItems;
    NotificationManager mNotificationManager;
    bool mPlaceNotifications;
    QDBusServiceWatcher mIMServiceWatcher;
};

#endif  // __emailmodel_h
