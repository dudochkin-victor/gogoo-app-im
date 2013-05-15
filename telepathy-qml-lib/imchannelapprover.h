/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMCHANNELAPPROVER_H
#define IMCHANNELAPPROVER_H

#include <TelepathyQt4/AbstractClientApprover>
#include <TelepathyQt4/ChannelDispatchOperation>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4Yell/CallChannel>

class IMChannelApprover : public QObject, public Tp::AbstractClientApprover
{
    Q_OBJECT
public:
    IMChannelApprover();
    ~IMChannelApprover();

    void addDispatchOperation(const Tp::MethodInvocationContextPtr<> &context,
                              const Tp::ChannelDispatchOperationPtr &dispatchOperation);
    Tp::ChannelClassSpecList channelFilters() const;

Q_SIGNALS:
    void addedDispatchOperation(const Tp::ChannelDispatchOperationPtr dispatchOperation);
    void textChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel);
    void callChannelAvailable(const QString &accountId, Tpy::CallChannelPtr channel);
    void fileTransferChannelAvailable(const QString &accountId, Tp::IncomingFileTransferChannelPtr channel);
    void incomingCall(const QString &accountId, const QString &contactId, const QString &operationPath);
    void invalidated(void);

public Q_SLOTS:
    void onCloseOperation(QString operationObjectPath);
    void setApplicationRunning(bool running);

private Q_SLOTS:
    void onCallChannelReady(Tp::PendingOperation *op);
    void onStreamChannelReady(Tp::PendingOperation *op);
    void onTextChannelReady(Tp::PendingOperation *op);
    void onFileTransferChannelReady(Tp::PendingOperation *op);
    void onInvalidated(void);

private:
    QList<Tp::ChannelDispatchOperationPtr> mDispatchOps;
    bool mApplicationRunning;
};

#endif // IMCHANNELAPPROVER_H
