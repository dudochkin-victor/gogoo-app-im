/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef ACCOUNTHELPER_H
#define ACCOUNTHELPER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <TelepathyQt4/AccountManager>

class IMAccountsModel;

class AccountHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionManager READ connectionManager
                                         WRITE setConnectionManager
                                         NOTIFY connectionManagerChanged)
    Q_PROPERTY(QString protocol READ protocol
                                WRITE setProtocol
                                NOTIFY protocolChanged)
    Q_PROPERTY(QString displayName READ displayName
                                   WRITE setDisplayName
                                   NOTIFY displayNameChanged)
    Q_PROPERTY(QString password READ password
                                   WRITE setPassword
                                   NOTIFY passwordChanged)
    Q_PROPERTY(QString icon READ icon
                            WRITE setIcon
                            NOTIFY iconChanged)
    Q_PROPERTY(QString avatar READ avatar
                              WRITE setAvatar
                              NOTIFY avatarChanged)
    Q_PROPERTY(bool saslAuth READ saslAuth
                             WRITE setSaslAuth
                             NOTIFY saslAuthChanged)
    Q_PROPERTY(QObject *model READ accountsModel
                                   WRITE setAccountsModel
                                   NOTIFY accountsModelChanged)
    Q_PROPERTY(uint allowTextChatFrom READ allowTextChatFrom
                                        WRITE setAllowTextChatFrom
                                        NOTIFY allowTextChatFromChanged)
    Q_PROPERTY(uint allowCallFrom READ allowCallFrom
                                        WRITE setAllowCallFrom
                                        NOTIFY allowCallFromChanged)
    Q_PROPERTY(uint allowOutsideCallFrom READ allowOutsideCallFrom
                                        WRITE setAllowOutsideCallFrom
                                        NOTIFY allowOutsideCallFromChanged)
    Q_PROPERTY(uint showMyAvatarTo READ showMyAvatarTo
                                    WRITE setShowMyAvatarTo
                                    NOTIFY showMyAvatarToChanged)
    Q_PROPERTY(uint showMyWebStatus READ showMyWebStatus
                                    WRITE setShowMyWebStatus
                                    NOTIFY showMyWebStatusChanged)
    Q_PROPERTY(uint showIHaveVideoTo READ showIHaveVideoTo
                                    WRITE setShowIHaveVideoTo
                                    NOTIFY showIHaveVideoToChanged)
    Q_PROPERTY(bool isOnline READ isOnline
                             NOTIFY onlineChanged)
    Q_PROPERTY(bool connectsAutomatically READ connectsAutomatically
                                          WRITE setConnectsAutomatically
                                          NOTIFY connectsAutomaticallyChanged)
    Q_PROPERTY(bool connectAfterSetup READ connectAfterSetup
                                      WRITE setConnectAfterSetup
                                      NOTIFY connectAfterSetupChanged)

public:
    explicit AccountHelper(QObject *parent = 0);

    Q_INVOKABLE void setAccountParameter(const QString &property, const QVariant &value);
    Q_INVOKABLE QVariant accountParameter(const QString &property) const;
    Q_INVOKABLE void unsetAccountParameter(const QString &property);

    Q_INVOKABLE void createAccount();

    QString connectionManager() const;
    void setConnectionManager(const QString &value);

    QString protocol() const;
    void setProtocol(const QString &value);

    QString displayName() const;
    void setDisplayName(const QString &value);

    QString password() const;
    void setPassword(const QString &value);

    QString icon() const;
    void setIcon(const QString &value);

    // the reading is just to get the URL path easily
    QString avatar() const;
    void setAvatar(const QString &value);

    bool saslAuth() const;
    void setSaslAuth(bool value);

    QObject *accountsModel() const;
    void setAccountsModel(QObject *value);

    uint allowTextChatFrom() const;
    void setAllowTextChatFrom(const uint &value);

    uint allowCallFrom() const;
    void setAllowCallFrom(const uint &value);

    uint allowOutsideCallFrom() const;
    void setAllowOutsideCallFrom(const uint &value);

    uint showMyAvatarTo() const;
    void setShowMyAvatarTo(const uint &value);

    uint showMyWebStatus() const;
    void setShowMyWebStatus(const uint &value);

    uint showIHaveVideoTo() const;
    void setShowIHaveVideoTo(const uint &value);

    Q_INVOKABLE void setAccount(QObject *account);
    Q_INVOKABLE void removeAccount();
    Q_INVOKABLE QString accountId() const;

    bool isOnline() const;

    bool connectsAutomatically() const;
    void setConnectsAutomatically(bool value);

    Q_INVOKABLE void introspectAccountPrivacySettings(const QString &interfaceName);
    Q_INVOKABLE void setPrivacySettings(const QString &interfaceName);
    bool connectAfterSetup() const;
    void setConnectAfterSetup(bool value);

Q_SIGNALS:
    void connectionManagerChanged();
    void protocolChanged();
    void displayNameChanged();
    void passwordChanged();
    void iconChanged();
    void avatarChanged();
    void saslAuthChanged();
    void accountsModelChanged();
    void accountSetupFinished();
    void allowTextChatFromChanged();
    void allowCallFromChanged();
    void allowOutsideCallFromChanged();
    void showMyAvatarToChanged();
    void showMyWebStatusChanged();
    void showIHaveVideoToChanged();
    void onlineChanged();
    void connectsAutomaticallyChanged();
    void accountChanged();
    void connectAfterSetupChanged();

protected:
    void updatePrivacySettings();

private:
    QString mConnectionManager;
    QString mProtocol;
    QString mDisplayName;
    QString mPassword;
    QString mIcon;
    QString mAvatar;
    bool mSaslAuth;
    IMAccountsModel *mAccountsModel;
    QVariantMap mParameters;
    QStringList mUnsetParameters;
    Tp::AccountManagerPtr mAccountManager;
    Tp::AccountPtr mAccount;
    uint mAllowTextChatFrom;
    uint mAllowCallFrom;
    uint mAllowOutsideCallFrom;
    uint mShowMyAvatarTo;
    uint mShowMyWebStatus;
    uint mShowIHaveVideoTo;
    bool mConnectsAutomatically;
    bool mConnectAfterSetup;

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onAccountCreated(Tp::PendingOperation *op);
    void onAccountEnabled(Tp::PendingOperation *op);
    void onPresenceChanged(Tp::PendingOperation *op);
    void onConnectionStatusChanged(Tp::ConnectionStatus status,
                                   Tp::ConnectionStatusReason statusReason);
    void onParametersUpdated(Tp::PendingOperation *op);
    void onPrivacySettingsLoaded(const QString &accountId);

    void onGotAllProperties(QDBusPendingCallWatcher *watcher);
    void onSetPrivacyProperty(QDBusPendingCallWatcher *watcher);
};

#endif // ACCOUNTHELPER_H
