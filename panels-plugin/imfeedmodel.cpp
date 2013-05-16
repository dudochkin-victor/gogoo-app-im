/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imfeedmodel.h"
#include <actions.h>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Feature>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>

#include <QDebug>

#include <QDateTime>
#include <QStringList>

IMFeedModel::IMFeedModel(PanelsChannelObserver *observer, Tp::AccountPtr account, QObject *parent)
    : McaFeedModel(parent),
      mObserver(observer),
      mAccount(account),
      mAccountId(account->uniqueIdentifier()),
      mNotificationManager(this),
      mPlaceNotifications(true),
      mIMServiceWatcher(this)
{
    mNotificationManager.setApplicationActive(false);
    mIMServiceWatcher.setConnection(QDBusConnection::sessionBus());
    mIMServiceWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    mIMServiceWatcher.addWatchedService("org.freedesktop.Telepathy.Client.MeeGoIM");
    connect(&mIMServiceWatcher,
            SIGNAL(serviceRegistered(QString)),
            SLOT(onServiceRegistered()));
    connect(&mIMServiceWatcher,
            SIGNAL(serviceUnregistered(QString)),
            SLOT(onServiceUnregistered()));
    connect(observer,
            SIGNAL(newTextChannel(Tp::AccountPtr,Tp::TextChannelPtr)),
            SLOT(onNewTextChannel(Tp::AccountPtr,Tp::TextChannelPtr)));
    connect(observer,
            SIGNAL(newCallChannel(Tp::AccountPtr,Tpy::CallChannelPtr)),
            SLOT(onNewCallChannel(Tp::AccountPtr,Tpy::CallChannelPtr)));
    connect(observer,
            SIGNAL(newFileTransferChannel(Tp::AccountPtr,Tp::IncomingFileTransferChannelPtr)),
            SLOT(onNewFileTransferChannel(Tp::AccountPtr,Tp::IncomingFileTransferChannelPtr)));
}

IMFeedModel::~IMFeedModel()
{
}

int IMFeedModel::rowCount(const QModelIndex &parent) const
{
    // The current UI plans only show at most 30 items from your model in
    //   a panel, and more likely 10 or fewer. So if there are performance
    //   improvements you can make by pruning down the queries you make to
    //   your back end, that is a guideline. However, if setSearchText is
    //   called, then you should try to search as widely as you can for
    //   matching text.
    // If you are given limit hints with the setHints function, you can also
    //   use those to help tune how much information you bother exposing
    //   because that will tell you how much will really be used.
    // When there is no search text set, your model should also restrict its
    //   queries to items from the last 30 days, if that is helpful for
    //   performance. Older items may be ignored.
    Q_UNUSED(parent);
    return mItems.count();
}

QVariant IMFeedModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= mItems.count())
        return QVariant();

    IMFeedModelItem *item = mItems.at(row);
    switch (role) {
    case RequiredTypeRole: {
        // Types defined currently are "content", "picture", and "request";
        //   see emailmodel.h for more information.
        int type = item->itemType();
        if (type == MessageType || type == InformationType) {
            return QString("content");
        } else if (type == RequestType) {
            return QString("request");
        } else {
            return QVariant();
        }
        break;
    }

    case RequiredUniqueIdRole:
        // RequiredUniqueIdRole is required, and the id you provide must be
        //   unique across this service and remain consistent for this data
        //   item across reboots, etc.
        // It doesn't have to be a true Uuid, although that would be fine.
        return item->uniqueId();

    case RequiredTimestampRole:
        // Each item should have a timestamp associated, and the model
        //   should be sorted by timestamp, most recent first.
        return item->timestamp();

    case GenericTitleRole:
        // The basic view for a content item contains two text fields,
        //   this one for the item title, plus the content preview (below).
        return item->contactName();

    case GenericContentRole:
        // The basic view for a content item contains two text fields,
        //   the item title (above), plus this content preview. The string
        //   should be limited to a few lines of text, maybe 160 or 256
        //   characters. More will be ignored.
        return item->message();

    case GenericAvatarUrlRole:
        // This field is for a thumbnail image of the message sender.
        return item->avatarUrl();

    case CommonActionsRole:
        // This is required to respond to user actions on your data item.
        return QVariant::fromValue<McaActions*>(item->actions());

    case GenericPictureUrlRole:
        return QString();

    case GenericRelevanceRole:
        return item->relevance();

    default:
        // There are a few other optional roles... you can provide a
        //   float "relevance" of the item from 0.0 (low) to 1.0 (high);
        //   for future use. You can set custom text for accept/reject
        //   buttons (localized!) for a "request" type item. You can
        //   provide a "picture url" if there is picture associated with
        //   the item, such as a photo upload notification.
        return QVariant();
    };
}

void IMFeedModel::performAction(QString action, QString uniqueid)
{
    // This is a slot connected to the signal for all items, so we need to
    //   figure out which one it applies to based on the uniqueid paramater.
    //   Alternately, you could connect to a slot on an individual item, if
    //   you make it a QObject, and you don't need that parameter.
    qDebug() << "Action" << action << "called for im" << uniqueid;

    //look for the model item
    foreach (IMFeedModelItem *item, mItems) {
        if (item->uniqueId() == uniqueid) {
            if (action == "accept") {
                emit acceptContact(mAccount, item->contactId());
                removeItem(item);
                break;
            } else if (action == "reject") {
                emit rejectContact(mAccount, item->contactId());
                removeItem(item);
                break;
            } else if (action == "default") {
                QString cmd;
                QString parameter;
                switch (item->itemType()) {
                   case MessageType: {
                       cmd = QString("show-chat");
                       parameter = QString(mAccountId + "&" + item->contactId());
                       break;
                   }
                   case RequestType:
                   case InformationType:
                   default:
                   {
                       cmd = QString("show-contacts");
                       parameter = mAccountId;
                       break;
                   }
                }
                QString executable("meego-qml-launcher");
                QStringList parameters;
                parameters << "--app" << "meego-app-im";
                parameters << "--opengl" << "--fullscreen";
                parameters << "--cmd" << cmd;
                parameters << "--cdata" << parameter;
                QProcess::startDetached(executable, parameters);
            }
        }
    }
}

void IMFeedModel::onMessageReceived(const Tp::ReceivedMessage &message)
{
    Tp::ContactPtr contact = message.sender();

    // do not log old messages
    if (message.isRescued() || message.isScrollback()) {
        return;
    }

    // do not log delivery reports
    if(message.messageType() == Tp::ChannelTextMessageTypeDeliveryReport) {
        return;
    }

    //this is to allow parsing later on
    QString token = QString(MessageType + "&" + mAccountId + "&" + message.sender()->id() + "&" + message.messageToken());

    if (!contact->actualFeatures().contains(Tp::Contact::FeatureAlias)
            || !contact->actualFeatures().contains(Tp::Contact::FeatureAvatarData)) {
        //upgrade contacts
        Tp::Features features;
        features << Tp::Contact::FeatureAlias
            << Tp::Contact::FeatureAvatarData;
        QList<Tp::ContactPtr>  contacts;
        contact->setProperty("messagetext", message.text());
        contact->setProperty("timesent", message.sent());
        contact->setProperty("feedtype", MessageType);
        contact->setProperty("messagetoken", token);
        contacts.append(contact);

        connect(contact->manager()->upgradeContacts(contacts, features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactUpgraded(Tp::PendingOperation*)));
    } else {
        QString avatar = contact->avatarData().fileName;
        QString alias = contact->alias();

        IMFeedModelItem *item = new IMFeedModelItem(mAccountId, alias, contact->id(),message.text(), message.sent(),
                                                    avatar, new McaActions(), MessageType, token);
        connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                this, SLOT(performAction(QString,QString)));

        insertItem(item);

        if (mPlaceNotifications) {
            mNotificationManager.notifyPendingMessage(mAccountId,
                                                      contact->id(),
                                                      alias,
                                                      message.sent(),
                                                      message.text());
        }
    }
}

void IMFeedModel::onNewTextChannel(const Tp::AccountPtr &account, const Tp::TextChannelPtr &textChannel)
{
    if (account->uniqueIdentifier() == mAccountId) {
        // enable the features we need to receive incoming messages
        connect(textChannel->becomeReady(Tp::Features()
                        << Tp::TextChannel::FeatureCore
                        << Tp::TextChannel::FeatureMessageQueue
                        << Tp::TextChannel::FeatureMessageCapabilities
                        << Tp::TextChannel::FeatureMessageSentSignal),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onTextChannelReady(Tp::PendingOperation*)));
    }
}

void IMFeedModel::onNewCallChannel(const Tp::AccountPtr &account, const Tpy::CallChannelPtr &callChannel)
{
    if (account->uniqueIdentifier() == mAccountId) {
        // enable the features we need to receive incoming messages
        connect(callChannel->becomeReady(Tp::Features()
                        << Tpy::CallChannel::FeatureCore
                        << Tpy::CallChannel::FeatureConferenceInitialInviteeContacts),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onCallChannelReady(Tp::PendingOperation*)));
    }
}

void IMFeedModel::onNewFileTransferChannel(const Tp::AccountPtr &account, const Tp::IncomingFileTransferChannelPtr &fileTransferChannel)
{
    if (account->uniqueIdentifier() == mAccountId) {
        // enable the features we need to receive incoming messages
        connect(fileTransferChannel->becomeReady(Tp::Features()
                        << Tp::IncomingFileTransferChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onFileTransferChannelReady(Tp::PendingOperation*)));
    }
}

void IMFeedModel::onContactUpgraded(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Contacts cannot be upgraded";
        return;
    }

    Tp::PendingContacts *pendingContacts = qobject_cast<Tp::PendingContacts *>(op);
    QList<Tp::ContactPtr> contacts = pendingContacts->contacts();

    // go through each contact in the model and update the data of the upgraded contact
    foreach (Tp::ContactPtr contact, contacts) {
        int type = contact->property("feedtype").toInt();
        if (type == MessageType) {
            QString messageText = contact->property("messagetext").toString();
            QString token = contact->property("messagetoken").toString();
            QDateTime sent = contact->property("timesent").toDateTime();

            if (!messageText.isEmpty()) {
                IMFeedModelItem *item = new IMFeedModelItem(mAccountId,
                                                            contact->alias(),
                                                            contact->id(),
                                                            messageText,
                                                            sent,
                                                            contact->avatarData().fileName,
                                                            new McaActions(),
                                                            MessageType,
                                                            token);
                connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                        this, SLOT(performAction(QString,QString)));
                insertItem(item);

                if (mPlaceNotifications) {
                    mNotificationManager.notifyPendingMessage(mAccountId,
                                                              contact->id(),
                                                              contact->alias(),
                                                              sent,
                                                              messageText);
                }
            }
        } else if (type == InformationType // an information message
                  || (type == RequestType && // or there was a request and it was accepted by the user
                      contact->publishState() == Tp::Contact::PresenceStateYes)) {
            QString messageText =  qApp->translate("Message indicating the contact has been added",
                                                           "has been added as contact");

            QString token = QString(InformationType + "&" + mAccountId + "&" + contact->id() + QDateTime::currentDateTime().toString(Qt::ISODate));
            IMFeedModelItem *item = new IMFeedModelItem(mAccountId,
                                                        contact->alias(),
                                                        contact->id(),
                                                        messageText,
                                                        QDateTime(),
                                                        contact->avatarData().fileName,
                                                        new McaActions(),
                                                        InformationType,
                                                        token);
            connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                    this, SLOT(performAction(QString,QString)));
            insertItem(item);
        } else if (type == CallType) {
            QString messageText =  tr("Incoming call from %1").arg(contact->alias());

            QString token = QString(CallType + "&" + mAccountId + "&" + contact->id() + QDateTime::currentDateTime().toString(Qt::ISODate));
            IMFeedModelItem *item = new IMFeedModelItem(mAccountId,
                                                        contact->alias(),
                                                        contact->id(),
                                                        messageText,
                                                        QDateTime(),
                                                        contact->avatarData().fileName,
                                                        new McaActions(),
                                                        InformationType,
                                                        token);
            connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                    this, SLOT(performAction(QString,QString)));
            insertItem(item);
        } else if (type == FileTransferType) {
            QString messageText = tr("Incoming file transfer from %1").arg(contact->alias());

            QString token = QString(FileTransferType + "&" + mAccountId + "&" + contact->id() + QDateTime::currentDateTime().toString(Qt::ISODate));
            IMFeedModelItem *item = new IMFeedModelItem(mAccountId,
                                                        contact->alias(),
                                                        contact->id(),
                                                        messageText,
                                                        QDateTime(),
                                                        contact->avatarData().fileName,
                                                        new McaActions(),
                                                        InformationType,
                                                        token);
            connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                    this, SLOT(performAction(QString,QString)));
            insertItem(item);
        }
    }
}

void IMFeedModel::insertItem(IMFeedModelItem *item)
{
    qDebug("item inserted");
    beginInsertRows(QModelIndex(), 0, 0);
    mItems.prepend(item);
    endInsertRows();
}

void IMFeedModel::removeItem(IMFeedModelItem *item)
{
    int row = mItems.indexOf(item);
    beginRemoveRows(QModelIndex(), row, row);
    mItems.removeOne(item);
    endRemoveRows();
}

void IMFeedModel::onItemChanged(int row)
{
    QModelIndex index = createIndex(row, 0, mItems.at(row));
    emit dataChanged(index, index);
}

void IMFeedModel::onPresencePublicationRequested(const Tp::Contacts &contacts)
{
    // Add new items
    foreach (Tp::ContactPtr contact, contacts) {
        QString token = QString(InformationType + "&" + mAccountId + "&" + contact->id() + QDateTime::currentDateTime().toString(Qt::ISODate));
        contact->setProperty("feedtype", RequestType);
        contact->setProperty("messagetoken", token);
        IMFeedModelItem *item = new IMFeedModelItem(mAccountId,
                                                    contact->id(),
                                                    contact->id(),
                                                    tr("Add as friend?"),
                                                    QDateTime::currentDateTime(),
                                                    QString("image://meegotheme/widgets/common/avatar/avatar-default"),
                                                    new McaActions(),
                                                    RequestType,
                                                    token);
        connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                this, SLOT(performAction(QString,QString)));
        insertItem(item);
    }
}

void IMFeedModel::onServiceRegistered()
{
    mPlaceNotifications = false;
    // remove all notifications
    mNotificationManager.clear();
    emit applicationRunningChanged(true);
}

void IMFeedModel::onServiceUnregistered()
{
    // if the application quits, we should continue placing notifications
    mPlaceNotifications = true;
    emit applicationRunningChanged(false);
}

Tp::AccountPtr IMFeedModel::account(void) const
{
    return mAccount;
}

void IMFeedModel::onTextChannelReady(Tp::PendingOperation *op)
{
    qDebug("IMFeedModel::onTextChannelReady: channel ready");

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "IMFeedModel::onTextChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

//    Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(pr->object());
//    if (textChannel.isNull()) {
//        qDebug() << "IMFeedModel::onTextChannelReady: channel invalid";
//        return;
//    }

//    //flush the queue and enter all messages into the model
//    // display messages already in queue
//    qDebug("message queue: %d", textChannel->messageQueue().count());
//    foreach (Tp::ReceivedMessage message, textChannel->messageQueue()) {
//        onMessageReceived(message);
//    }

//    //connect to incoming messages
//    connect(textChannel.data(),
//                SIGNAL(messageReceived(Tp::ReceivedMessage)),
//                SLOT(onMessageReceived(Tp::ReceivedMessage))); //DV
}

void IMFeedModel::onCallChannelReady(Tp::PendingOperation *op)
{
    qDebug("IMFeedModel::onCallChannelReady: channel ready");

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "IMFeedModel::onCallChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

//    Tpy::CallChannelPtr callChannel = Tpy::CallChannelPtr::dynamicCast(pr->object());
//    if (callChannel.isNull()) {
//        qDebug() << "IMFeedModel::onCallChannelReady: channel invalid";
//        return;
//    }

//    createNewChannelItem(callChannel, CallType);//DV
}

void IMFeedModel::onFileTransferChannelReady(Tp::PendingOperation *op)
{
    qDebug("IMFeedModel::onFileTransferChannelReady: channel ready");

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "IMFeedModel::onFileTransferChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

//    Tp::IncomingFileTransferChannelPtr fileTransferChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(pr->object());
//    if (fileTransferChannel.isNull()) {
//        qDebug() << "IMFeedModel::onFileTransferChannelReady: channel invalid";
//        return;
//    }

//    createNewChannelItem(fileTransferChannel, FileTransferType);//DV

}

void IMFeedModel::createNewChannelItem(const Tp::ChannelPtr &channel, const FeedType &type)
{

    Tp::ContactPtr contact;
    foreach(Tp::ContactPtr channelContact, channel->groupContacts().toList()) {
        if(contact != channel->groupSelfContact()) {
            contact = channelContact;
            break;
        }
    }

    if(contact.isNull()) {
        return;
    }

    QString token = QString(type + "&" + mAccountId + "&" + contact->id() + QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!contact->actualFeatures().contains(Tp::Contact::FeatureAlias)
            || !contact->actualFeatures().contains(Tp::Contact::FeatureAvatarData)) {
        //upgrade contacts
        Tp::Features features;
        features << Tp::Contact::FeatureAlias
            << Tp::Contact::FeatureAvatarData;
        QList<Tp::ContactPtr>  contacts;
        contact->setProperty("timesent", QDateTime::currentDateTime());
        contact->setProperty("feedtype", type);
        contact->setProperty("messagetoken", token);
        contacts.append(contact);

        connect(contact->manager()->upgradeContacts(contacts, features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactUpgraded(Tp::PendingOperation*)));
    } else {
        QString avatar = contact->avatarData().fileName;
        QString alias = contact->alias();
        QString message;

        switch(type) {
        case CallType:
            message = tr("Incoming call from %1").arg(alias);
            break;
        case FileTransferType:
            message = tr("Incoming file transfer from %1").arg(contact->alias());
            break;
        default:
            message.clear();
            break;
        }

        IMFeedModelItem *item = new IMFeedModelItem(mAccountId, alias, contact->id(), message, QDateTime::currentDateTime(),
                                                    avatar, new McaActions(), MessageType, token);
        connect(item->actions(), SIGNAL(standardAction(QString,QString)),
                this, SLOT(performAction(QString,QString)));

        insertItem(item);
    }
}

void IMFeedModel::onAllKnownContactsChanged(const Tp::Contacts &contactsAdded,
                                            const Tp::Contacts &contactsRemoved,
                                            const Tp::Channel::GroupMemberChangeDetails &details)
{
    Q_UNUSED(contactsRemoved);
    Q_UNUSED(details);

    //go over each new contact and insert the feedtype property
    //that way, when its publish state changes to yes, we can detect them and add only those to the model
    foreach (Tp::ContactPtr contact, contactsAdded) {
        contact->setProperty("feedtype", InformationType);

        connect(contact.data(),
                SIGNAL(publishStateChanged(Tp::Contact::PresenceState, QString)),
                SLOT(onPublishStateChanged(Tp::Contact::PresenceState)));
    }
}

void IMFeedModel::onPublishStateChanged(Tp::Contact::PresenceState state)
{
    //only process if it was added and it's a contact marked as InformationType
    if (state == Tp::Contact::PresenceStateYes) {
        Tp::Contact *contact = qobject_cast<Tp::Contact *>(sender());
        if (contact->property("feedtype").toInt() == InformationType) {
            Tp::Features features;
            features << Tp::Contact::FeatureAlias
                << Tp::Contact::FeatureAvatarData;
            QList<Tp::ContactPtr>  contacts;
            contacts.append(Tp::ContactPtr(contact));

            //upgrade the contact
            //it will be added to the model once it's upgraded
            connect(contact->manager()->upgradeContacts(contacts, features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onContactUpgraded(Tp::PendingOperation*)));
        }
    }
}
