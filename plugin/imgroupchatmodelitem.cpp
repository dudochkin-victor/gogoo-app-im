/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imgroupchatmodelitem.h"

#include "imaccountsmodel.h"

#include <TelepathyQt4Yell/Models/AccountsModel>
#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/ReceivedMessage>


IMGroupChatModelItem::IMGroupChatModelItem(const QString &accountId, const Tp::TextChannelPtr &channel) :
    mAccountId(accountId),
    mTextChannel(channel)
{
    connect(mTextChannel.data(),
            SIGNAL(groupMembersChanged(Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(groupMembersChanged(Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onGroupMembersChanged()));
    connect(mTextChannel.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(groupFlagsChanged(Tp::ChannelGroupFlags,Tp::ChannelGroupFlags,Tp::ChannelGroupFlags)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(groupSelfContactChanged()),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(groupHandleOwnersChanged(Tp::HandleOwnerMap,Tp::UIntList,Tp::UIntList)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(chatStateChanged(Tp::ContactPtr,Tp::ChannelChatState)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(propertyChanged(QString)),
            SLOT(onChanged()));
    connect(mTextChannel.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(onChanged()));
}

QVariant IMGroupChatModelItem::data(int role) const
{
    switch(role)
    {
        case Tpy::AccountsModel::ItemRole:
            return QVariant::fromValue(
                const_cast<QObject *>(
                    static_cast<const QObject *>(this)));
        case Tpy::AccountsModel::IdRole: {
            return mTextChannel->objectPath();
    }
        case Qt::DisplayRole:
        case Tpy::AccountsModel::AliasRole: {
            QString alias;
            int contactsCount = mTextChannel->groupContacts().count();
            if(mTextChannel->groupContacts().contains(mTextChannel->groupSelfContact())) {
                --contactsCount;
            }
            alias = tr("%1 chatting").arg(QString(contactsCount));
            return alias;
        }
        case IMAccountsModel::PresenceTypesListRole: {
            QList<QVariant> presenceList;
            foreach(Tp::ContactPtr contact, mTextChannel->groupContacts()) {
                if(contact != mTextChannel->groupSelfContact()) {
                    presenceList.append(QVariant(contact->presence().type()));
                }
            }
            return QVariant(presenceList);
        }
        case Tpy::AccountsModel::PresenceTypeRole:
            return QVariant(Tp::ConnectionPresenceTypeAvailable);
        case Tpy::AccountsModel::AvatarRole: {
            return QVariant(QLatin1String("MULTIPLE"));
        }
        case IMAccountsModel::AvatarsListRole: {
            QStringList avatars;
            foreach(Tp::ContactPtr contact, mTextChannel->groupContacts()) {
                if(contact != mTextChannel->groupSelfContact()) {
                    avatars.append(contact->avatarData().fileName);
                }
            }
            return QVariant(avatars);
        }
        case IMAccountsModel::PendingMessagesRole: {
            return QVariant(mTextChannel->messageQueue().count());
        }
        case IMAccountsModel::IsGroupChatRole:
            return QVariant(mTextChannel->isConference());
        case Tpy::AccountsModel::PresenceMessageRole:
            return QVariant(QString());
        case Tpy::AccountsModel::PresenceStatusRole:
            return Tp::ConnectionPresenceTypeAvailable;
        case Tpy::AccountsModel::TextChatCapabilityRole:
            return true;
        case Tpy::AccountsModel::MediaCallCapabilityRole:
            return false;
        case Tpy::AccountsModel::AudioCallCapabilityRole:
            return false;
        case Tpy::AccountsModel::VideoCallCapabilityRole:
            return false;
        case Tpy::AccountsModel::VideoCallWithAudioCapabilityRole:
            return false;
        case Tpy::AccountsModel::UpgradeCallCapabilityRole:
            return false;
        case IMAccountsModel::ChatOpenedRole:
            return true;
        case IMAccountsModel::MissedAudioCallsRole:
            return 0;
        case IMAccountsModel::MissedVideoCallsRole:
            return 0;
        case Tpy::AccountsModel::FileTransferCapabilityRole:
            return false;
        default:
            break;
    }

    return QVariant();
}

void IMGroupChatModelItem::onChanged()
{
    emit changed(this);
}

void IMGroupChatModelItem::onGroupMembersChanged()
{
    foreach(Tp::ContactPtr contact, mTextChannel->groupContacts().toList()) {
        connect(contact.data(), SIGNAL(avatarDataChanged(Tp::AvatarData)),
                SLOT(onChanged()));
        connect(contact.data(), SIGNAL(aliasChanged(QString)),
                SLOT(onChanged()));
        connect(contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
                SLOT(onChanged()));
        connect(contact.data(), SIGNAL(avatarTokenChanged(QString)),
                SLOT(onChanged()));
    }
}

Tp::TextChannelPtr IMGroupChatModelItem::channel() const
{
    return mTextChannel;
}

QString IMGroupChatModelItem::accountId() const
{
    return mAccountId;
}
