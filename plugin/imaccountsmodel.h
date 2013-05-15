/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMACCOUNTSMODEL_H
#define IMACCOUNTSMODEL_H

#include <TelepathyQt4/Types>
#include <TelepathyQt4Yell/Models/AccountsModel>
#include <TelepathyQt4Yell/Models/ContactModelItem>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyLoggerQt4/Types>
#include <TelepathyQt4Yell/CallChannel>
#include <QMap>
#include "../telepathy-qml-lib/callagent.h"
#include "../telepathy-qml-lib/telepathymanager.h"

class ChatAgent;
class FileTransferAgent;
class ServerAuthAgent;

typedef QHash<QString, ChatAgent*> ChatAgentHash;
typedef QHash<QString, CallAgent*> CallAgentHash;
typedef QHash<QString, FileTransferAgent*> FileTransferAgentHash;
typedef QHash<QString, ServerAuthAgent*> ServerAuthAgentHash;


class NotificationManager;

class IMAccountsModel : public Tpy::AccountsModel
{
    Q_OBJECT
    Q_ENUMS(IMAccountRoles)
public:
    enum IMAccountRoles {
        PendingMessagesRole = Tpy::AccountsModel::CustomRole,
        ChatOpenedRole,
        LastPendingMessageRole,
        LastPendingMessageSentRole,
        MissedVideoCallsRole,
        MissedAudioCallsRole,
        ExistingCallRole,
        IncomingVideoCallRole,
        IncomingAudioCallRole,
        ModelDataRole,
        PresenceTypesListRole,
        AvatarsListRole,
        IsGroupChatRole,
        GroupChatCapableRole,
        CanBlockContactsRole,
        ParentDisplayNameRole,
        ParentIdRole
    };

    explicit IMAccountsModel(const Tp::AccountManagerPtr &am, QObject *parent = 0);
    virtual ~IMAccountsModel();

    virtual QVariant data(const QModelIndex &index, int role) const;

    Q_INVOKABLE void startChat(const QString &accountId, const QString &contactId);
    Q_INVOKABLE QString startPrivateChat(const QString &accountId, const QString &channelPath, const QString &contactId);
    Q_INVOKABLE void startGroupChat(const QString &accountId, const QString &channelPath);
    Q_INVOKABLE void endChat(const QString &accountId, const QString &contactId);
    Q_INVOKABLE QObject *conversationModel(const QString &accountId, const QString &contactId);
    Q_INVOKABLE QObject *groupConversationModel(const QString &accountId, const QString &channelPath);
    Q_INVOKABLE void disconnectConversationModel(const QString &accountId, const QString &contactId);
    Q_INVOKABLE void disconnectGroupConversationModel(const QString &accountId, const QString &channelPath);
    Q_INVOKABLE ChatAgent *chatAgentByKey(const QString &account, const QString &suffix) const;
    Q_INVOKABLE CallAgent *callAgent(const QString &accountId, const QString &contactId);
    Q_INVOKABLE QObject *fileTransferAgent(const QString &accountId, const QString &contactId);
    Q_INVOKABLE void addContactsToChat(const QString &accountId, const QString &channelPath, const QString &contacts);
    Q_INVOKABLE int accountsOfType(const QString &type) const;
    Q_INVOKABLE QStringList accountIdsOfType(const QString &type) const;
    Q_INVOKABLE bool existsConnectsAutomaticallyByType(const QString &type) const;
    Q_INVOKABLE void addContactFromGroupChat(const QString &accountId, const QString &channelPath, const QString &contactId);
    Q_INVOKABLE bool isContactKnown(const QString &accountId, const QString &channelPath, const QString &contactId) const;
    Q_INVOKABLE void blockContact(const QString &accountId, const QString &contactId);
    Q_INVOKABLE void unblockContact(const QString &accountId, const QString &contactId);
    Q_INVOKABLE void removeContact(const QString &accountId, const QString &contactId);
    Q_INVOKABLE bool isAccountRegistered(const QString &cm, const QString &protocol, const QString &displayName) const;

    Q_INVOKABLE QString accountDisplayName(const QString &iconName, const QString &displayName) const;

    Q_INVOKABLE void setAccountPassword(const QString &account, const QString &password);

    Q_INVOKABLE QStringList channelContacts(const QString &accountId, const QString &channelPath) const;

    void setNotificationManager(NotificationManager *notificationManager);
    void setTelepathyManager(TelepathyManager *manager);

    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void clearAccountHistory(const QString &accountId);
    Q_INVOKABLE void clearContactHistory(const QString &accountId, const QString &contactId);
    Q_INVOKABLE void clearGroupChatHistory(const QString &accountId, const QString &channelPath);

Q_SIGNALS:
    void chatReady(const QString &accountId, const QString &contactId);
    void groupChatReady(const QString &accountId, const QString &channelPath);
    void componentsLoaded();
    void incomingFileTransferAvailable(const QString &accountId, const QString &contactId);
    void incomingCallAvailable(const QString &accountId, const QString &contactId);
    void requestedGroupChatCreated(QObject *agent);
    void passwordRequestRequired(const QString &accountId);

public Q_SLOTS:
    void onTextChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel);
    void onCallChannelAvailable(const QString &accountId, Tpy::CallChannelPtr channel);
    void onIncomingFileTransferChannelAvailable(const QString &accountId, Tp::IncomingFileTransferChannelPtr channel);
    void onOutgoingFileTransferChannelAvailable(const QString &accountId,
                                                Tp::OutgoingFileTransferChannelPtr channel,
                                                Tp::ChannelRequestPtr request);
    void onServerAuthChannelAvailable(const QString &accountId, Tp::ChannelPtr channel);
    void onComponentsLoaded();
    void onNetworkStatusChanged(bool isOnline);

protected:
    Tpy::ContactModelItem *itemForChatAgent(ChatAgent *agent) const;
    Tpy::ContactModelItem *itemForCallAgent(CallAgent *agent) const;
    Tpy::ContactModelItem *itemForFileTransferAgent(FileTransferAgent *agent) const;
    Tp::ContactPtr contactFromChannelId(const Tp::AccountPtr &account, const QString &channelPath, const QString &contactId) const;

private Q_SLOTS:
    void onChatCreated();
    void onRequestedGroupChatCreated();
    void onPendingMessagesChanged();
    void onCallAgentChanged();
    void onCallStatusChanged(CallAgent::CallStatus oldCallStatus, CallAgent::CallStatus newCallStatus);
    void onCallError(const QString & errorString);
    void onPendingFileTransfersChanged();
    void onMissedCallsChanged();
    void onBlockedContact(Tp::PendingOperation *op);
    void onUnblockedContact(Tp::PendingOperation *op);
    void onGotAllProperties(QDBusPendingCallWatcher *watcher);
    void onSetPrivacyProperty(QDBusPendingCallWatcher *watcher);
    void onAccountCountChanged();
    void onAccountConnectionStatusChanged(const QString &accountId, const int status);
    void onConnectionReady(Tp::ConnectionPtr connection);

private:
    Tpl::LoggerPtr mLogger;
    ChatAgentHash mChatAgents;
    CallAgentHash mCallAgents;
    FileTransferAgentHash mFileTransferAgents;
    ServerAuthAgentHash mServerAuthAgents;
    NotificationManager *mNotificationManager;
    TelepathyManager *mTelepathyManager;
    QMap<QString, QString> mAccountPasswords;

    ChatAgent *chatAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists = true);
    ChatAgent *chatAgentByPtr(const Tp::AccountPtr &account, const Tp::TextChannelPtr &channel, bool createIfNotExists = true);
    ChatAgent *chatAgentByIndex(const QModelIndex &index) const;
    ChatAgent *chatAgent(const QString &accountId, const QString &contactId, bool createIfNotExists = true);
    ChatAgent *chatAgent(const QString &accountId, const Tp::TextChannelPtr &channel, bool createIfNotExists = true);
    QList<ChatAgent *> chatAgentsByAccount(const QString &accountId) const;
    QList<FileTransferAgent *> fileTransferAgentsByAccount(const QString &accountId) const;
    int pendingMessagesByAccount(const QString &accountId) const;
    int fileTransfersByAccount(const QString &accountId) const;
    bool openedChatByAccount(const QString &accountId) const;

    CallAgent *callAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists = true);
    CallAgent *callAgentByIndex(const QModelIndex &index) const;
    CallAgent *callAgent(const QString &accountId, const QString &contactId, bool createIfNotExists);

    FileTransferAgent *fileTransferAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists = true);
    FileTransferAgent *fileTransferAgentByIndex(const QModelIndex &index) const;
    Q_INVOKABLE FileTransferAgent *fileTransferAgent(const QString &accountId, const QString &contactId, bool createIfNotExists);

    ServerAuthAgent *serverAuthAgentByPtr(const Tp::AccountPtr &account);
};

#endif // IMACCOUNTSMODEL_H
