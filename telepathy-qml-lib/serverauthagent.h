#ifndef SERVERAUTHAGENT_H
#define SERVERAUTHAGENT_H

#include <QObject>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/ChannelInterfaceSASLAuthenticationInterface>
#include <TelepathyQt4Yell/ChannelInterfaceCredentialsStorageInterface>

class ServerAuthAgent : public QObject
{
    Q_OBJECT
public:
    explicit ServerAuthAgent(Tp::AccountPtr account, QObject *parent = 0);

    void setChannel(Tp::ChannelPtr channel);
    void startAuth();
    void setPassword(const QString &password);

signals:
    void passwordRequestRequired(const QString &accountId);

private slots:
    void onAvailableSASLMechanisms(Tp::PendingOperation *op);
    void onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details);

private:
    Tp::AccountPtr mAccount;
    Tp::ChannelPtr mChannel;
    Tp::Client::ChannelInterfaceSASLAuthenticationInterface *mSaslInterface;
    Tpy::Client::ChannelInterfaceCredentialsStorageInterface *mCredentialsInterface;
    QString mPassword;
    bool mWaitingForPassword;
};

#endif // SERVERAUTHAGENT_H
