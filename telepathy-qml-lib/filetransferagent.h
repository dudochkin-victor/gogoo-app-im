/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FILETRANSFERAGENT_H
#define FILETRANSFERAGENT_H

#include "imconversationmodel.h"

#include <TelepathyQt/Types>
#include <TelepathyQt/Account>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/ChannelRequest>

namespace Tp {
    class PendingMediaStreams;
}

class IncomingTransfer
{
public:
    QFile *file;
    Tp::IncomingFileTransferChannelPtr channel;
};

class OutgoingTransfer
{
public:
    QFile *file;
    Tp::OutgoingFileTransferChannelPtr channel;
};

class FileTransferAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeTransfers READ activeTransfers NOTIFY activeTransfersChanged)
    Q_PROPERTY(int pendingTransfers READ pendingTransfers NOTIFY pendingTransfersChanged)
public:
    explicit FileTransferAgent(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, QObject *parent);
    virtual ~FileTransferAgent();

    Tp::AccountPtr account() const;
    Tp::ContactPtr contact() const;

    int activeTransfers() const;
    int pendingTransfers() const;

    Q_INVOKABLE void setModel(QObject *model);
    Q_INVOKABLE void sendFile(const QString &fileName);

    QString lastPendingTransfer() const;
    QDateTime lastPendingTransferDateTime() const;

    void acceptTransfer(Tp::IncomingFileTransferChannelPtr channel);
    void cancelTransfer(Tp::FileTransferChannelPtr channel);

Q_SIGNALS:
    void error(const QString &msg);

    // property notification signals
    void activeTransfersChanged();
    void pendingTransfersChanged();

public Q_SLOTS:
    void onIncomingTransferAvailable(Tp::IncomingFileTransferChannelPtr channel);
    void onOutgoingTransferAvailable(Tp::OutgoingFileTransferChannelPtr channel, Tp::ChannelRequestPtr request);

protected:
    void createModelForChat(void);

private Q_SLOTS:
    void onChannelRequestCreated(Tp::ChannelRequestPtr request);
    void onChannelReady(Tp::PendingOperation *op);
    void onChannelInvalidated(Tp::DBusProxy *, const QString &errorName, const QString &errorMessage);
    void onChannelStateChanged(Tp::FileTransferState state, Tp::FileTransferStateChangeReason reason);

private:
    Tp::AccountPtr mAccount;
    Tp::ContactPtr mContact;
    QList<IncomingTransfer> mIncomingTransfers;
    QList<OutgoingTransfer> mOutgoingTransfers;
    IMConversationModel* mModel;
    QList<Tp::ChannelRequestPtr> mPendingRequests;
};

#endif // FileTransferAgent_H
