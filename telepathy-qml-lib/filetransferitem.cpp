/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "filetransferitem.h"
#include <QDateTime>

FileTransferItem::FileTransferItem(Tp::ContactPtr sender,
                                   Tp::ContactPtr receiver,
                                   FileTransferAgent *agent,
                                   Tp::FileTransferChannelPtr channel,
                                   QObject *parent)
: Tpy::EventItem(sender, receiver, QDateTime::currentDateTime(), parent),
  mChannel(channel), mAgent(agent)
{
    mIncomingChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(channel);
    mIncoming = !mIncomingChannel.isNull();

    if (!mIncoming) {
        mOutgoingChannel = Tp::OutgoingFileTransferChannelPtr::dynamicCast(channel);
    }

    connect(mChannel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SIGNAL(itemChanged()));
    connect(mChannel.data(),
            SIGNAL(transferredBytesChanged(qulonglong)),
            SIGNAL(itemChanged()));
}

bool FileTransferItem::incomingTransfer() const
{
    return mIncoming;
}

QString FileTransferItem::fileName() const
{
    return mChannel->property("fileName").toString();
}

QString FileTransferItem::filePath() const
{
    return mChannel->property("filePath").toString();
}

qulonglong FileTransferItem::fileSize() const
{
    return mChannel->size();
}

int FileTransferItem::transferState() const
{
    return mChannel->state();
}

int FileTransferItem::transferStateReason() const
{
    return mChannel->stateReason();
}

int FileTransferItem::percentTransferred() const
{
    if (mChannel->size() == 0) {
        return 100;
    }

    return (mChannel->transferredBytes() * 100 / mChannel->size());
}

void FileTransferItem::acceptTransfer()
{
    if (!mIncoming) {
        return;
    }

    mAgent->acceptTransfer(mIncomingChannel);
}

void FileTransferItem::cancelTransfer()
{
    mAgent->cancelTransfer(mChannel);
}
