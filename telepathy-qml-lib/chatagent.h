/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CHATAGENT_H
#define CHATAGENT_H

#include "imconversationmodel.h"

#include "simplecontactslistmodel.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>

namespace Tp {
    class PendingMediaStreams;
}

class ChatAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool existsChat READ existsChat NOTIFY existsChatChanged)
    Q_PROPERTY(int pendingConversations READ pendingConversations NOTIFY pendingConversationsChanged)
    Q_PROPERTY(QString channelPath READ channelPath)
    Q_PROPERTY(bool isConference READ isConference NOTIFY isConferenceChanged)
    Q_PROPERTY(bool canAddContacts READ canAddContacts NOTIFY isConferenceChanged)
    Q_PROPERTY(bool isGroupChatCapable READ isConnectionGroupChatCapable NOTIFY isConferenceChanged)
public:
    explicit ChatAgent(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, QObject *parent);
    explicit ChatAgent(const Tp::AccountPtr &account, const QString &roomName, QObject *parent);
    explicit ChatAgent(const Tp::AccountPtr &account, const Tp::TextChannelPtr &channel, QObject *parent);
    virtual ~ChatAgent();

    Tp::AccountPtr account() const;
    Tp::ContactPtr contact() const;
    IMConversationModel *model() const;
    Tp::TextChannelPtr textChannel() const;

    void startChat();
    void endChat();
    bool existsChat();
    int pendingConversations() const;
    void resetPendingConversations();
    QString channelPath() const;
    bool isConference() const;
    bool canAddContacts() const;
    bool isConnectionGroupChatCapable() const;

    QString lastPendingMessage() const;
    QDateTime lastPendingMessageSent() const;

    Q_INVOKABLE SimpleContactsListModel *contactsModel();
    Q_INVOKABLE void addContacts(const QList<Tp::ContactPtr> contacts);
    Tp::ContactPtr channelContact(const QString &contactId) const;
    Tp::ContactPtr contactByChannelHandle(const uint &handle) const;

    //void setTextChannel(Tp::TextChannelPtr textChannel);
    //Tp::TextChannelPtr textChannel() const;

Q_SIGNALS:
    void chatCreated();
    void chatEnded();
    void error(const QString &msg);
    void requestedChatCreated();
    void requestedGroupChatCreated();
    void isConferenceChanged();

    // property notification signals
    void existsChatChanged();
    void pendingConversationsChanged();


public Q_SLOTS:
    void onTextChannelAvailable(Tp::TextChannelPtr channel);

protected:
    void createModelForChat(void);
    void handleReadyChannel();
    uint globalHandle(const uint &channelHandle) const;

private Q_SLOTS:
    //void onOutgoingChannelEnsured(Tp::PendingOperation *op);
    void onChannelInvalidated(Tp::DBusProxy *, const QString &errorName, const QString &errorMessage);
    //void onOutgoingChannelReady(Tp::PendingOperation *op);
    void onPendingChanelRequestFinished(Tp::PendingOperation *op);
    void onPendingChanelRequestCreated(const Tp::ChannelRequestPtr &channelRequest);
    void onTextChannelReady(Tp::PendingOperation *op);
    void onGroupHandleOwnersChanged(const Tp::HandleOwnerMap &owners, const Tp::UIntList &added, const Tp::UIntList &removed);
    void onNewHandleOwner(const uint &handle);
    void onNewHandleContact(Tp::PendingOperation *op);
    void onConnectionStatusChanged(Tp::ConnectionStatus status);
    void onAccountConnectionChanged(const Tp::ConnectionPtr &conn);
    void onConnectionInvalidated(Tp::DBusProxy *proxy);

private:
    Tp::AccountPtr mAccount;
    Tp::ContactPtr mContact;
    QList<Tp::ChannelPtr> mChannels;
    Tp::HandleOwnerMap mHandleOwners;
    Tp::TextChannelPtr mTextChannel;
    bool mExistsChat;
    IMConversationModel *mModel;
    QMap<uint, Tp::ContactPtr> mHandleContactMap;
    Tp::PendingChannelRequest *mPendingChannelRequest;
};

#endif // ChatAgent_H
