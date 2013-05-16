/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "components.h"
#include "accountsmodelfactory.h"
#include "accountssortfilterproxymodel.h"
#include "contactssortfilterproxymodel.h"
#include "accounthelper.h"
#include "imaccountsmodel.h"
#include "imavatarimageprovider.h"

#include "../telepathy-qml-lib/chatagent.h"
#include "../telepathy-qml-lib/imchannelapprover.h"

#include <TelepathyLoggerQt4/Init>
#include <TelepathyQt/Account>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Debug>
#include <TelepathyQt4Yell/Models/FlatModelProxy>
#include <TelepathyQt4Yell/Types>
#include <TelepathyQt4Yell/CallChannel>

#include <QtDebug>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/qdeclarative.h>
#include <QSettings>
#include <QtGstQmlSink/qmlgstvideoitem.h>

#include <glib-object.h>

//#include "imtextedit.h"

void Components::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    qDebug() << "MeeGoIM initializeEngine" << uri;
    Q_ASSERT(engine);

    // needed for tp-logger
    g_type_init();

    Tp::registerTypes();
    //Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tpy::registerTypes();

    Tpl::init();

    mTpManager = new TelepathyManager(true);
    connect(mTpManager, SIGNAL(accountManagerReady()), SLOT(onAccountManagerReady()));

    mProtocolsModel = new IMProtocolsModel(this);
    mTpManager->setProtocolNames(mProtocolsModel->protocolNames());

    AccountsModelFactory *accountFactory = new AccountsModelFactory(mTpManager);
    connect(accountFactory, SIGNAL(modelCreated(IMAccountsModel*)),SLOT(onAccountsModelReady(IMAccountsModel*)));

    mRootContext = engine->rootContext();
    Q_ASSERT(mRootContext);

    mRootContext->setContextProperty(QString::fromLatin1("accountsModel"), (QObject *) 0);

    mRootContext->setContextProperty(QString::fromLatin1("telepathyManager"), mTpManager);
    mRootContext->setContextProperty(QString::fromLatin1("protocolsModel"), mProtocolsModel);
    mRootContext->setContextProperty(QString::fromLatin1("accountFactory"), accountFactory);

    // create the notification manager
    mNotificationManager = new NotificationManager(this);
    mRootContext->setContextProperty(QString::fromLatin1("notificationManager"),
                                      mNotificationManager);

    // create the notification manager
    mRootContext->setContextProperty(QString::fromLatin1("settingsHelper"),
                                      SettingsHelper::self());

    // get the network status and load it
    mNetworkConfigManager = new QNetworkConfigurationManager();
    connect(mNetworkConfigManager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetworkStatusChanged(bool)));
    onNetworkStatusChanged(mNetworkConfigManager->isOnline());
}

void Components::registerTypes(const char *uri)
{
    qmlRegisterType<AccountHelper>(uri, 0, 1, "AccountHelper");
    qmlRegisterType<QmlGstVideoItem>(uri, 0, 1, "VideoItem");
    qmlRegisterUncreatableType<IMAccountsModel>(uri, 0, 1, "IMAccountsModel", "This is a read-only type");
    qmlRegisterUncreatableType<Tpy::ContactModelItem>(uri, 0, 1,"ContactModelItem", "This is a read-only type");
    qmlRegisterUncreatableType<TelepathyManager>(uri, 0, 1, "TelepathyManager", "This is a read-only type");
    qmlRegisterUncreatableType<IMChannelApprover>(uri, 0, 1, "IMChannelApprover", "This is a read-only type");
    qmlRegisterUncreatableType<ChatAgent>(uri, 0, 1, "ChatAgent", "This is a read-only type");
    qmlRegisterUncreatableType<SimpleContactsListModel>(uri, 0, 1, "SimpleContactsListModel", "This is a read-only type");
}

void Components::onAccountManagerReady()
{
    // register the avatar image provider
    IMAvatarImageProvider::registerProvider(mRootContext->engine(), mTpManager->accountManager());
}

void Components::onAccountsModelReady(IMAccountsModel *model)
{
    mAccountsModel = model;
    mAccountsModel->setTelepathyManager(mTpManager);
    Tpy::FlatModelProxy *flatModel = new Tpy::FlatModelProxy(model);
    mMergedModel = new MergedModel(this);
    mMergedModel->addModel(flatModel);

    mGroupChatModel = new IMGroupChatModel(this);
    mMergedModel->addModel(mGroupChatModel);

    // initialize the accounts sorted model
    AccountsSortFilterProxyModel *accountsSortedModel = new AccountsSortFilterProxyModel(model, this);

    mContactsModel = new ContactsSortFilterProxyModel(mTpManager, mMergedModel, this);
    mRequestsModel = new ContactsSortFilterProxyModel(mTpManager, mMergedModel, this);
    mRequestsModel->setRequestsOnly(true);

    // this call will make sure that known contacts for each logged in account are loaded
    // after all components are in place
    onContactsUpgraded();

    // get last used account
    QSettings settings("MeeGo", "MeeGoIM");
    QString lastUsedAccount = settings.value("LastUsed/Account", QString()).toString();

    // the load order is inverted so that signals emitted by the accountsModel can guarantee that the
    // contactsModel is present
    mRootContext->setContextProperty(QString::fromLatin1("contactsModel"), mContactsModel);
    emit contactsModelCreated();
    mRootContext->setContextProperty(QString::fromLatin1("contactRequestModel"), mRequestsModel);
    emit requestsModelCreated();
    mRootContext->setContextProperty(QString::fromLatin1("accountsModel"), model);
    mRootContext->setContextProperty(QString::fromLatin1("accountsSortedModel"), accountsSortedModel);
    emit accountsModelCreated();

    connect(mTpManager, SIGNAL(handlerRegistered()), SLOT(onHandlerRegistered()));
    connect(mTpManager, SIGNAL(connectionAvailable(Tp::ConnectionPtr)),
            this, SLOT(onContactsUpgraded()));
    connect(mNetworkConfigManager, SIGNAL(onlineStateChanged(bool)),
            mAccountsModel, SLOT(onNetworkStatusChanged(bool)));

    mAccountsModel->setNotificationManager(mNotificationManager);

    // this signals that all components have been loaded at this point
    // in turn, this signal can be used to call the tpManager and register
    // the handler and approver, if necessary
    mAccountsModel->onComponentsLoaded();

    loadLastUsedAccount(lastUsedAccount, model);
}

void Components::onContactsUpgraded()
{
    for (int i = 0; i < mAccountsModel->rowCount(); ++i) {
        QModelIndex index = mAccountsModel->index(i, 0, QModelIndex());
        Tpy::AccountsModelItem *accountItem = static_cast<Tpy::AccountsModelItem *>(index.data(Tpy::AccountsModel::ItemRole).value<QObject *>());
        accountItem->addKnownContacts();
    }
    mContactsModel->slotResetModel();
}

/**
  * This method checks whether the last used account is connected. If it is connected
  * it will trigger a signal to open the list of contacts for that account.
  * That is done through a signal in the contacts proxy model as a workaround because
  * this class cannot send a signal to the QML files on its own.
  */
void Components::loadLastUsedAccount(const QString accountId, IMAccountsModel *model)
{
    // if not empty
    if (accountId.isEmpty()) {
        return;
    }
    // locate the account object matching the id
    for (int i = 0; i < model->accountCount(); ++i) {
        QModelIndex index = model->index(i, 0, QModelIndex());
        Tp::AccountPtr account = model->accountForIndex(index);
        if (account->uniqueIdentifier() == accountId) {
            // only send the signal if the account is connected
            if (!account->connection().isNull()
                    && account->connection()->isValid()
                    && account->connection()->status() == Tp::ConnectionStatusConnected) {
                mContactsModel->filterByLastUsedAccount(accountId);
                break;
            }
        }
    }
}

void Components::onNetworkStatusChanged(bool isOnline)
{
    mRootContext->setContextProperty(QString::fromLatin1("networkOnline"), QVariant(isOnline));
}

void Components::onHandlerRegistered()
{
    // only do it if the handler has been created
    if (mTpManager->channelHandler()) {
        connect(mTpManager->channelHandler(),
                SIGNAL(textChannelAvailable(QString,Tp::TextChannelPtr)),
                mAccountsModel,
                SLOT(onTextChannelAvailable(QString,Tp::TextChannelPtr)));
        connect(mTpManager->channelHandler(),
                SIGNAL(callChannelAvailable(QString,Tpy::CallChannelPtr)),
                mAccountsModel,
                SLOT(onCallChannelAvailable(QString,Tpy::CallChannelPtr)));
        connect(mTpManager->channelHandler(),
                SIGNAL(incomingFileTransferChannelAvailable(QString,Tp::IncomingFileTransferChannelPtr)),
                mAccountsModel,
                SLOT(onIncomingFileTransferChannelAvailable(QString,Tp::IncomingFileTransferChannelPtr)));
        connect(mTpManager->channelHandler(),
                SIGNAL(outgoingFileTransferChannelAvailable(QString,Tp::OutgoingFileTransferChannelPtr,Tp::ChannelRequestPtr)),
                mAccountsModel,
                SLOT(onOutgoingFileTransferChannelAvailable(QString,Tp::OutgoingFileTransferChannelPtr, Tp::ChannelRequestPtr)));
        connect(mTpManager->channelHandler(),
                SIGNAL(serverAuthChannelAvailable(QString,Tp::ChannelPtr)),
                mAccountsModel,
                SLOT(onServerAuthChannelAvailable(QString,Tp::ChannelPtr)));
        connect(mTpManager->channelHandler(),
                SIGNAL(textChannelAvailable(QString,Tp::TextChannelPtr)),
                mGroupChatModel,
                SLOT(onTextChannelAvailable(QString,Tp::TextChannelPtr)));
    }
}

Q_EXPORT_PLUGIN2(components, Components);
