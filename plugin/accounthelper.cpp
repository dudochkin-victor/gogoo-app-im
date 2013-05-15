/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "accounthelper.h"
#include "imaccountsmodel.h"
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingStringList>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/ProtocolParameter>

#include <TelepathyQt4Yell/Models/AccountsModelItem>
#include <QUrl>
#include <QImage>

AccountHelper::AccountHelper(QObject *parent) :
    QObject(parent),
    mSaslAuth(false),
    mAccountsModel(0),
    mAllowTextChatFrom(0),
    mAllowCallFrom(0),
    mAllowOutsideCallFrom(0),
    mShowMyAvatarTo(0),
    mShowMyWebStatus(0),
    mShowIHaveVideoTo(0),
    mConnectsAutomatically(true),
    mConnectAfterSetup(true)
{
    mAccountManager = Tp::AccountManager::create();
    connect(mAccountManager->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAccountManagerReady(Tp::PendingOperation*)));
}

QString AccountHelper::connectionManager() const
{
    return mConnectionManager;
}

void AccountHelper::setConnectionManager(const QString &value)
{
    mConnectionManager = value;
    emit connectionManagerChanged();
}

QString AccountHelper::protocol() const
{
    return mProtocol;
}

void AccountHelper::setProtocol(const QString &value)
{
    mProtocol = value;
    emit protocolChanged();
}

QString AccountHelper::displayName() const
{
    return mDisplayName;
}

void AccountHelper::setDisplayName(const QString &value)
{
    mDisplayName = value;
    emit displayNameChanged();
}

QString AccountHelper::password() const
{
    return mPassword;
}

void AccountHelper::setPassword(const QString &value)
{
    mPassword = value;
    emit passwordChanged();
}

QString AccountHelper::icon() const
{
    return mIcon;
}

void AccountHelper:: setIcon(const QString &value)
{
    mIcon = value;
}

QString AccountHelper::avatar() const
{
    return mAvatar;
}

void AccountHelper::setAvatar(const QString &value)
{
    if (mAccount.isNull()) {
        return;
    }

    QString localFile = QUrl(value).path();
    mAvatar = localFile;
    qDebug() << "Setting avatar from file" << localFile;

    QImage img(localFile);
    img = img.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray ba;
    QBuffer buf(&ba);

    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");

    Tp::Avatar accountAvatar;
    accountAvatar.avatarData = ba;
    accountAvatar.MIMEType = "image/png";
    mAccount->setAvatar(accountAvatar);
    emit avatarChanged();
}

bool AccountHelper::saslAuth() const
{
    return mSaslAuth;
}

void AccountHelper::setSaslAuth(bool value)
{
    mSaslAuth = value;
    emit saslAuthChanged();
}

QObject *AccountHelper::accountsModel() const
{
    return mAccountsModel;
}

void AccountHelper::setAccountsModel(QObject *value)
{
    mAccountsModel = qobject_cast<IMAccountsModel*>(value);
    emit accountsModelChanged();
}

void AccountHelper::setAccountParameter(const QString &property, const QVariant &value)
{
    mParameters[property] = value;
}

QVariant AccountHelper::accountParameter(const QString &property) const
{
    if (!mParameters.contains(property))
        return QVariant();
    return mParameters[property];
}

void AccountHelper::unsetAccountParameter(const QString &property)
{
    mUnsetParameters.append(property);
    if (mParameters.contains(property)) {
        mParameters.remove(property);
    }
}

void AccountHelper::createAccount()
{
    mParameters["account"] = mDisplayName;

    if (!mSaslAuth) {
        mParameters["password"] = mPassword;
    }

    // set the icon as a property so that the account has the
    // correct value right when it is created
    QVariantMap props;
    props["org.freedesktop.Telepathy.Account.Icon"] = mIcon;

    if (mAccount.isNull()) {
        Tp::PendingAccount *pa = mAccountManager->createAccount(mConnectionManager,
                                                                 mProtocol,
                                                                 mDisplayName,
                                                                 mParameters,
                                                                 props);
        connect(pa, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onAccountCreated(Tp::PendingOperation*)));
    }
    else {
        if (mSaslAuth && mAccountsModel) {
            mAccountsModel->setAccountPassword(mAccount->uniqueIdentifier(),
                                               mPassword);
        }
        Tp::ProtocolInfo info = mAccount->protocolInfo();
        Tp::ProtocolParameterList protocolParameters = info.parameters();

        // remove all parameters that are equal to the default value
        // and set the correct type
        foreach (Tp::ProtocolParameter param, protocolParameters) {
            if (mParameters.contains(param.name())) {
                if (mParameters[param.name()] == param.defaultValue()) {
                    mParameters.remove(param.name());
                    mUnsetParameters.append(param.name());
                } else {
                    // set the correct type
                    if (mParameters[param.name()].type() != param.type()) {
                        QString name = param.name();
                        QVariant value = mParameters[name];
                        switch (param.type()) {
                            case QVariant::Int:
                                mParameters[param.name()] = value.toInt();
                                break;
                            case QVariant::UInt:
                                mParameters[param.name()] = value.toUInt();
                                break;
                            default:
                                break;
                        }
                        // TODO: check if there is any other type that needs conversion
                    }
                }
            }
        }

        QVariantMap::iterator it = mParameters.begin();

        // set account to connect automatically
        mAccount->setConnectsAutomatically(mConnectsAutomatically);
        mAccount->setDisplayName(mDisplayName);

        Tp::PendingStringList *psl = mAccount->updateParameters(mParameters, mUnsetParameters);
        connect(psl, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onParametersUpdated(Tp::PendingOperation*)));
    }
}

void AccountHelper::setAccount(QObject *object)
{
    Tpy::AccountsModelItem *accountItem = qobject_cast<Tpy::AccountsModelItem*>(object);
    mAccount = accountItem->account();
    emit onlineChanged();

    mParameters = mAccount->parameters();
    mPassword = mParameters["password"].toString();

    mConnectsAutomatically = mAccount->connectsAutomatically();

    // load the default parameter values
    Tp::ProtocolInfo info = mAccount->protocolInfo();
    Tp::ProtocolParameterList protocolParameters = info.parameters();

    foreach (Tp::ProtocolParameter param, protocolParameters) {
        if (!mParameters.contains(param.name())) {
            QVariant value = param.defaultValue();
            if (value.isNull()) {
                continue;
            }
            mParameters[param.name()] = param.defaultValue();
        }
    }
    connect(mAccount.data(), SIGNAL(connectionStatusChanged(Tp::ConnectionStatus)),
            SIGNAL(onlineChanged()));
    emit accountChanged();
}

void AccountHelper::removeAccount()
{
    if (mAccount.isNull()) {
        return;
    }

    mAccount->remove();
}

QString AccountHelper::accountId() const
{
    if (mAccount.isNull()) {
        return QString();
    }
    return mAccount->uniqueIdentifier();
}

void AccountHelper::onAccountManagerReady(Tp::PendingOperation *op)
{
    Q_UNUSED(op);

    // TODO: check what we need to do here
}

void AccountHelper::onAccountCreated(Tp::PendingOperation *op)
{
    if (op->isError()) {
        // TODO: check how to notify errors
        return;
    }

    Tp::PendingAccount *pendingAccount = qobject_cast<Tp::PendingAccount*>(op);
    if (!pendingAccount) {
        // TODO: notify error
        return;
    }

    mAccount = pendingAccount->account();

    if (mSaslAuth && mAccountsModel) {
        mAccountsModel->setAccountPassword(mAccount->uniqueIdentifier(),
                                           mPassword);
    }

    // set account to connect automatically
    mAccount->setConnectsAutomatically(mConnectsAutomatically);

    // get the account online
    connect(mAccount->setEnabled(true), SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAccountEnabled(Tp::PendingOperation*)));
}

void AccountHelper::onAccountEnabled(Tp::PendingOperation *op)
{
    if (op->isError()) {
        // TODO: notify errors and get back to the setup screen
        return;
    }

    if (mConnectAfterSetup) {
        Tp::SimplePresence presence;
        presence.type = Tp::ConnectionPresenceTypeAvailable;
        presence.status = "online";
        presence.statusMessage = "";
        mAccount->setRequestedPresence(presence);
    }
    emit accountSetupFinished();
}

void AccountHelper::onParametersUpdated(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qDebug() << "Error:" << op->errorMessage();
        return;
    }
    Tp::PendingStringList *list = qobject_cast<Tp::PendingStringList*>(op);
    qDebug() << "Parameters on relogin:" << list->result();

    if (list->result().count() && mConnectAfterSetup) {
        mAccount->reconnect();
    }

    emit accountSetupFinished();
}

uint AccountHelper::allowTextChatFrom() const
{
    return mAllowTextChatFrom;
}

uint AccountHelper::allowCallFrom() const
{
    return mAllowCallFrom;
}

uint AccountHelper::allowOutsideCallFrom() const
{
    return mAllowOutsideCallFrom;
}

uint AccountHelper::showMyAvatarTo() const
{
    return mShowMyAvatarTo;
}

uint AccountHelper::showMyWebStatus() const
{
    return mShowMyWebStatus;
}

uint AccountHelper::showIHaveVideoTo() const
{
    return mShowIHaveVideoTo;
}

void AccountHelper::setAllowTextChatFrom(const uint &value)
{
    mAllowTextChatFrom = value;
    emit allowTextChatFromChanged();
}

void AccountHelper::setAllowCallFrom(const uint &value)
{
    mAllowCallFrom = value;
    emit allowTextChatFromChanged();
}

void AccountHelper::setAllowOutsideCallFrom(const uint &value)
{
    mAllowOutsideCallFrom = value;
    emit allowOutsideCallFromChanged();
}

void AccountHelper::setShowMyAvatarTo(const uint &value)
{
    mShowMyAvatarTo = value;
    emit showMyAvatarToChanged();
}

void AccountHelper::setShowMyWebStatus(const uint &value)
{
    mShowMyWebStatus = value;
    emit showMyWebStatusChanged();
}

void AccountHelper::setShowIHaveVideoTo(const uint &value)
{
    mShowIHaveVideoTo = value;
    emit showIHaveVideoToChanged();
}

bool AccountHelper::isOnline() const
{
    if (mAccount) {
        Tp::ConnectionPtr connection = mAccount->connection();
        if (!connection.isNull() && connection->isValid()) {
            if (connection->status() == Tp::ConnectionStatusConnected) {
                return true;
            }
        }
    }
    return false;
}

bool AccountHelper::connectsAutomatically() const
{
    return mConnectsAutomatically;
}

void AccountHelper::setConnectsAutomatically(bool value)
{
    mConnectsAutomatically = value;
    emit connectsAutomaticallyChanged();
}

void AccountHelper::introspectAccountPrivacySettings(const QString &interfaceName)
{
    if (!mAccount.isNull() && mAccount->isValid()) {
        Tp::ConnectionPtr connection = mAccount->connection();
        if (!connection.isNull() && connection->isValid()) {
            if(connection->hasInterface(interfaceName))
            {
                qDebug() << "IMAccountsModel::introspectAccountPrivacySettings: found account with privacy settings";
                Tp::Client::DBus::PropertiesInterface *properties = qobject_cast<Tp::Client::DBus::PropertiesInterface*>(connection->interface<Tp::Client::DBus::PropertiesInterface>());
                if(properties) {
                    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    properties->GetAll(interfaceName), this);
                    watcher->setProperty("accountUniqueIdentifier", QVariant(mAccount->uniqueIdentifier()));
                    connect(watcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onGotAllProperties(QDBusPendingCallWatcher*)));
                }
            }
        }
    }
}

void AccountHelper::onGotAllProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (watcher->isError()) {
        qDebug() << "Failed introspecting privacy properties";
        return;
    }
    QVariantMap map = reply.value();

    // get the account id
    QString accountId = watcher->property("accountUniqueIdentifier").toString();
    qDebug() << "AccountHelper::onGotAllProperties: got " << map.count() << " properties for account: " << accountId;


    uint allowTextChannelsFrom  = qdbus_cast<uint>(map[QLatin1String("AllowTextChannelsFrom")]);
    uint allowCallChannelsFrom  = qdbus_cast<uint>(map[QLatin1String("AllowCallChannelsFrom")]);
    uint allowOutsideCallsFrom  = qdbus_cast<uint>(map[QLatin1String("AllowOutsideCallsFrom")]);
    uint showMyAvatarTo  = qdbus_cast<uint>(map[QLatin1String("ShowMyAvatarTo")]);
    uint showMyWebStatus  = qdbus_cast<uint>(map[QLatin1String("ShowMyWebStatus")]);
    uint showIHaveVideoTo  = qdbus_cast<uint>(map[QLatin1String("ShowIHaveVideoTo")]);

    setAllowTextChatFrom(allowTextChannelsFrom);
    setAllowCallFrom(allowCallChannelsFrom);
    setAllowOutsideCallFrom(allowOutsideCallsFrom);
    setShowMyAvatarTo(showMyAvatarTo);
    setShowMyWebStatus(showMyWebStatus);
    setShowIHaveVideoTo(showIHaveVideoTo);
}

void AccountHelper::setPrivacySettings(const QString &interfaceName)
{
    if(!mAccount.isNull() && mAccount->isValid()) {
        Tp::ConnectionPtr connection = mAccount->connection();
        if(!connection.isNull()
                && connection->isValid()
                && connection->status() == Tp::ConnectionStatusConnected) {
            if(connection->hasInterface(interfaceName))
            {
                qDebug() << "AccountHelper::setPrivacySettings: setting properties via D-Bus interface";
                Tp::Client::DBus::PropertiesInterface *properties = qobject_cast<Tp::Client::DBus::PropertiesInterface*>(connection->interface<Tp::Client::DBus::PropertiesInterface>());
                if(properties) {
                    QDBusPendingCallWatcher *chatWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("AllowTextChannelsFrom"), QDBusVariant(QVariant(allowTextChatFrom()))), this);
                    chatWatcher->setProperty("accountId", accountId());
                    chatWatcher->setProperty("interfaceName", interfaceName);
                    connect(chatWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));

                    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("AllowCallChannelsFrom"), QDBusVariant(QVariant(allowCallFrom()))), this);
                    callWatcher->setProperty("accountId", accountId());
                    callWatcher->setProperty("interfaceName", interfaceName);
                    connect(callWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));

                    QDBusPendingCallWatcher *outsideWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("AllowOutsideCallsFrom"), QDBusVariant(QVariant(allowOutsideCallFrom()))), this);
                    outsideWatcher->setProperty("accountId", accountId());
                    outsideWatcher->setProperty("interfaceName", interfaceName);
                    connect(outsideWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));

                    QDBusPendingCallWatcher *avatarWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("ShowMyAvatarTo"), QDBusVariant(QVariant(showMyAvatarTo()))), this);
                    avatarWatcher->setProperty("accountId", accountId());
                    avatarWatcher->setProperty("interfaceName", interfaceName);
                    connect(avatarWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));

                    QDBusPendingCallWatcher *webWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("ShowMyWebStatus"), QDBusVariant(QVariant(showMyWebStatus()))), this);
                    webWatcher->setProperty("accountId", accountId());
                    webWatcher->setProperty("interfaceName", interfaceName);
                    connect(webWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));

                    QDBusPendingCallWatcher *videoWatcher = new QDBusPendingCallWatcher(
                    properties->Set(interfaceName, QString("ShowIHaveVideoTo"), QDBusVariant(QVariant(showIHaveVideoTo()))), this);
                    videoWatcher->setProperty("accountId", accountId());
                    videoWatcher->setProperty("interfaceName", interfaceName);
                    connect(videoWatcher,
                            SIGNAL(finished(QDBusPendingCallWatcher*)),
                            SLOT(onSetPrivacyProperty(QDBusPendingCallWatcher*)));
                }
            }
        }
    }
}

void AccountHelper::onSetPrivacyProperty(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        qDebug() << "Failed setting privacy property";
        return;
    }

    qDebug() << "Privacy property set";

    // update the privacy settings
    QString accountId = watcher->property("accountId").toString();
    QString interfaceName = watcher->property("interfaceName").toString();
    if(!accountId.isEmpty()) {
        introspectAccountPrivacySettings(interfaceName);
    }
}

bool AccountHelper::connectAfterSetup() const
{
    return mConnectAfterSetup;
}

void AccountHelper::setConnectAfterSetup(bool value)
{
    mConnectAfterSetup = value;
    emit connectAfterSetupChanged();
}
