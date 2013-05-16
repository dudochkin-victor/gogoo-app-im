/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "chatagent.h"
#include <TelepathyQt/Channel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ReceivedMessage>
#include <QDebug>

ChatAgent::ChatAgent(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, QObject *parent)
    : QObject(parent),
      mAccount(account),
      mContact(contact),
      mHandleOwners(Tp::HandleOwnerMap()),
      mExistsChat(false),
      mModel(0),
      mPendingChannelRequest(0)
{
    qDebug() << "ChatAgent::ChatAgent: created for contact " << mContact->id();

    connect(mAccount.data(), SIGNAL(connectionChanged(const Tp::ConnectionPtr&)),
            SLOT(onAccountConnectionChanged(const Tp::ConnectionPtr&)));

    onAccountConnectionChanged(mAccount->connection());
    // todo check contact readyness
}

ChatAgent::ChatAgent(const Tp::AccountPtr &account, const QString &roomName, QObject *parent)
    : QObject(parent),
      mAccount(account),
      mHandleOwners(Tp::HandleOwnerMap()),
      mTextChannel(0),
      mExistsChat(false),
      mModel(0),
      mPendingChannelRequest(0)
{
    qDebug() << "ChatAgent::ChatAgent: created for account " << mAccount->uniqueIdentifier()
             << "for a group chat named " << roomName;

    mPendingChannelRequest = mAccount->ensureTextChatroom(roomName, QDateTime::currentDateTime(), QLatin1String("org.freedesktop.Telepathy.Client.MeeGoIM"));
    if (!mPendingChannelRequest) {
        emit error(tr("Unable to create text channel room %1")
                   .arg(roomName));
        endChat();
        return;
    }

    connect(mPendingChannelRequest,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingChanelRequestFinished(Tp::PendingOperation*)));
    connect(mPendingChannelRequest,
            SIGNAL(channelRequestCreated(Tp::ChannelRequestPtr)),
            SLOT(onPendingChanelRequestCreated(Tp::ChannelRequestPtr)));
    connect(mAccount.data(), SIGNAL(connectionChanged(const Tp::ConnectionPtr&)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    onAccountConnectionChanged(mAccount->connection());
}

ChatAgent::ChatAgent(const Tp::AccountPtr &account, const Tp::TextChannelPtr &channel, QObject *parent)
    : QObject(parent),
      mAccount(account),
      mHandleOwners(Tp::HandleOwnerMap()),
      mTextChannel(channel),
      mExistsChat(true),
      mModel(0),
      mPendingChannelRequest(0)
{
    qDebug() << "ChatAgent::ChatAgent: created for account " << mAccount->uniqueIdentifier()
             << "with an existing channel " << channel->objectPath();

    handleReadyChannel();
    emit isConferenceChanged();

    connect(mAccount.data(), SIGNAL(connectionChanged(const Tp::ConnectionPtr&)),
            SLOT(onAccountConnectionChanged(const Tp::ConnectionPtr&)));

    onAccountConnectionChanged(mAccount->connection());
}

ChatAgent::~ChatAgent()
{
    qDebug() << "ChatAgent::~ChatAgent: destroyed for contact " << mContact->id();

    // todo deallocate whatever is needed
    endChat();
}

void ChatAgent::startChat()
{
    qDebug() << "ChatAgent::startChat: contact=" << mContact->id();

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   QLatin1String(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   mContact->handle()[0]);
    mPendingChannelRequest = mAccount->ensureChannel(
                request,
                QDateTime::currentDateTime(),
                QLatin1String("org.freedesktop.Telepathy.Client.MeeGoIM"));
    if (!mPendingChannelRequest) {
        emit error(tr("Unable to create text channel for contact %1")
                       .arg(mContact->id()));
        endChat();
        return;
    }

    connect(mPendingChannelRequest,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingChanelRequestFinished(Tp::PendingOperation*)));
    connect(mPendingChannelRequest,
            SIGNAL(channelRequestCreated(Tp::ChannelRequestPtr)),
            SLOT(onPendingChanelRequestCreated(Tp::ChannelRequestPtr)));
}

void ChatAgent::onPendingChanelRequestFinished(Tp::PendingOperation *op)
{
    Q_ASSERT(op);

    qDebug() << "ChatAgent::onPendingChanelRequestFinished:";

    if (op->isError()) {
        emit error(tr("Unable to create channel: %1 - %2")
                      .arg(op->errorName())
                      .arg(op->errorMessage()));
        endChat();
        return;
    }

    mPendingChannelRequest = 0;
}

void ChatAgent::onPendingChanelRequestCreated(const Tp::ChannelRequestPtr &channelRequest)
{
    qDebug() << "ChatAgent::onPendingChanelRequestCreated:";

    if (channelRequest.isNull()) {
        emit error(tr("Unable to create channel"));
        endChat();
        return;
    }

    mPendingChannelRequest = 0;
}

void ChatAgent::onTextChannelAvailable(Tp::TextChannelPtr channel)
{
    qDebug() << "ChatAgent::onTextChannelAvailable: handling channel";

    if (channel.isNull()) {
        emit error(tr("Channel not available"));
        endChat();
        return;
    }

    mTextChannel = channel;
    emit isConferenceChanged();

    Tp::Features features;
    features << Tp::TextChannel::FeatureCore
             << Tp::TextChannel::FeatureChatState
             << Tp::TextChannel::FeatureMessageCapabilities
             << Tp::TextChannel::FeatureMessageQueue
             << Tp::TextChannel::FeatureMessageSentSignal;
    connect(channel->becomeReady(features),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTextChannelReady(Tp::PendingOperation*)));
}

void ChatAgent::onTextChannelReady(Tp::PendingOperation *op)
{
    qDebug() << "ChatAgent::onTextChannelReady";

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        emit error(tr("Unable to create text channel for contact %1 - %2 - %3")
                      .arg(!mContact.isNull() ? mContact->id() : "")
                      .arg(op ? op->errorName() : "")
                      .arg(op ? op->errorMessage() : ""));
        endChat();
        return;
    }

    //Q_ASSERT(mTextChannel == qobject_cast<Tp::TextChannel*>(pr->object()));

    handleReadyChannel();
}

void ChatAgent::handleReadyChannel()
{
    qDebug() << "ChatAgent::handleReadyChannel():";

    if (mTextChannel.isNull()) {
        emit error(tr("Unable to create text channel"));
        endChat();
        return;
    }
    //Q_ASSERT(mTextChannel->targetHandle() == mContact->handle()[0]);

    connect(mTextChannel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    mExistsChat = true;

    createModelForChat();
    emit chatCreated();
    emit existsChatChanged();
    emit pendingConversationsChanged();

    if(mTextChannel->isRequested()) {
        if(mTextChannel->isConference()) {
            emit requestedGroupChatCreated();
        } else {
            emit requestedChatCreated();
        }
    }

    // maintain a map of handles and contacts for group contacts
    if(mTextChannel->isConference()) {
        mHandleOwners = mTextChannel->groupHandleOwners();
        connect(mTextChannel.data(),
                SIGNAL(groupHandleOwnersChanged(Tp::HandleOwnerMap,Tp::UIntList,Tp::UIntList)),
                SLOT(onGroupHandleOwnersChanged(Tp::HandleOwnerMap,Tp::UIntList,Tp::UIntList)));


        foreach(uint handle, mHandleOwners) {
            onNewHandleOwner(handle);
        }
    }

}

void ChatAgent::onGroupHandleOwnersChanged(const Tp::HandleOwnerMap &owners, const Tp::UIntList &added, const Tp::UIntList &removed)
{
    Q_UNUSED(removed);
    mHandleOwners = owners;
    foreach(uint handle, added) {
        onNewHandleOwner(handle);
    }
}

void ChatAgent::onChannelInvalidated(Tp::DBusProxy *, const QString &errorName, const QString &errorMessage)
{
    qDebug() << "ChatAgent::onChannelInvalidated: channel became invalid:" <<
        errorName << "-" << errorMessage;

    if(isConference()) {
        emit error(tr("Invalidated text channel  %1 - %2 - %3")
                   .arg(mTextChannel->objectPath())
                   .arg(errorName)
                   .arg(errorMessage));
    } else {
        emit error(tr("Invalidated text channel for contact %1 - %2 - %3")
                   .arg(mContact->id())
                   .arg(errorName)
                   .arg(errorMessage));
    }

    endChat();
}

void ChatAgent::endChat()
{
    // check status

    if (mTextChannel.isNull()) {
        return;
    }

    disconnect(mTextChannel.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
               this, SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    // todo set status, clean objects, member vars
    // todo handle close
    mTextChannel->requestClose();
    mExistsChat = false;
    emit chatEnded();
}

bool ChatAgent::existsChat()
{
    return mExistsChat;
}

int ChatAgent::pendingConversations() const
{
    if (mModel) {
        return mModel->numPendingMessages();
    }

    return 0;
}

QString ChatAgent::lastPendingMessage() const
{
    if (!pendingConversations()) {
        return QString::null;
    }

    return mTextChannel->messageQueue().last().text();
}

QDateTime ChatAgent::lastPendingMessageSent() const
{
    if (!pendingConversations()) {
        return QDateTime();
    }

    return mTextChannel->messageQueue().first().sent();
}

IMConversationModel *ChatAgent::model() const
{
    return mModel;
}

Tp::TextChannelPtr ChatAgent::textChannel() const
{
    return mTextChannel;
}

QString ChatAgent::channelPath() const
{
    if(mTextChannel) {
        return mTextChannel->objectPath();
    }
    return QString();
}

bool ChatAgent::isConference() const
{
    if(mTextChannel) {
        return mTextChannel->isConference();
    }
    return false;
}

bool ChatAgent::canAddContacts() const
{
    if(mTextChannel) {
        return mTextChannel->groupCanAddContacts();
    }
    return false;
}

bool ChatAgent::isConnectionGroupChatCapable() const
{
    if(mTextChannel) {
        return (mTextChannel->connection()->capabilities().conferenceTextChats()
                || mTextChannel->connection()->capabilities().conferenceTextChatrooms());
    }
    return false;
}

void ChatAgent::createModelForChat()
{
    qDebug() << "ChatAgent::createModelForChat:";

    if (mTextChannel.isNull()) {
        qDebug() << "ChatAgent::createModelForChat: error text channel is invalid";
        return;
    }

    if (mTextChannel->connection().isNull()) {
        qDebug() << "ChatAgent::createModelForChat: connection is invalid";
        return;
    }

    qDebug() << " ChatAgent::createModelForChat: "
             << " connection=" << mTextChannel->connection().data()
             << " cmName=" << mTextChannel->connection()->cmName()
             << " protocolName=" << mTextChannel->connection()->protocolName()
             << " features=" << mTextChannel->connection()->actualFeatures();

    if (mTextChannel->connection()->selfContact().isNull()) {
        qDebug() << "ChatAgent::createModelForChat: self contact is invalid";
        return;
    }
    mModel = new IMConversationModel(mAccount,
        mTextChannel->connection()->selfContact(),
        mContact,
        mTextChannel,
        this);
    if (mModel) {
        connect(mModel,
                SIGNAL(numPendingMessagesChanged()),
                SIGNAL(pendingConversationsChanged()));
    }
}

Tp::AccountPtr ChatAgent::account() const
{
    return mAccount;
}

Tp::ContactPtr ChatAgent::contact() const
{
    return mContact;
}

SimpleContactsListModel *ChatAgent::contactsModel()
{
    if(mTextChannel) {
        Tp::Contacts contacts = mTextChannel->groupContacts();
        //convert to list and remove self contact
        QList<Tp::ContactPtr> contactsList = contacts.toList();
        foreach(Tp::ContactPtr contact, contactsList) {
            if(contact == mTextChannel->groupSelfContact()) {
                contactsList.removeOne(contact);
            }
        }
        return new SimpleContactsListModel(contactsList, this);
    }
    return 0;
}

void ChatAgent::addContacts(const QList<Tp::ContactPtr> contacts)
{
    if(mTextChannel && mTextChannel->groupCanAddContacts()) {
        mTextChannel->groupAddContacts(contacts);
    }
}

uint ChatAgent::globalHandle(const uint &channelHandle) const
{
    if(mHandleOwners.isEmpty()) {
        return 0;
    }

    return mHandleOwners.value(channelHandle, 0);
}

Tp::ContactPtr ChatAgent::channelContact(const QString &contactId) const
{
    if(!mTextChannel) {
        return Tp::ContactPtr();
    }

    foreach(Tp::ContactPtr contact, mTextChannel->groupContacts().toList()) {
        if(contact->id() == contactId) {
            return contact;
        }
    }
    return Tp::ContactPtr();
}

void ChatAgent::onNewHandleOwner(const uint &handle)
{
    uint globalHandle = mHandleOwners.value(handle, 0);

    if(handle != 0) {
        Tp::ContactManagerPtr contactManager = mTextChannel->connection()->contactManager();
        connect(contactManager->contactsForHandles(Tp::UIntList() << globalHandle),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onNewHandleContact(Tp::PendingOperation*)));
    }
}

void ChatAgent::onNewHandleContact(Tp::PendingOperation *op)
{
    if (!op || op->isError()) {
        qWarning() << "Contacts cannot be obtained from handles";
        return;
    }

    Tp::PendingContacts *pendingContacts = qobject_cast<Tp::PendingContacts *>(op);
    QList<Tp::ContactPtr> contacts = pendingContacts->contacts();

    foreach(Tp::ContactPtr contact, contacts) {
        foreach(uint handle, contact->handle()) {
            mHandleContactMap.insert(handle, contact);
        }
    }
}

Tp::ContactPtr ChatAgent::contactByChannelHandle(const uint &handle) const
{
    return mHandleContactMap.value(globalHandle(handle), Tp::ContactPtr());
}

void ChatAgent::onAccountConnectionChanged(const Tp::ConnectionPtr &conn)
{
    if (conn == NULL) {
        qDebug() << "NULL connection";
        endChat();
    }
    else {
        connect(conn.data(),
            SIGNAL(statusChanged(Tp::ConnectionStatus)),
            SLOT(onConnectionStatusChanged(Tp::ConnectionStatus)));
        connect(conn.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                SLOT(onConnectionInvalidated(Tp::DBusProxy*)));
        onConnectionStatusChanged(conn->status());
    }
}

void ChatAgent::onConnectionStatusChanged(Tp::ConnectionStatus status)
{
    if(status == Tp::ConnectionStatusDisconnected) {
        endChat();
    }
}

void ChatAgent::onConnectionInvalidated(Tp::DBusProxy *proxy)
{
    Tp::Connection *conn = qobject_cast<Tp::Connection *>(proxy);
    if (conn) {
        disconnect(conn, 0, this, 0);
    }
}
