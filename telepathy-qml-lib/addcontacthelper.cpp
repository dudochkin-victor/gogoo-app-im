/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "addcontacthelper.h"
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

AddContactHelper::AddContactHelper(QObject *parent) :
    QObject(parent),
    mAccountItem(0),
    mState(StateIdle)
{
}

AddContactHelper::~AddContactHelper()
{
}

void AddContactHelper::setAccountItem(QObject *accountItem)
{
    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem *>(accountItem);
    if(item) {
    qDebug() << "AddContactHelper::setAccountItem: accountId=" << item->account()->uniqueIdentifier();

    if (mState != StateSending) {
        mAccountItem = item;
        emit accountItemChanged();
    }
    }
}

QObject *AddContactHelper::accountItem()
{
    return mAccountItem;
}

void AddContactHelper::setContactId(const QString &contactId)
{
    qDebug() << "AddContactHelper::setContactId: contactId=" << contactId;

    if (mState != StateSending) {
        mContactId = contactId;
        emit contactIdChanged();
    }
}

QString AddContactHelper::contactId() const
{
    return mContactId;
}

QString AddContactHelper::error() const
{
    return mError;
}

AddContactHelper::State AddContactHelper::state() const
{
    return mState;
}

void AddContactHelper::sendRequest()
{
    qDebug() << "AddContactHelper::sendRequest()";

    if (mState == StateSending) {
        return;
    }

    if (mContactId.isEmpty()) {
        setError();
        return;
    }

    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem *>(accountItem());
    if (!item) {
        setError();
        return;
    }

    Tp::AccountPtr account = item->account();
    if (account.isNull() ||
        account->connection().isNull() ||
        account->connection()->contactManager().isNull()) {
        setError();
        return;
    }

    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList(mContactId)),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    setState(StateSending);
}

void AddContactHelper::onPendingContactsFinished(Tp::PendingOperation *op)
{
    qDebug() << "AddContactHelper::onPendingContactsFinished:";

    Tp::PendingContacts *pendingContacts = static_cast<Tp::PendingContacts*>(op);
    if (!pendingContacts) {
        setError();
        return;
    }

    if (pendingContacts->isError()) {
        if (op->errorName() == TP_QT_ERROR_NETWORK_ERROR) {
            setError("A network error occurred. Please check your connection and try again.", StateNoNetwork);
        } else {
            setError(tr("An error occurred while trying to complete your request. Please try again."));
        }
        return;
    }

    if (pendingContacts->contacts().size() <= 0) {
        setError();
        return;
    }

    Tp::ContactPtr contactPtr = pendingContacts->contacts().at(0);
    if (contactPtr.isNull()) {
        setError();
        return;
    }

    Tpy::AccountsModelItem *item = qobject_cast<Tpy::AccountsModelItem *>(accountItem());
    if (!item) {
        setError();
        return;
    }

    Tp::AccountPtr account = item->account();
    if (account.isNull() ||
        account->connection().isNull() ||
        account->connection()->contactManager().isNull()) {
        setError();
        return;
    }

    Tp::PendingOperation * op2 = account->connection()->contactManager()->requestPresenceSubscription(pendingContacts->contacts());
    if (!op2) {
        setError();
        return;
    }

    connect(op2, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPresenceSubscriptionFinished(Tp::PendingOperation*)));
}

void AddContactHelper::onPresenceSubscriptionFinished(Tp::PendingOperation *op)
{
    qDebug() << "AddContactHelper::onPresenceSubscriptionFinished";

    if (!op || op->isError()) {
        if (op->errorName() == TP_QT_ERROR_NETWORK_ERROR) {
            setError("A network error occurred. Please check your connection and try again.", StateNoNetwork);
        } else {
            setError(tr("An error occurred while trying to complete your request. Please try again."));
        }
        return;
    }

    setState(StateSent);
}

void AddContactHelper::setError(const QString &error, AddContactHelper::State state)
{
    qDebug() << "AddContactHelper::setError: error=" << error << " state=" << state;

    if (error.isEmpty()) {
        mError = tr("Account not found. Please enter a valid account name.");
    } else {
        mError = error;
    }
    setState(state);
}

void AddContactHelper::setState(const AddContactHelper::State &state)
{
    qDebug() << "AddContactHelper::setState: state=" << " state=" << state;
    mState = state;
    emit stateChanged(mState);
}

void AddContactHelper::resetHelper()
{
    setState(StateIdle);
    mError.clear();
    emit errorChanged();
    mContactId.clear();
    emit contactIdChanged();
}
