/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imgroupchatmodel.h"

#include "imaccountsmodel.h"
#include <TelepathyQt4Yell/Models/AccountsModel>

IMGroupChatModel::IMGroupChatModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[Tpy::AccountsModel::ItemRole] = "item";
    roles[Tpy::AccountsModel::IdRole] = "id";
    roles[Tpy::AccountsModel::ValidRole] = "valid";
    roles[Tpy::AccountsModel::EnabledRole] = "enabled";
    roles[Tpy::AccountsModel::ConnectionManagerNameRole] = "connectionManager";
    roles[Tpy::AccountsModel::ProtocolNameRole] = "protocol";
    roles[Tpy::AccountsModel::DisplayNameRole] = "displayName";
    roles[Tpy::AccountsModel::IconRole] = "icon";
    roles[Tpy::AccountsModel::NicknameRole] = "nickname";
    roles[Tpy::AccountsModel::ConnectsAutomaticallyRole] = "connectsAutomatically";
    roles[Tpy::AccountsModel::ChangingPresenceRole] = "changingPresence";
    roles[Tpy::AccountsModel::AutomaticPresenceRole] = "automaticPresence";
    roles[Tpy::AccountsModel::CurrentPresenceRole] = "status";
    roles[Tpy::AccountsModel::CurrentPresenceTypeRole] = "statusType";
    roles[Tpy::AccountsModel::CurrentPresenceStatusMessageRole] = "statusMessage";
    roles[Tpy::AccountsModel::RequestedPresenceRole] = "requestedStatus";
    roles[Tpy::AccountsModel::RequestedPresenceTypeRole] = "requestedStatusType";
    roles[Tpy::AccountsModel::RequestedPresenceStatusMessageRole] = "requestedStausMessage";
    roles[Tpy::AccountsModel::ConnectionStatusRole] = "connectionStatus";
    roles[Tpy::AccountsModel::ConnectionStatusReasonRole] = "connectionStatusReason";
    roles[Tpy::AccountsModel::AliasRole] = "aliasName";
    roles[Tpy::AccountsModel::AvatarRole] = "avatar";
    roles[Tpy::AccountsModel::PresenceStatusRole] = "presenceStatus";
    roles[Tpy::AccountsModel::PresenceTypeRole] = "presenceType";
    roles[Tpy::AccountsModel::PresenceMessageRole] = "presenceMessage";
    roles[Tpy::AccountsModel::SubscriptionStateRole] = "subscriptionState";
    roles[Tpy::AccountsModel::PublishStateRole] = "publishState";
    roles[Tpy::AccountsModel::BlockedRole] = "blocked";
    roles[Tpy::AccountsModel::GroupsRole] = "groups";
    roles[Tpy::AccountsModel::TextChatCapabilityRole] = "textChat";
    roles[Tpy::AccountsModel::MediaCallCapabilityRole] = "mediaCall";
    roles[Tpy::AccountsModel::AudioCallCapabilityRole] = "audioCall";
    roles[Tpy::AccountsModel::VideoCallCapabilityRole] = "videoCall";
    roles[Tpy::AccountsModel::VideoCallWithAudioCapabilityRole] = "videoCallWithAudio";
    roles[Tpy::AccountsModel::UpgradeCallCapabilityRole] = "upgradeCall";
    roles[IMAccountsModel::PendingMessagesRole] = "pendingMessages";
    roles[IMAccountsModel::ChatOpenedRole] = "chatOpened";
    roles[IMAccountsModel::LastPendingMessageRole] = "lastPendingMessage";
    roles[IMAccountsModel::LastPendingMessageSentRole] = "lastPendingMessageSent";
    roles[IMAccountsModel::MissedVideoCallsRole] = "missedVideoCalls";
    roles[IMAccountsModel::MissedAudioCallsRole] = "missedAudioCalls";
    roles[IMAccountsModel::ExistingCallRole] = "existingCall";
    roles[IMAccountsModel::IncomingVideoCallRole] = "incomingVideoCall";
    roles[IMAccountsModel::IncomingAudioCallRole] = "incomingAudioCall";
    roles[IMAccountsModel::ModelDataRole] = "modelData";
    roles[IMAccountsModel::PresenceTypesListRole] = "presenceTypesList";
    roles[IMAccountsModel::AvatarsListRole] = "avatarsList";
    setRoleNames(roles);
}

QVariant IMGroupChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= mChildren.count()) {
        return QVariant();
    }

    IMGroupChatModelItem *item = mChildren[index.row()];

    switch(role) {
        default: {
            return item->data(role);
        }
    }
}

int IMGroupChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mChildren.count();
}

void IMGroupChatModel::onTextChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel)
{
    if(channel->isConference()) {
        // accept the group request
        if(channel->groupLocalPendingContacts().count() > 0) {
            Tp::Contacts contacts = channel->groupLocalPendingContacts();
            foreach(Tp::ContactPtr contact, contacts.values()) {
                if(contact == channel->groupSelfContact()) {
                    channel->groupAddContacts(QList<Tp::ContactPtr>() << contact);
                }
            }
        }

        // add item to model
        IMGroupChatModelItem *item = new IMGroupChatModelItem(accountId, channel);
        connect(item, SIGNAL(changed(IMGroupChatModelItem*)),
                SLOT(onItemChanged(IMGroupChatModelItem*)));
        connect(channel.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                SLOT(onChannelInvalidated()));

        beginInsertRows(QModelIndex(), mChildren.count(), mChildren.count());
        mChildren.append(item);
        endInsertRows();
    }
}

void IMGroupChatModel::onChannelInvalidated()
{
    Tp::Channel *channel = qobject_cast<Tp::Channel *>(sender());
    foreach(IMGroupChatModelItem *item, mChildren) {
        if(channel->objectPath() == item->channel()->objectPath()) {
            beginRemoveRows(QModelIndex(), mChildren.indexOf(item), mChildren.indexOf(item));
            mChildren.removeOne(item);
            delete item;
            endRemoveRows();
        }
    }
}

QModelIndex IMGroupChatModel::index(IMGroupChatModelItem *item)
{
    if(item) {
        return createIndex(mChildren.indexOf(item), 0, item);
    }
    return QModelIndex();
}

void IMGroupChatModel::onItemChanged(IMGroupChatModelItem *item)
{
    QModelIndex itemIndex = index(item);
    if(itemIndex.isValid()) {
        emit dataChanged(itemIndex, itemIndex);
        beginResetModel();
        endResetModel();
    }
}
