/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef ADDCONTACTHELPER_H
#define ADDCONTACTHELPER_H

#include <QObject>
#include <TelepathyQt4/Account>
#include <TelepathyQt4Yell/Models/AccountsModelItem>

class AddContactHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *accountItem READ accountItem WRITE setAccountItem NOTIFY accountItemChanged);
    Q_PROPERTY(QString contactId READ contactId WRITE setContactId NOTIFY contactIdChanged);
    Q_PROPERTY(QString error READ error NOTIFY errorChanged);
    Q_PROPERTY(AddContactHelper::State state READ state NOTIFY stateChanged);
    Q_ENUMS(State);

public:
    typedef enum {
        StateIdle,
        StateSending,
        StateSent,
        StateError,
        StateNoNetwork
    } State;

    explicit AddContactHelper(QObject *parent = 0);
    virtual ~AddContactHelper();

    void setAccountItem(QObject *account);
    void setContactId(const QString &contactId);

    QObject *accountItem();
    QString contactId() const;
    QString error() const;
    AddContactHelper::State state() const;

    Q_INVOKABLE void resetHelper();

signals:
    void stateChanged(const AddContactHelper::State &newState);
    void accountItemChanged();
    void contactIdChanged();
    void errorChanged();
    void stateChanged();

public slots:
    void sendRequest();

private slots:
    void onPendingContactsFinished(Tp::PendingOperation *op);
    void onPresenceSubscriptionFinished(Tp::PendingOperation *op);

private:
    Tp::AccountPtr account();
    void setError(const QString &error = QString(), AddContactHelper::State state = StateError);
    void setState(const AddContactHelper::State &state);

    QObject *mAccountItem;
    QString mContactId;
    State mState;
    QString mError;
};

#endif // ADDCONTACTHELPER_H
