/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "contactssortfilterproxymodel.h"
#include "imaccountsmodel.h"
#include "imgroupchatmodel.h"
#include "settingshelper.h"

#include "../telepathy-qml-lib/telepathymanager.h"
#include "../telepathy-qml-lib/channelhandler.h"
#include "../telepathy-qml-lib/mergedmodel.h"
#include "../telepathy-qml-lib/notificationmanager.h"
#include "../telepathy-qml-lib/improtocolsmodel.h"

#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeExtensionPlugin>
#include <QtNetwork/QNetworkConfigurationManager>

class Components : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void initializeEngine(QDeclarativeEngine *engine, const char *uri);
    void registerTypes(const char *uri);

public Q_SLOTS:
    void onAccountManagerReady();
    void onAccountsModelReady(IMAccountsModel *model);

Q_SIGNALS:
    void accountsModelCreated();
    void contactsModelCreated();
    void chatModelCreated();
    void requestsModelCreated();

protected:
    void loadLastUsedAccount(const QString accountId, IMAccountsModel *model);

private Q_SLOTS:
    void onContactsUpgraded();
    void onNetworkStatusChanged(bool isOnline);
    void onHandlerRegistered();
    void onApproverRegistered();

private:
    QNetworkConfigurationManager *mNetworkConfigManager;
    QDeclarativeContext *mRootContext;
    TelepathyManager *mTpManager;
    ChannelHandler *mChannelHandler;
    IMAccountsModel *mAccountsModel;
    ContactsSortFilterProxyModel *mContactsModel;
    ContactsSortFilterProxyModel *mRequestsModel;
    MergedModel *mMergedModel;
    NotificationManager *mNotificationManager;
    SettingsHelper *mSettingsHelper;
    IMGroupChatModel *mGroupChatModel;
    IMProtocolsModel *mProtocolsModel;

};

#endif // COMPONENTS_H
