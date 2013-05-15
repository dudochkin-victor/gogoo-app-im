/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imaccountsmodel.h"
#include <TelepathyQt4Yell/Models/ContactModelItem>
#include <TelepathyQt4Yell/Models/AbstractConversationModel>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/ReferencedHandles>
#include "../telepathy-qml-lib/chatagent.h"
#include "../telepathy-qml-lib/callagent.h"
#include "../telepathy-qml-lib/filetransferagent.h"
#include "../telepathy-qml-lib/serverauthagent.h"
#include "../telepathy-qml-lib/notificationmanager.h"

#include <TelepathyLoggerQt4/Logger>

#include <TelepathyQt4/Client>
#include <TelepathyQt4/PropertiesInterface>

#include <QVariantMap>

IMAccountsModel::IMAccountsModel(const Tp::AccountManagerPtr &am,
                                 QObject *parent)
 : Tpy::AccountsModel(am, parent),
   mLogger(Tpl::Logger::create()),
   mNotificationManager(0),
   mTelepathyManager(0)
{
    QHash<int, QByteArray> roles = roleNames();
    roles[PendingMessagesRole] = "pendingMessages";
    roles[ChatOpenedRole] = "chatOpened";
    roles[LastPendingMessageRole] = "lastPendingMessage";
    roles[LastPendingMessageSentRole] = "lastPendingMessageSent";
    roles[MissedVideoCallsRole] = "missedVideoCalls";
    roles[MissedAudioCallsRole] = "missedAudioCalls";
    roles[ExistingCallRole] = "existingCall";
    roles[IncomingVideoCallRole] = "incomingVideoCall";
    roles[IncomingAudioCallRole] = "incomingAudioCall";
    roles[ModelDataRole] = "modelData";
    roles[PresenceTypesListRole] = "presenceTypesList";
    roles[AvatarsListRole] = "avatarsList";
    roles[IsGroupChatRole] = "isGroupChat";
    roles[GroupChatCapableRole] = "groupChatCapable";
    roles[CanBlockContactsRole] = "canBlockContacts";
    roles[ParentDisplayNameRole] = "parentDisplayName";
    roles[ParentIdRole] = "parentId";
    setRoleNames(roles);

    // get privacy settings for all accounts
    connect(this, SIGNAL(accountConnectionStatusChanged(QString,int)),
            SLOT(onAccountConnectionStatusChanged(QString, int)));
}

IMAccountsModel::~IMAccountsModel()
{
    ChatAgentHash::const_iterator it;
    for (it = mChatAgents.constBegin(); it != mChatAgents.constEnd(); ++it) {
        ChatAgent *agent = it.value();
        if (agent) {
            agent->endChat();
            agent->deleteLater();
        }
    }
    mChatAgents.clear();

    CallAgentHash::const_iterator it2;
    for (it2 = mCallAgents.constBegin(); it2 != mCallAgents.constEnd(); ++it2) {
        CallAgent *agent = it2.value();
        if (agent) {
            agent->endCall();
            agent->deleteLater();
        }
    }
    mCallAgents.clear();
}

QVariant IMAccountsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case PendingMessagesRole: {
            // a valid parent means it is a contact index
            if (index.parent().isValid()) {
                int count = 0;
                ChatAgent *chatAgent = chatAgentByIndex(index);
                if (chatAgent) {
                    count += chatAgent->pendingConversations();
                }

                FileTransferAgent *fileTransferAgent = fileTransferAgentByIndex(index);
                if (fileTransferAgent) {
                    count += fileTransferAgent->pendingTransfers();
                }
                return count;
            }

            // in case the parent is not valid, we need to count the children's pending conversations
            int messages = pendingMessagesByAccount(Tpy::AccountsModel::data(index, IdRole).toString());
            messages += fileTransfersByAccount(Tpy::AccountsModel::data(index, IdRole).toString());
            return messages;
        }

        case ChatOpenedRole: {
            // contact index
            if (index.parent().isValid()) {
                ChatAgent *chatAgent = chatAgentByIndex(index);
                if (chatAgent) {
                    return chatAgent->existsChat();
                }

                FileTransferAgent *fileTransferAgent = fileTransferAgentByIndex(index);
                if (fileTransferAgent) {
                    return fileTransferAgent->pendingTransfers() > 0;
                }
                return false;
            }

            // account index
            return (openedChatByAccount(Tpy::AccountsModel::data(index, IdRole).toString())
                    || fileTransfersByAccount(Tpy::AccountsModel::data(index, IdRole).toString()));
        }

        case LastPendingMessageRole: {
            // contact index
            if (index.parent().isValid()) {
                ChatAgent *chatAgent = chatAgentByIndex(index);
                if (chatAgent) {
                    return chatAgent->lastPendingMessage();
                }
            }
            return QVariant();
        }

        case LastPendingMessageSentRole: {
            // contact index
            if (index.parent().isValid()) {
                ChatAgent *chatAgent = chatAgentByIndex(index);
                if (chatAgent) {
                    return chatAgent->lastPendingMessageSent();
                }
            }
            return QVariant();
        }

        case MissedVideoCallsRole: {
            // contact index
            if (index.parent().isValid()) {
                CallAgent *callAgent = callAgentByIndex(index);
                if (callAgent) {
                    return callAgent->missedVideoCalls();
                }
                return 0;
            }

            // account index
            uint numCalls = 0;
            int count = Tpy::AccountsModel::rowCount(index);
            for (int i = 0; i < count; ++i) {
                QModelIndex child = Tpy::AccountsModel::index(i, 0, index);
                numCalls += data(child, MissedVideoCallsRole).toUInt();
            }
            return numCalls;
        }

        case MissedAudioCallsRole: {
            // contact index
            if (index.parent().isValid()) {
                CallAgent *callAgent = callAgentByIndex(index);
                if (callAgent) {
                    return callAgent->missedAudioCalls();
                }
                return 0;
            }

            // account index
            uint numCalls = 0;
            int count = Tpy::AccountsModel::rowCount(index);
            for (int i = 0; i < count; ++i) {
                QModelIndex child = Tpy::AccountsModel::index(i, 0, index);
                numCalls += data(child, MissedAudioCallsRole).toUInt();
            }
            return numCalls;
        }

        case ExistingCallRole: {
            // contact index
            if (index.parent().isValid()) {
                CallAgent *callAgent = callAgentByIndex(index);
                if (callAgent) {
                    return callAgent->existingCall();
                }
                return false;
            }

            // account index
            bool existingCall = false;
            int count = Tpy::AccountsModel::rowCount(index);
            for (int i = 0; i < count; ++i) {
                QModelIndex child = Tpy::AccountsModel::index(i, 0, index);
                existingCall = existingCall || data(child, ExistingCallRole).toBool();
            }
            return existingCall;
        }

        case IncomingVideoCallRole: {
            // contact index
            if (index.parent().isValid()) {
                CallAgent *callAgent = callAgentByIndex(index);
                if (callAgent) {
                    return callAgent->incomingVideoCall();
                }
                return false;
            }

            // account index
            bool incomingVideoCall = false;
            int count = Tpy::AccountsModel::rowCount(index);
            for (int i = 0; i < count; ++i) {
                QModelIndex child = Tpy::AccountsModel::index(i, 0, index);
                incomingVideoCall = incomingVideoCall || data(child, IncomingVideoCallRole).toBool();
            }
            return incomingVideoCall;
        }

        case IncomingAudioCallRole: {
            // contact index
            if (index.parent().isValid()) {
                CallAgent *callAgent = callAgentByIndex(index);
                if (callAgent) {
                    return callAgent->incomingAudioCall();
                }
                return false;
            }

            // account index
            bool incomingAudioCall = false;
            int count = Tpy::AccountsModel::rowCount(index);
            for (int i = 0; i < count; ++i) {
                QModelIndex child = Tpy::AccountsModel::index(i, 0, index);
                incomingAudioCall = incomingAudioCall || data(child, IncomingAudioCallRole).toBool();
            }
            return incomingAudioCall;
        }

        // this is just to make the display role available in QML
        case ModelDataRole: {
            return data(index, AccountsModel::DisplayNameRole);
        }

        // this will display the service name
        // it will display the account name only in case there is more than one account for the service

        case AccountsModel::DisplayNameRole: {
            return accountDisplayName(Tpy::AccountsModel::data(index, AccountsModel::IconRole).toString(),
                                      Tpy::AccountsModel::data(index, AccountsModel::DisplayNameRole).toString());
        }

        // always return false as this is only for group chat items
        case IsGroupChatRole:
            return false;

        case GroupChatCapableRole: {
            // contact index
            if (index.parent().isValid()) {
                Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(
                            contactItemForId(Tpy::AccountsModel::data(index.parent(), IdRole).toString(),
                                             Tpy::AccountsModel::data(index, IdRole).toString()));
                if(contactItem) {
                    Tp::ContactPtr contact = contactItem->contact();
                    if(!contact->manager()->connection().isNull()) {
                        return contact->manager()->connection()->capabilities().conferenceTextChats();
                    }
                }
                return false;
            }

            // account index
            Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                        accountItemForId(Tpy::AccountsModel::data(index, IdRole).toString()));
            if(accountItem) {
                Tp::AccountPtr account = accountItem->account();
                if(!account->connection().isNull()) {
                    return account->connection()->capabilities().conferenceTextChats();
                }
            }
            return false;
        }

        case CanBlockContactsRole: {
            Tpy::AccountsModelItem *accountItem;

            if (index.parent().isValid()) {
                // contact index
                accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                            accountItemForId(Tpy::AccountsModel::data(index.parent(), IdRole).toString()));
            } else {
                // account index
                accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                            accountItemForId(Tpy::AccountsModel::data(index, IdRole).toString()));
            }

            if(accountItem) {
                Tp::AccountPtr account = accountItem->account();
                if(!account->connection().isNull()) {
                    return account->connection()->contactManager()->canBlockContacts();
                }
            }
            return false;
        }

        case ParentDisplayNameRole: {
            if (index.parent().isValid()) {
                return data(index.parent(), AccountsModel::DisplayNameRole).toString();
            }

            return QString();
        }

        case ParentIdRole: {
            if (index.parent().isValid()) {
                return Tpy::AccountsModel::data(index.parent(), IdRole).toString();
            }

            return QString();
        }

        default:
            return Tpy::AccountsModel::data(index, role);
    }
}

void IMAccountsModel::startChat(const QString &accountId, const QString &contactId)
{
    qDebug() << "Starting chat with: " << accountId << contactId;

    ChatAgent *agent = chatAgent(accountId, contactId, true);
    if (!agent) {
        return;
    }

    if (agent->existsChat()) {
        qDebug("chat exists. emitting signal");
        emit chatReady(accountId, contactId);
    } else {
        qDebug("chat does not exist. creating channel");
        agent->startChat();
    }
}

void IMAccountsModel::startGroupChat(const QString &accountId, const QString &channelPath)
{
    qDebug() << "Starting group chat with: " << accountId << channelPath;

    ChatAgent *agent = chatAgentByKey(accountId, channelPath);
    if (!agent) {

        return;
    }

    // this should always be true, as the group chats are always becomeReady as soon as they become available
    if (agent->existsChat()) {
        qDebug("groupChatReady signal emitted");
        emit groupChatReady(accountId, agent->channelPath());
    }
}

void IMAccountsModel::addContactsToChat(const QString &accountId, const QString &channelPath, const QString &contacts)
{
    // parse the contacts and get the actual objects
    QStringList contactsStringList = contacts.split(QRegExp("\\s+"));

    QList<Tp::ContactPtr> contactsList;
    foreach(QString contactId, contactsStringList) {
        Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
        if(contactItem) {
            contactsList.append(contactItem->contact());
        }
    }

    ChatAgent *chatAgent = chatAgentByKey(accountId, channelPath);

    if(chatAgent && chatAgent->canAddContacts()) {
        // add contact to existing channel
        chatAgent->addContacts(contactsList);
    } else {
        // create a new group chat
        // add contacts from the existing channel, if possible
        if (chatAgent) {
            foreach (Tp::ContactPtr contact, chatAgent->textChannel()->groupContacts().toList()) {
                if (chatAgent->textChannel()->connection()->selfContact() != contact) {
                    contactsList.append(contact);
                }
            }
        }

        // retrieve account object
        Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
        if(!accountItem) {
            return;
        }
        Tp::AccountPtr account = accountItem->account();
        account->createConferenceTextChat(QList<Tp::ChannelPtr>(),
                                          contactsList,
                                          QDateTime::currentDateTime(),
                                          QLatin1String("org.freedesktop.Telepathy.Client.MeeGoIM"));
    }
}

void IMAccountsModel::endChat(const QString &accountId, const QString &contactId)
{
    const QString key = accountId + "&" + contactId;
    if (mChatAgents.contains(key)) {
        mChatAgents[key]->endChat();
    }

    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if (contactItem && contactItem->parent()) {
        onItemChanged(contactItem);
        onItemChanged(contactItem->parent());
    }
}

int IMAccountsModel::accountsOfType(const QString &type) const
{
    int count = 0;
    for (int i = 0; i < rowCount(); ++i) {
        if (index(i, 0).data(Tpy::AccountsModel::IconRole).toString() == type) {
            count++;
        }
    }

    return count;
}

QStringList IMAccountsModel::accountIdsOfType(const QString &type) const
{
    QStringList ids;
    for (int i = 0; i < rowCount(); ++i) {
        if (index(i, 0).data(Tpy::AccountsModel::IconRole).toString() == type) {
            ids.append(index(i,0).data(Tpy::AccountsModel::IdRole).toString());
        }
    }

    return ids;
}

bool IMAccountsModel::existsConnectsAutomaticallyByType(const QString &type) const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (index(i, 0).data(Tpy::AccountsModel::IconRole).toString() == type
                && index(i,0).data(Tpy::AccountsModel::ConnectsAutomaticallyRole).toBool()) {
            return true;
        }
    }
    return false;
}

void IMAccountsModel::onChatCreated()
{
    ChatAgent *agent = qobject_cast<ChatAgent*>(sender());
    if (!agent) {
        return;
    }

    Tpy::ContactModelItem *contactItem = itemForChatAgent(agent);
    if (contactItem && contactItem->parent()) {
        QString accountId = contactItem->parent()->data(IdRole).toString();
        QString contactId = contactItem->data(IdRole).toString();
        emit chatReady(accountId, contactId);

        onItemChanged(contactItem);
        onItemChanged(contactItem->parent());
    }
}

void IMAccountsModel::onRequestedGroupChatCreated()
{
    ChatAgent *agent = qobject_cast<ChatAgent*>(sender());
    if (!agent) {
        return;
    }

    // this signals the chat agent is ready for use
    emit requestedGroupChatCreated(agent);
}

void IMAccountsModel::onPendingMessagesChanged()
{
    ChatAgent *agent = qobject_cast<ChatAgent*>(sender());

    Tpy::ContactModelItem *contactItem = itemForChatAgent(agent);
    if (contactItem && contactItem->parent()) {
        onItemChanged(contactItem);
        onItemChanged(contactItem->parent());

        if (!agent || !agent->pendingConversations()) {
            return;
        }

        QModelIndex idx = index(contactItem);
        // if there is a pending message, place a notification
        if (mNotificationManager) {
            mNotificationManager->notifyPendingMessage(contactItem->parent()->data(IdRole).toString(),
                                                       contactItem->data(IdRole).toString(),
                                                       contactItem->data(AliasRole).toString(),
                                                       data(idx, LastPendingMessageSentRole).toDateTime(),
                                                       data(idx, LastPendingMessageRole).toString());
        }
    }
}

void IMAccountsModel::onPendingFileTransfersChanged()
{
    FileTransferAgent *agent = qobject_cast<FileTransferAgent*>(sender());

    Tpy::ContactModelItem *contactItem = itemForFileTransferAgent(agent);
    if (contactItem && contactItem->parent()) {
        onItemChanged(contactItem);
        onItemChanged(contactItem->parent());

        if (!agent || !agent->pendingTransfers()) {
            return;
        }

        QModelIndex idx = index(contactItem);
        if (mNotificationManager) {
            mNotificationManager->notifyIncomingFileTransfer(contactItem->parent()->data(IdRole).toString(),
                                                             contactItem->data(IdRole).toString(),
                                                             contactItem->data(AliasRole).toString(),
                                                             agent->lastPendingTransferDateTime(),
                                                             agent->lastPendingTransfer());
        }
    }
}

void IMAccountsModel::onMissedCallsChanged()
{
    CallAgent *agent = qobject_cast<CallAgent*>(sender());

    Tpy::ContactModelItem *contactItem = itemForCallAgent(agent);
    if (contactItem && contactItem->parent()) {
        // the changed signals don't need to be emitted here, they are already
        // emitted in onCallAgentChanged()
        if (!agent || !agent->missedAudioCalls() || !agent->missedVideoCalls()) {
            return;
        }

        QDateTime lastAudioCall, lastVideoCall;

        if (agent->missedAudioCalls()) {
            lastAudioCall = agent->lastMissedAudioCall();
        }

        if (agent->missedVideoCalls()) {
            lastVideoCall = agent->lastMissedVideoCall();
        }

        QModelIndex idx = index(contactItem);
        if (mNotificationManager) {
            if (lastAudioCall > lastVideoCall) {
                mNotificationManager->notifyMissedCall(contactItem->parent()->data(IdRole).toString(),
                                                       contactItem->data(IdRole).toString(),
                                                       contactItem->data(AliasRole).toString(),
                                                       lastAudioCall);
            } else {
                mNotificationManager->notifyMissedVideoCall(contactItem->parent()->data(IdRole).toString(),
                                                            contactItem->data(IdRole).toString(),
                                                            contactItem->data(AliasRole).toString(),
                                                            lastVideoCall);
            }
        }
    }
}

QObject *IMAccountsModel::conversationModel(const QString &accountId, const QString &contactId)
{
    ChatAgent *agent = chatAgent(accountId, contactId, false);
    if (agent && agent->model()) {
        // connect the model to the channel message queue and return the model
        // disconnect first to make sure there's no duplicate connection
        agent->model()->disconnectChannelQueue();
        agent->model()->connectChannelQueue();
        return agent->model();
    }
    return 0;
}

QObject *IMAccountsModel::groupConversationModel(const QString &accountId, const QString &channelPath)
{
    ChatAgent *agent = chatAgentByKey(accountId, channelPath);
    if (agent && agent->model()) {
        //connect the model to the channel message queue and return the model
        agent->model()->disconnectChannelQueue();
        agent->model()->connectChannelQueue();
        return agent->model();
    }
    return 0;
}

/**
  * This finds the corresponding chatAgent and then disconnect the conversationModel from the channel message queue
  * This could be done in QML but here it is much easier to check the validity of the objects
  */
void IMAccountsModel::disconnectConversationModel(const QString &accountId, const QString &contactId)
{
    ChatAgent *agent = chatAgent(accountId, contactId, false);
    if (agent && agent->model()) {
        agent->model()->disconnectChannelQueue();
    }
}

void IMAccountsModel::disconnectGroupConversationModel(const QString &accountId, const QString &channelPath)
{
    ChatAgent *agent = chatAgentByKey(accountId, channelPath);
    if (agent && agent->model()) {
        agent->model()->disconnectChannelQueue();
    }
}

Tpy::ContactModelItem *IMAccountsModel::itemForChatAgent(ChatAgent *agent) const
{
    if (agent && !agent->contact().isNull() && !agent->account().isNull()) {
        QString contactId = agent->contact()->id();
        QString accountId = agent->account()->uniqueIdentifier();
        Tpy::ContactModelItem *contactItem;
        contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
        return contactItem;
    }

    return 0;
}

ChatAgent *IMAccountsModel::chatAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists)
{
    if (account.isNull() || contact.isNull()) {
        return 0;
    }

    ChatAgent *agent = 0;
    const QString key = account->uniqueIdentifier() + "&" + contact->id();
    if (!mChatAgents.contains(key)) {
        if (createIfNotExists) {
            agent = new ChatAgent(account, contact, this);
            mChatAgents[key] = agent;
            connect(agent, SIGNAL(chatCreated()), SLOT(onChatCreated()));
            connect(agent, SIGNAL(pendingConversationsChanged()), SLOT(onPendingMessagesChanged()));
        }
    } else {
      agent = mChatAgents[key];
    }

    return agent;
}

ChatAgent *IMAccountsModel::chatAgentByPtr(const Tp::AccountPtr &account, const Tp::TextChannelPtr &channel, bool createIfNotExists)
{
    if (account.isNull() || channel.isNull()) {
        return 0;
    }

    ChatAgent *agent = 0;
    const QString key = account->uniqueIdentifier() + "&" + channel->objectPath();
    if (!mChatAgents.contains(key)) {
        if (createIfNotExists) {
            qDebug("group chat agent created");
            agent = new ChatAgent(account, channel, this);
            mChatAgents[key] = agent;
       }
    } else {
      agent = mChatAgents[key];
    }

    return agent;
}

ChatAgent* IMAccountsModel::chatAgentByIndex(const QModelIndex &index) const
{
    // if it is a contact
    if (index.isValid() && index.parent().isValid()) {
        return chatAgentByKey(Tpy::AccountsModel::data(index.parent(), IdRole).toString(),
                              Tpy::AccountsModel::data(index, IdRole).toString());
    }
    return 0;
}

ChatAgent *IMAccountsModel::chatAgentByKey(const QString &account, const QString &suffix) const
{
    if(!account.isEmpty() && !suffix.isEmpty()) {
        QString key = account + "&" + suffix;
        if (mChatAgents.contains(key)) {
            return mChatAgents[key];
        }
    }
    return 0;
}

Tpy::ContactModelItem *IMAccountsModel::itemForCallAgent(CallAgent *agent) const
{
    Tp::AccountPtr account = agent->account();
    if (account.isNull()) {
        return 0;
    }

    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem*>(
                accountItemForId(account->uniqueIdentifier()));
    if (!item || item->account().isNull()) {
        qDebug() << "IMAccountsModel::itemForCallAgent: account not found for " << account->uniqueIdentifier();
        return 0;
    }

    Tp::ContactPtr contact = agent->contact();
    for (int i = 0; i < item->size(); ++ i) {
        Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(item->childAt(i));
        if (contactItem && contactItem->contact() == contact) {
            return contactItem;
        }
    }

    return 0;
}

Tpy::ContactModelItem *IMAccountsModel::itemForFileTransferAgent(FileTransferAgent *agent) const
{
    Tp::AccountPtr account = agent->account();
    if (account.isNull()) {
        return 0;
    }

    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem*>(
                accountItemForId(account->uniqueIdentifier()));
    if (!item || item->account().isNull()) {
        qDebug() << "IMAccountsModel::itemForCallAgent: account not found for " << account->uniqueIdentifier();
        return 0;
    }

    Tp::ContactPtr contact = agent->contact();
    for (int i = 0; i < item->size(); ++ i) {
        Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(item->childAt(i));
        if (contactItem && contactItem->contact() == contact) {
            return contactItem;
        }
    }

    return 0;
}

void IMAccountsModel::onTextChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel)
{
    if (channel.isNull()) {
        return;
    }

    if (channel->connection().isNull()) {
        return;
    }

    if (channel->connection()->contactManager().isNull()) {
        return;
    }

    if(channel->targetHandleType() == Tp::HandleTypeContact) {
        Tp::ContactPtr contact = channel->connection()->contactManager()->lookupContactByHandle( channel->targetHandle() );

        if (contact.isNull()) {
            return;
        }

        // if contact is not known, add it
        Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem *>(contactItemForId(accountId, contact->id()));
        if(!contactItem) {
            Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem *>(accountItemForId(accountId));
            Tpy::ContactModelItem *newContactItem = new Tpy::ContactModelItem(contact);
            accountItem->addChild(newContactItem);
        }

        ChatAgent *agent = chatAgent(accountId, contact->id());
        if (!agent) {
            qWarning() << "IMAccountsModel::onTextChannelAvailable: agent could not be created for text available"
                       << "accountId=" << accountId
                       << "contactId=" << contact->id();
            return;
        }

        agent->onTextChannelAvailable(channel);
    } else if(channel->targetHandleType() == Tp::HandleTypeGroup
              || channel->targetHandleType() == Tp::HandleTypeRoom) {
        ChatAgent *agent = chatAgent(accountId, channel);
        if (!agent) {
            qWarning() << "IMAccountsModel::onTextChannelAvailable: agent could not be created for text available"
                       << "accountId=" << accountId
                       << "channel=" << channel->objectPath();
            return;
        }
        agent->onTextChannelAvailable(channel);
        connect(agent, SIGNAL(requestedGroupChatCreated()), SLOT(onRequestedGroupChatCreated()));
    }
}

ChatAgent *IMAccountsModel::chatAgent(const QString &accountId, const QString &contactId, bool createIfNotExists)
{
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if (!contactItem || contactItem->contact().isNull()) {
        qDebug() << "IMAccountsModel::chatAgent: contact not found";
        return 0;
    }

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(contactItem->parent());
    if (!accountItem || accountItem->account().isNull()) {
        qDebug() << "IMAccountsModel::chatAgent: parent of contact item not found";
        return 0;
    }

    Q_ASSERT(accountItem->data(IdRole).toString() == accountId);
    Tp::ContactPtr contact = contactItem->contact();
    Tp::AccountPtr account = accountItem->account();

    return chatAgentByPtr(account, contact, createIfNotExists);
}

ChatAgent *IMAccountsModel::chatAgent(const QString &accountId, const Tp::TextChannelPtr &channel, bool createIfNotExists)
{
    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (!accountItem || accountItem->account().isNull()) {
        qDebug() << "IMAccountsModel::chatAgent: account item not found";
        return 0;
    }

    Q_ASSERT(accountItem->data(IdRole).toString() == accountId);
    Tp::AccountPtr account = accountItem->account();

    return chatAgentByPtr(account, channel, createIfNotExists);
}

CallAgent* IMAccountsModel::callAgent(const QString &accountId, const QString &contactId, bool createIfNotExists)
{
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if (!contactItem || contactItem->contact().isNull()) {
        qDebug() << "IMAccountsModel::callAgent: contact not found";
        return 0;
    }

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(contactItem->parent());
    if (!accountItem || accountItem->account().isNull()) {
        qDebug() << "IMAccountsModel::callAgent: parent of contact item not found";
        return 0;
    }

    Q_ASSERT(accountItem->data(IdRole).toString() == accountId);
    Tp::ContactPtr contact = contactItem->contact();
    Tp::AccountPtr account = accountItem->account();
    return callAgentByPtr(account, contact, createIfNotExists);
}

CallAgent *IMAccountsModel::callAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists)
{
    if (account.isNull() || contact.isNull()) {
        return 0;
    }

    CallAgent *agent = 0;
    const QString key = account->uniqueIdentifier() + "&" + contact->id();
    if (!mCallAgents.contains(key)) {
        if (createIfNotExists) {
            agent = new CallAgent(account, contact, this);
            mCallAgents[key] = agent;
            connect(agent, SIGNAL(missedVideoCallsChanged()), SLOT(onCallAgentChanged()));
            connect(agent, SIGNAL(missedAudioCallsChanged()), SLOT(onCallAgentChanged()));
            connect(agent, SIGNAL(missedVideoCallsChanged()), SLOT(onMissedCallsChanged()));
            connect(agent, SIGNAL(missedAudioCallsChanged()), SLOT(onMissedCallsChanged()));
            connect(agent, SIGNAL(existingCallChanged()), SLOT(onCallAgentChanged()));
            connect(agent, SIGNAL(incomingVideoCallChanged()), SLOT(onCallAgentChanged()));
            connect(agent, SIGNAL(incomingAudioCallChanged()), SLOT(onCallAgentChanged()));
            connect(agent, SIGNAL(callStatusChanged(CallAgent::CallStatus,CallAgent::CallStatus)), SLOT(onCallStatusChanged(CallAgent::CallStatus,CallAgent::CallStatus)));
            connect(agent, SIGNAL(error(QString)), SLOT(onCallError(QString)));
        }
    } else {
      agent = mCallAgents[key];
    }

    return agent;
}

CallAgent *IMAccountsModel::callAgentByIndex(const QModelIndex &index) const
{
    // if it is a contact
    if (index.isValid() && index.parent().isValid()) {
        QString key = Tpy::AccountsModel::data(index.parent(), IdRole).toString() + "&";
        key += Tpy::AccountsModel::data(index, IdRole).toString();
        if (mChatAgents.contains(key)) {
            return mCallAgents[key];
        }
    }
    return 0;
}

FileTransferAgent* IMAccountsModel::fileTransferAgent(const QString &accountId, const QString &contactId, bool createIfNotExists)
{
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if (!contactItem || contactItem->contact().isNull()) {
        qDebug() << "IMAccountsModel::callAgent: contact not found";
        return 0;
    }

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(contactItem->parent());
    if (!accountItem || accountItem->account().isNull()) {
        qDebug() << "IMAccountsModel::callAgent: parent of contact item not found";
        return 0;
    }

    Q_ASSERT(accountItem->data(IdRole).toString() == accountId);
    Tp::ContactPtr contact = contactItem->contact();
    Tp::AccountPtr account = accountItem->account();
    return fileTransferAgentByPtr(account, contact, createIfNotExists);
}

QObject* IMAccountsModel::fileTransferAgent(const QString &accountId, const QString &contactId)
{
    return fileTransferAgent(accountId, contactId, true);
}

FileTransferAgent* IMAccountsModel::fileTransferAgentByPtr(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, bool createIfNotExists)
{
    if (account.isNull() || contact.isNull()) {
        return 0;
    }

    FileTransferAgent *agent = 0;
    const QString key = account->uniqueIdentifier() + "&" + contact->id();
    if (!mFileTransferAgents.contains(key)) {
        if (createIfNotExists) {
            agent = new FileTransferAgent(account, contact, this);
            mFileTransferAgents[key] = agent;
            connect(agent,
                    SIGNAL(pendingTransfersChanged()),
                    SLOT(onPendingFileTransfersChanged()));
        }
    } else {
      agent = mFileTransferAgents[key];
    }

    return agent;
}

FileTransferAgent* IMAccountsModel::fileTransferAgentByIndex(const QModelIndex &index) const
{
    // if it is a contact
    if (index.isValid() && index.parent().isValid()) {
        QString key = Tpy::AccountsModel::data(index.parent(), IdRole).toString() + "&";
        key += Tpy::AccountsModel::data(index, IdRole).toString();
        if (mFileTransferAgents.contains(key)) {
            return mFileTransferAgents[key];
        }
    }
    return 0;
}

ServerAuthAgent* IMAccountsModel::serverAuthAgentByPtr(const Tp::AccountPtr &account)
{
    ServerAuthAgent *agent = 0;
    if (!mServerAuthAgents.contains(account->uniqueIdentifier())) {
        agent = new ServerAuthAgent(account, this);
        mServerAuthAgents[account->uniqueIdentifier()] = agent;
        connect(agent, SIGNAL(passwordRequestRequired(const QString&)),
                SIGNAL(passwordRequestRequired(const QString&)));
    } else {
        agent = mServerAuthAgents[account->uniqueIdentifier()];
    }

    return agent;
}

void IMAccountsModel::onCallChannelAvailable(const QString &accountId, Tpy::CallChannelPtr channel)
{
    if (channel.isNull()) {
        return;
    }

    if (channel->connection().isNull()) {
        return;
    }

    if (channel->connection()->contactManager().isNull()) {
        return;
    }

    Tp::ContactPtr contact = channel->connection()->contactManager()->lookupContactByHandle( channel->targetHandle() );
    if (contact.isNull()) {
        return;
    }

    // if contact is not known, add it
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem *>(contactItemForId(accountId, contact->id()));
    if(!contactItem) {
        Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem *>(accountItemForId(accountId));
        Tpy::ContactModelItem *newContactItem = new Tpy::ContactModelItem(contact);
        accountItem->addChild(newContactItem);
    }

    CallAgent *agent = callAgent(accountId, contact->id());
    if (!agent) {
        qWarning() << "IMAccountsModel::onCallChannelAvailable: agent could not be created for call channel available"
                   << "accountId=" << accountId
                   << "contactId=" << contact->id();
        return;
    }

    qDebug() << "IMAccountsModel::onCallChannelAvailable: forwarding to agent channel=" << channel.data();
    agent->onChannelAvailable(channel);
    if (!channel->isRequested()) {
        emit incomingCallAvailable(accountId, contact->id());
    }
}

void IMAccountsModel::onIncomingFileTransferChannelAvailable(const QString &accountId, Tp::IncomingFileTransferChannelPtr channel)
{
    if (channel.isNull()) {
        return;
    }

    if (channel->connection().isNull()) {
        return;
    }

    if (channel->connection()->contactManager().isNull()) {
        return;
    }

    Tp::ContactPtr contact = channel->connection()->contactManager()->lookupContactByHandle( channel->targetHandle() );
    if (contact.isNull()) {
        return;
    }

    FileTransferAgent *agent = fileTransferAgent(accountId, contact->id(), true);
    if (!agent) {
        qWarning() << "IMAccountsModel::onIncomingFileTransferChannelAvailable: agent could not be created for file transfer channel available"
                   << "accountId=" << accountId
                   << "contactId=" << contact->id();
        return;
    }

    agent->onIncomingTransferAvailable(channel);

    emit incomingFileTransferAvailable(accountId, contact->id());
}

void IMAccountsModel::onOutgoingFileTransferChannelAvailable(const QString &accountId,
                                                             Tp::OutgoingFileTransferChannelPtr channel,
                                                             Tp::ChannelRequestPtr request)
{
    if (channel.isNull()) {
        return;
    }

    if (channel->connection().isNull()) {
        return;
    }

    if (channel->connection()->contactManager().isNull()) {
        return;
    }

    Tp::ContactPtr contact = channel->connection()->contactManager()->lookupContactByHandle( channel->targetHandle() );
    if (contact.isNull()) {
        return;
    }

    FileTransferAgent *agent = fileTransferAgent(accountId, contact->id(), true);
    if (!agent) {
        qWarning() << "IMAccountsModel::onOutgoingFileTransferChannelAvailable: agent could not be created for file transfer channel available"
                   << "accountId=" << accountId
                   << "contactId=" << contact->id();
        return;
    }

    // give the channel back to the agent
    agent->onOutgoingTransferAvailable(channel, request);
}

void IMAccountsModel::onServerAuthChannelAvailable(const QString &accountId, Tp::ChannelPtr channel)
{
    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (!item) {
        return;
    }

    ServerAuthAgent *agent = serverAuthAgentByPtr(item->account());
    agent->setChannel(channel);
    agent->setPassword(mAccountPasswords[accountId]);
    agent->startAuth();
}

void IMAccountsModel::onCallAgentChanged()
{
    CallAgent *agent = qobject_cast<CallAgent*>(sender());

    Tpy::ContactModelItem *contactItem = itemForCallAgent(agent);
    if (contactItem) {
        onItemChanged(contactItem);
        onItemChanged(contactItem->parent());
    }
}

CallAgent *IMAccountsModel::callAgent(const QString &accountId, const QString &contactId)
{
    return callAgent(accountId, contactId, true);
}

void IMAccountsModel::setNotificationManager(NotificationManager *notificationManager)
{
    mNotificationManager = notificationManager;
}

void IMAccountsModel::setTelepathyManager(TelepathyManager *manager)
{
    mTelepathyManager = manager;
    connect(mTelepathyManager, SIGNAL(connectionAvailable(Tp::ConnectionPtr)),
            SLOT(onConnectionReady(Tp::ConnectionPtr)));

    // trigger an update on the items
    if (rowCount()) {
        QModelIndex topLeft = index(0, 0);
        QModelIndex bottomRight = index(rowCount()-1,0);

        emit dataChanged(topLeft, bottomRight);
    }
}

void IMAccountsModel::onCallStatusChanged(CallAgent::CallStatus oldCallStatus, CallAgent::CallStatus newCallStatus)
{
    CallAgent *callAgent = qobject_cast<CallAgent*>(sender());
    if (!callAgent) {
        return;
    }

    ChatAgent *chatAgent = chatAgentByPtr(callAgent->account(), callAgent->contact());
    if (!chatAgent) {
        return;
    }

    IMConversationModel *model = chatAgent->model();
    if (!model) {
        return;
    }

    model->notifyCallStatusChanged(callAgent, oldCallStatus, newCallStatus);
}

void IMAccountsModel::onCallError(const QString & errorString)
{
    CallAgent *callAgent = qobject_cast<CallAgent*>(sender());
    if (!callAgent) {
        return;
    }

    ChatAgent *chatAgent = chatAgentByPtr(callAgent->account(), callAgent->contact());
    if (!chatAgent) {
        return;
    }

    IMConversationModel *model = chatAgent->model();
    if (!model) {
        return;
    }

    model->notifyCallError(callAgent->contact(), errorString);
}

void IMAccountsModel::onComponentsLoaded()
{
    emit componentsLoaded();
}

QString IMAccountsModel::startPrivateChat(const QString &accountId, const QString &channelPath, const QString &contactId)
{
    qDebug() << "Starting chat with special id: " << accountId << channelPath << contactId;

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if(!accountItem) {
        return QString();
    }

    Tp::AccountPtr account = accountItem->account();

    Tp::ContactPtr contact = contactFromChannelId(account, channelPath, contactId);
    if(contact.isNull()) {
        return QString();
    }

    chatAgentByPtr(account, contact, true);
    return contact->id();
}

Tp::ContactPtr IMAccountsModel::contactFromChannelId(const Tp::AccountPtr &account, const QString &channelPath, const QString &contactId) const
{
    // get connection id
    ChatAgent *agent = chatAgentByKey(account->uniqueIdentifier(), channelPath);
    if (!agent) {
        return Tp::ContactPtr();
    }

    // if not a conference channel, the contact can be used as is
    if(!agent->textChannel()->isConference()) {
        foreach(Tp::ContactPtr contact, agent->textChannel()->groupContacts().toList()) {
            if(contact->id() == contactId) {
                return contact;
            }
        }
        return Tp::ContactPtr();
    }

    if(account->connection().isNull()) {
        return Tp::ContactPtr();
    }

    Tp::ContactManagerPtr contactManager = account->connection()->contactManager();

    if(contactManager.isNull()) {
        return Tp::ContactPtr();
    }

    // get the handle of the contact and get the global handle
    Tp::ContactPtr contact = agent->channelContact(contactId);
    Tp::ReferencedHandles channelHandleList = contact->handle();
    uint channelHandle = channelHandleList.first();
    return agent->contactByChannelHandle(channelHandle);
}

void IMAccountsModel::addContactFromGroupChat(const QString &accountId, const QString &channelPath, const QString &contactId)
{
    qDebug() << "adding contact with special id: " << accountId << channelPath << contactId;

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if(!accountItem) {
        return;
    }

    Tp::AccountPtr account = accountItem->account();

    Tp::ContactPtr contact = contactFromChannelId(account, channelPath, contactId);
    if(contact.isNull()) {
        return;
    }

    // only request subscription if not already requested
    if(contact->subscriptionState() == Tp::Contact::PresenceStateNo) {
        account->connection()->contactManager()->requestPresenceSubscription(QList<Tp::ContactPtr>() << contact);
    }
}

bool IMAccountsModel::isContactKnown(const QString &accountId, const QString &channelPath, const QString &contactId) const
{
    qDebug() << "checking subscription status of contact with special id: " << accountId << channelPath << contactId;

    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if(!accountItem) {
        return false;
    }

    Tp::AccountPtr account = accountItem->account();

    Tp::ContactPtr contact = contactFromChannelId(account, channelPath, contactId);
    if(contact.isNull()) {
        return false;
    }
    qDebug() << "IMAccountsModel::isContactKnown: contact returned by channel handle: " << contact->id();

    //
    if(contact->subscriptionState() == Tp::Contact::PresenceStateYes) {
        return true;
    } else {
        return false;
    }
}

QString IMAccountsModel::accountDisplayName(const QString &iconName, const QString &displayName) const
{
    if (!mTelepathyManager) {
        return displayName;
    }

    return mTelepathyManager->accountDisplayName(iconName, displayName);
}

void IMAccountsModel::setAccountPassword(const QString &account, const QString &password)
{
    mAccountPasswords[account] = password;
    if (mServerAuthAgents.contains(account)) {
        mServerAuthAgents[account]->setPassword(password);
    }
}

bool IMAccountsModel::isAccountRegistered(const QString &cm, const QString &protocol, const QString &displayName) const
{
    for (int i = 0; i < rowCount(); ++i) {
        QObject *obj = index(i).data(ItemRole).value<QObject*>();
        Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(obj);
        if (!accountItem) {
            continue;
        }

        Tp::AccountPtr account = accountItem->account();
        if (account->cmName() == cm &&
            account->protocolName() == protocol &&
            account->displayName() == displayName) {
            return true;
        }
    }
    return false;
}

void IMAccountsModel::blockContact(const QString &accountId, const QString &contactId)
{
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if(contactItem) {
        Tp::ContactPtr contact = contactItem->contact();
        if(!contact->isBlocked()) {
            qDebug() << "IMAccountsModel::blockContact: blocking contact";
            connect(contact->block(true), SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onBlockedContact(Tp::PendingOperation*)));
        }
    }
}

void IMAccountsModel::unblockContact(const QString &accountId, const QString &contactId)
{
    Tpy::ContactModelItem *contactItem = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if(contactItem) {
        Tp::ContactPtr contact = contactItem->contact();
        if(contact->isBlocked()) {
            connect(contact->block(false), SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onUnblockedContact(Tp::PendingOperation*)));
        }
    }
}

void IMAccountsModel::removeContact(const QString &accountId, const QString &contactId)
{
    qDebug() << "IMAccountsModel::removeContact:" << accountId << contactId;
    Tpy::ContactModelItem *item = qobject_cast<Tpy::ContactModelItem*>(contactItemForId(accountId, contactId));
    if (!item) {
        return;
    }

    item->contact()->manager()->removeContacts(QList<Tp::ContactPtr>() << item->contact());
}

void IMAccountsModel::onBlockedContact(Tp::PendingOperation *op)
{

    qDebug() << "IMAccountsModel::onBlockedContact:";

    if (op->isError()) {
        qWarning() << "Cannot block contact: " << op->errorMessage();
        return;
    }
}

void IMAccountsModel::onUnblockedContact(Tp::PendingOperation *op)
{

    qDebug() << "IMAccountsModel::onUnblockedContact:";

    if (op->isError()) {
        qWarning() << "Cannot unblock contact: " << op->errorMessage();
        return;
    }
}

void IMAccountsModel::onNetworkStatusChanged(bool isOnline)
{
    if(isOnline) {
        // reconnect when network is back online
        for (int i = 0; i < rowCount(); ++i) {
            Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                        accountItemForId(index(i, 0).data(Tpy::AccountsModel::IdRole).toString()));

            // this may not work if AutomaticPresence returns null while the account is offline
            accountItem->setRequestedPresence(
                        accountItem->data(AccountsModel::AutomaticPresenceRole).toInt(),
                        accountItem->data(AccountsModel::AutomaticPresenceTypeRole).toString(),
                        accountItem->data(AccountsModel::AutomaticPresenceStatusMessageRole).toString());
        }
    } else {
        // disconnect all accounts when network is offline
        for (int i = 0; i < rowCount(); ++i) {
            Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                        accountItemForId(index(i, 0).data(Tpy::AccountsModel::IdRole).toString()));
            if(accountItem) {
                accountItem->setRequestedPresence(Tp::ConnectionPresenceTypeOffline, QString(), QString());
            }
        }
    }
}

void IMAccountsModel::onAccountConnectionStatusChanged(const QString &accountId, const int status)
{
    qDebug() << "IMAccountsModel::onAccountConnectionStatusChanged";
    Tpy::AccountsModelItem* accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (accountItem) {
        Tp::ConnectionPtr connection = accountItem->account()->connection();
        if (!connection.isNull()
                && connection->isValid()
                && status == Tp::ConnectionStatusConnected) {
            if (connection->actualFeatures().contains(Tp::Connection::FeatureRoster)) {
                accountItem->addKnownContacts();
            }
        }
    }
}

void IMAccountsModel::onConnectionReady(Tp::ConnectionPtr connection)
{
    qDebug() << "IMAccountsModel::onConnectionReady";
    for (int i = 0; i < rowCount(); ++i) {
        Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                    accountItemForId(index(i, 0).data(Tpy::AccountsModel::IdRole).toString()));

        if(accountItem) {
            Tp::AccountPtr account = accountItem->account();
            if(account->connection() == connection) {
                accountItem->addKnownContacts();
            }
        }
    }
}

QStringList IMAccountsModel::channelContacts(const QString &accountId, const QString &channelPath) const
{
    QStringList contactsList;
    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(
                accountItemForId(accountId));

    if(accountItem) {
        Tp::AccountPtr account = accountItem->account();

        // get connection id
        ChatAgent *agent = chatAgentByKey(account->uniqueIdentifier(), channelPath);
        if (!agent) {
            return QStringList();
        }

        if (account->connection().isNull()) {
            return QStringList();
        }

        // if not a conference channel, the contact can be used as is
        if (!agent->textChannel()->isConference()) {
            foreach (Tp::ContactPtr contact, agent->textChannel()->groupContacts().toList()) {
                contactsList.append(contact->id());
            }
        } else {
            Tp::ContactManagerPtr contactManager = account->connection()->contactManager();

            if (contactManager.isNull()) {
                return QStringList();
            }

            foreach (Tp::ContactPtr contact, agent->textChannel()->groupContacts().toList()) {
                // get the handle of the contact and get the global handle
                Tp::ReferencedHandles channelHandleList = contact->handle();
                uint channelHandle = channelHandleList.first();
                Tp::ContactPtr refContact = agent->contactByChannelHandle(channelHandle);
                if(!refContact.isNull()) {
                    contactsList.append(refContact->id());
                }
            }
        }
    }
    return contactsList;
}

QList<ChatAgent *> IMAccountsModel::chatAgentsByAccount(const QString &accountId) const
{
    QList<ChatAgent *> agentsList;

    foreach (const QString &key, mChatAgents.uniqueKeys()) {
        QString subkey = key.left(key.indexOf('&'));
        if(subkey == accountId) {
            agentsList.append(mChatAgents.value(key));
        }

    }
    return agentsList;
}

QList<FileTransferAgent *> IMAccountsModel::fileTransferAgentsByAccount(const QString &accountId) const
{
    QList<FileTransferAgent *> agentsList;

    foreach (const QString &key, mFileTransferAgents.uniqueKeys()) {
        QString subkey = key.left(key.indexOf('&'));
        if(subkey == accountId) {
            agentsList.append(mFileTransferAgents.value(key));
        }

    }
    return agentsList;
}

int IMAccountsModel::pendingMessagesByAccount(const QString &accountId) const
{
    int messages = 0;
    foreach(ChatAgent *agent, chatAgentsByAccount(accountId)) {
        messages += agent->pendingConversations();
    }
    return messages;
}

int IMAccountsModel::fileTransfersByAccount(const QString &accountId) const
{
    int transfers = 0;
    foreach(FileTransferAgent *agent, fileTransferAgentsByAccount(accountId)) {
        transfers += agent->pendingTransfers();
    }
    return transfers;

}

bool IMAccountsModel::openedChatByAccount(const QString &accountId) const
{
    foreach(ChatAgent *agent, chatAgentsByAccount(accountId)) {
        if (agent->existsChat()) {
            return true;
        }
    }
    return false;
}

void IMAccountsModel::clearHistory()
{
    mLogger->clearLog();
    foreach (ChatAgent *agent, mChatAgents) {
        agent->model()->clearLog();
    }
}

void IMAccountsModel::clearAccountHistory(const QString &accountId)
{
    Tpy::AccountsModelItem* accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (accountItem) {
        mLogger->clearAccount(accountItem->account());
        foreach(ChatAgent *agent, chatAgentsByAccount(accountId)) {
            agent->model()->clearLog();
        }
    }
}

void IMAccountsModel::clearContactHistory(const QString &accountId, const QString &contactId)
{
    Tpy::AccountsModelItem* accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (accountItem) {
        Tpl::LoggerPtr logger = Tpl::Logger::create();
        mLogger->clearContact(accountItem->account(), contactId);
        ChatAgent *chatAgent = chatAgentByKey(accountId, contactId);
        if(chatAgent) {
            chatAgent->model()->clearLog();
        }
    }

}

void IMAccountsModel::clearGroupChatHistory(const QString &accountId, const QString &channelPath)
{
    qDebug() << "IMAccountsModel::onAccountConnectionStatusChanged";
    Tpy::AccountsModelItem* accountItem = qobject_cast<Tpy::AccountsModelItem*>(accountItemForId(accountId));
    if (accountItem) {
        ChatAgent *chatAgent = chatAgentByKey(accountId, channelPath);
        if(chatAgent) {
            QString roomName = chatAgent->textChannel()->immutableProperties().value(TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            mLogger->clearRoom(accountItem->account(), roomName);
            chatAgent->model()->clearLog();
        }
    }
}
