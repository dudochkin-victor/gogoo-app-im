#include "serverauthagent.h"
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/AccountManager>

ServerAuthAgent::ServerAuthAgent(Tp::AccountPtr account, QObject *parent)
    : QObject(parent), mAccount(account), mWaitingForPassword(false)
{
    qDebug() << "Server auth agent created for account" << account->uniqueIdentifier();
}

void ServerAuthAgent::setChannel(Tp::ChannelPtr channel)
{
    mChannel = channel;
    mSaslInterface = mChannel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>();

    if (mChannel->hasInterface(mCredentialsInterface->staticInterfaceName())) {
        mCredentialsInterface = mChannel->interface<Tpy::Client::ChannelInterfaceCredentialsStorageInterface>();
        mCredentialsInterface->StoreCredentials(true);
    }

    connect(mSaslInterface,
            SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)),
            SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));

}

void ServerAuthAgent::setPassword(const QString &password)
{
    mPassword = password;
    if (mWaitingForPassword && !mPassword.isNull()) {
         mSaslInterface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), mPassword.toUtf8());
    }

    mWaitingForPassword = false;
}

void ServerAuthAgent::startAuth()
{
    connect(mSaslInterface->requestPropertyAvailableMechanisms(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAvailableSASLMechanisms(Tp::PendingOperation*)));
}

void ServerAuthAgent::onAvailableSASLMechanisms(Tp::PendingOperation *op)
{
    if (op->isError()) {
        // TODO: handle errors
        return;
    }

    Tp::PendingVariant *pv = qobject_cast<Tp::PendingVariant*>(op);
    QStringList mechanisms = qdbus_cast<QStringList>(pv->result());
    if (!mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        qDebug() << "ServerAuthAgent::onAvailableSASLMechanisms: no supported mechanism found";
        return;
    }

    if (mPassword.isNull()) {
        mWaitingForPassword = true;
        emit passwordRequestRequired(mAccount->uniqueIdentifier());
        return;
    }

    mSaslInterface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), mPassword.toUtf8());
}

void ServerAuthAgent::onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details)
{
    qDebug() << "ServerAuthAgent::onSASLStatusChanged:";
    qDebug() << "status:" << status << "reason:" << reason
             << "details:" << details;

    switch (status) {
    case Tp::SASLStatusServerSucceeded:
        mSaslInterface->AcceptSASL();
        break;
    case Tp::SASLStatusSucceeded:
        qDebug() << "Finished!";
        // TODO: report we are finished
        break;
    case Tp::SASLStatusInProgress:
        qDebug() << "Proceeding with the authentication";
        break;
    case Tp::SASLStatusServerFailed:
        qDebug() << "Error authenticating!";
        mChannel->requestClose();
        //TODO: notify the error
        break;
    }
}
