/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef TELEPATHYMANAGER_H
#define TELEPATHYMANAGER_H

#include <QObject>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/Types>
#include "channelhandler.h"

class TelepathyManager : public QObject
{
    Q_OBJECT

public:
    explicit TelepathyManager(bool fullStart = false);
    ~TelepathyManager();
    Tp::AccountManagerPtr accountManager(void);
    ChannelHandler *channelHandler() const;
    QList<Tp::AccountPtr> accounts(void);

    // These two methods are to provide information to the MeeGo Contacts application
    // Please check with them before modifying the API or the format of the returned values
    Q_INVOKABLE QVariantMap availableAccounts() const;
    Q_INVOKABLE QStringList availableContacts(QString accountId) const;
    void setProtocolNames(QMap<QString, QString> names);
    QString accountDisplayName(const QString &iconName, const QString &displayName) const;
    bool isFinished() const;

public Q_SLOTS:
    void initializeAccounts();
    void initializeConnection(const Tp::ConnectionPtr &conn);
    void initializeContacts(const Tp::ContactManagerPtr &contactManager);
    void initializeChannelHandler();
    void registerClients(void);

Q_SIGNALS:
    void accountManagerReady();
    void accountAvailable(Tp::AccountPtr account);
    void connectionAvailable(Tp::ConnectionPtr connection);
    void finished();
    void handlerRegistered();

protected:
    Q_INVOKABLE QString accountServiceName(const QString &iconName) const;
    Q_INVOKABLE int accountsOfType(const QString &iconName) const;

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onNewAccount(const Tp::AccountPtr &account);
    void onConnectionChanged(const Tp::ConnectionPtr &connection);
    void onTextChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel);
    void onFinished();
    void onAccountConnectionChanged(const Tp::ConnectionPtr &conn);

private:
    Tp::AccountManagerPtr mAccountManager;
    Tp::Features mAccountManagerFeatures;
    QList<Tp::AccountPtr> mAccounts;
    Tp::Features mAccountFeatures;
    Tp::Features mContactFeatures;
    Tp::Features mConnectionFeatures;
    ChannelHandler *mChannelHandler;
    Tp::ClientRegistrarPtr mClientRegistrar;
    bool mFullStart;
    bool mFinished;
    QMap<QString,QString> mProtocols;
};

#endif // TELEPATHYMANAGER_H
