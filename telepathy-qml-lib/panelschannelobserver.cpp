/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "panelschannelobserver.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/PendingReady>

PanelsChannelObserver::PanelsChannelObserver(const Tp::ChannelClassSpecList &channelFilter, bool shouldRecover)
    : AbstractClientObserver(channelFilter, shouldRecover)
{

}

PanelsChannelObserver::~PanelsChannelObserver()
{
}

void PanelsChannelObserver::observeChannels(const Tp::MethodInvocationContextPtr<> &context,
                                            const Tp::AccountPtr &account,
                                            const Tp::ConnectionPtr &connection,
                                            const QList<Tp::ChannelPtr> &channels,
                                            const Tp::ChannelDispatchOperationPtr &dispatchOperation,
                                            const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                            const ObserverInfo &observerInfo)
{
    Q_UNUSED(connection);
    Q_UNUSED(dispatchOperation);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(observerInfo);
    qDebug("PanelsChannelObserver::observeChannels(): New channel to handle");

    foreach (Tp::ChannelPtr channel, channels) {
        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);
        if (!textChannel.isNull()) {
            mTextChannels.append(textChannel);
            emit newTextChannel(account, textChannel);
            continue;
        }


        Tpy::CallChannelPtr callChannel = Tpy::CallChannelPtr::dynamicCast(channel);
        if (!callChannel.isNull()) {
            qDebug("PanelsChannelObserver::observeChannels(): New Call Channel detected");
            mCallChannels.append(callChannel);
            emit newCallChannel(account, callChannel);
            continue;
        }

        Tp::IncomingFileTransferChannelPtr fileTransferChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(channel);
        if (!fileTransferChannel.isNull()) {
            mFileTransferChannels.append(fileTransferChannel);
            emit newFileTransferChannel(account, fileTransferChannel);
            continue;
        }
    }

    context->setFinished();
}
