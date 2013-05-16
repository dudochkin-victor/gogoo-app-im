/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CHANNELHANDLER_H
#define CHANNELHANDLER_H

#include <QObject>
#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/AbstractClientApprover>
#include <TelepathyQt/Types>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Account>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt4Yell/CallChannel>

class ChannelHandler : public QObject,
        public Tp::AbstractClientHandler
{
    Q_OBJECT
public:
    explicit ChannelHandler(QObject *parent = 0);
    virtual ~ChannelHandler();

    virtual bool bypassApproval() const;

    virtual void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                                const Tp::AccountPtr &account,
                                const Tp::ConnectionPtr &connection,
                                const QList<Tp::ChannelPtr> &channels,
                                const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                const QDateTime &userActionTime,
                                const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

    Tp::ChannelClassSpecList channelFilters();
    Tp::AbstractClientHandler::Capabilities channelCapabilites();

signals:
    void textChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel);
    void callChannelAvailable(const QString &accountId, Tpy::CallChannelPtr channel);
    void incomingFileTransferChannelAvailable(const QString &accountId, Tp::IncomingFileTransferChannelPtr channel);
    void outgoingFileTransferChannelAvailable(const QString &accountId,
                                              Tp::OutgoingFileTransferChannelPtr channel,
                                              Tp::ChannelRequestPtr request);
    void serverAuthChannelAvailable(const QString &accountId, Tp::ChannelPtr channel);

private slots:
    void onCallChannelReady(Tp::PendingOperation *op);
    void onStreamChannelReady(Tp::PendingOperation *op);
    void onTextChannelReady(Tp::PendingOperation *op);
    void onIncomingFileTransferChannelReady(Tp::PendingOperation *op);
    void onOutgoingFileTransferChannelReady(Tp::PendingOperation *op);

private:
    QMap<Tp::OutgoingFileTransferChannel*, Tp::ChannelRequestPtr> mOutgoingRequests;
};

#endif // CHANNELHANDLER_H
