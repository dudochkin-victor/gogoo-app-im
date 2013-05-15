/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "simplecontactslistmodel.h"

#include <TelepathyQt4Yell/Models/AccountsModel>

SimpleContactsListModel::SimpleContactsListModel(const QList<Tp::ContactPtr> contacts, QObject *parent):
    QAbstractListModel(parent)
{
    beginInsertRows(QModelIndex(), 0, contacts.count() - 1);
    // create the contact model items and fill up the list
    foreach(Tp::ContactPtr contact, contacts) {
        Tpy::ContactModelItem *item = new Tpy::ContactModelItem(contact);
        mItems.append(item);
    }
    endInsertRows();

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
    roles[Tpy::AccountsModel::AliasRole] = "aliasName";
    roles[Tpy::AccountsModel::AvatarRole] = "avatar";
    roles[Tpy::AccountsModel::PresenceStatusRole] = "presenceStatus";
    roles[Tpy::AccountsModel::PresenceTypeRole] = "presenceType";
    roles[Tpy::AccountsModel::PresenceMessageRole] = "presenceMessage";
    roles[Tpy::AccountsModel::TextChatCapabilityRole] = "textChat";
    roles[Tpy::AccountsModel::MediaCallCapabilityRole] = "mediaCall";
    roles[Tpy::AccountsModel::AudioCallCapabilityRole] = "audioCall";
    roles[Tpy::AccountsModel::VideoCallCapabilityRole] = "videoCall";
    roles[Tpy::AccountsModel::VideoCallWithAudioCapabilityRole] = "videoCallWithAudio";
    roles[Tpy::AccountsModel::UpgradeCallCapabilityRole] = "upgradeCall";
    setRoleNames(roles);
}

SimpleContactsListModel::~SimpleContactsListModel()
{

}

int SimpleContactsListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mItems.count();
}

QVariant SimpleContactsListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= mItems.count()) {
        return QVariant();
    }

    Tpy::ContactModelItem *item = mItems[index.row()];

    switch(role) {
        default: {
            return item->data(role);
        }
    }
}
