/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "filetransferagent.h"
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/PendingChannelRequest>
#include <QDebug>

FileTransferAgent::FileTransferAgent(const Tp::AccountPtr &account, const Tp::ContactPtr &contact, QObject *parent)
    : QObject(parent),
      mAccount(account),
      mContact(contact),
      mModel(0)
{
    qDebug() << "FileTransferAgent::FileTransferAgent: created for contact " << mContact->id();

    // todo check contact readyness
}

FileTransferAgent::~FileTransferAgent()
{
    qDebug() << "FileTransferAgent::~FileTransferAgent: destroyed for contact " << mContact->id();

    // todo deallocate whatever is needed
}

void FileTransferAgent::onIncomingTransferAvailable(Tp::IncomingFileTransferChannelPtr channel)
{
    qDebug() << "FileTransferAgent::onTextChannelAvailable: handling channel";

    if (channel.isNull()) {
        emit error(tr("Channel not available"));
        // TODO: what to do now?
        return;
    }

    // at this point the channel is already ready with the features we need
    // so no need to become ready again

    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SIGNAL(activeTransfersChanged()));

    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SIGNAL(pendingTransfersChanged()));
    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SLOT(onChannelStateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));

    // TODO: ask user for confirmation and where to save
    IncomingTransfer transfer;
    transfer.channel = channel;
    transfer.file = 0;
    transfer.channel->setProperty("fileName", transfer.channel->fileName());

    mIncomingTransfers.append(transfer);
    emit pendingTransfersChanged();

    if (mModel) {
        mModel->notifyFileTransfer(mContact, this, channel);
    }
}

void FileTransferAgent::onOutgoingTransferAvailable(Tp::OutgoingFileTransferChannelPtr channel,
                                                    Tp::ChannelRequestPtr request)
{
    qDebug() << "FileTransferAgent::onOutgoingTransferAvailable: handling channel";

    if (channel.isNull()) {
        emit error(tr("Channel not available"));
        // TODO: what to do now?
        return;
    }

    // at this point the channel is already ready with the features we need
    // so no need to become ready again
    bool found = false;
    foreach (Tp::ChannelRequestPtr req, mPendingRequests) {
        if (req->objectPath() == request->objectPath()) {
            request = req;
            mPendingRequests.removeAll(request);
            found = true;
            break;
        }
    }

    if (!found) {
        return;
    }

    OutgoingTransfer transfer;
    transfer.channel = channel;
    transfer.file = new QFile(request->property("filePath").toString());

    channel->setProperty("filePath", request->property("filePath"));
    channel->setProperty("fileName", request->property("fileName"));

    if (!transfer.file->exists() || !transfer.file->open(QIODevice::ReadOnly)) {
        delete transfer.file;
        return;
    }

    transfer.channel->provideFile(transfer.file);
    mOutgoingTransfers.append(transfer);

    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SIGNAL(activeTransfersChanged()));

    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SIGNAL(pendingTransfersChanged()));
    connect(channel.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SLOT(onChannelStateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));

    if (mModel) {
        mModel->notifyFileTransfer(mContact, this, channel);
    }
}

void FileTransferAgent::onChannelInvalidated(Tp::DBusProxy *, const QString &errorName, const QString &errorMessage)
{
    qDebug() << "FileTransferAgent::onChannelInvalidated: channel became invalid:" <<
        errorName << "-" << errorMessage;

    emit error(tr("Invalidated file transfer channel for contact %1 - %2 - %3")
               .arg(mContact->id())
               .arg(errorName)
               .arg(errorMessage));

}

void FileTransferAgent::onChannelStateChanged(Tp::FileTransferState state, Tp::FileTransferStateChangeReason /*reason*/)
{
    if (state == Tp::FileTransferStateCancelled ||
        state == Tp::FileTransferStateCompleted) {
        Tp::FileTransferChannelPtr channel(qobject_cast<Tp::FileTransferChannel*>(sender()));
        Tp::IncomingFileTransferChannelPtr incomingChannel;
        incomingChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(channel);
        Tp::OutgoingFileTransferChannelPtr outgoingChannel;
        outgoingChannel = Tp::OutgoingFileTransferChannelPtr::dynamicCast(channel);

        if (incomingChannel) {
            QList<IncomingTransfer>::iterator it = mIncomingTransfers.begin();
            for ( ; it != mIncomingTransfers.end(); ++it) {
                if ((*it).channel == incomingChannel) {
                    if ((*it).file) {
                        if ((*it).file->isOpen()) {
                            (*it).file->close();
                        }
                        delete (*it).file;
                        (*it).file = 0;
                    }
                }
            }
        }

        if (outgoingChannel) {
            QList<OutgoingTransfer>::iterator it = mOutgoingTransfers.begin();
            for ( ; it != mOutgoingTransfers.end(); ++it) {
                if ((*it).channel == outgoingChannel) {
                    if ((*it).file) {
                        if ((*it).file->isOpen()) {
                            (*it).file->close();
                        }
                        delete (*it).file;
                        (*it).file = 0;
                    }
                }
            }
        }
    }
}

int FileTransferAgent::activeTransfers() const
{
    int count = 0;

    foreach (const IncomingTransfer &transfer, mIncomingTransfers) {
        if (transfer.channel->state() == Tp::FileTransferStateAccepted ||
            transfer.channel->state() == Tp::FileTransferStateOpen) {
            count++;
        }
    }

    foreach (const OutgoingTransfer &transfer, mOutgoingTransfers) {
        if (transfer.channel->state() == Tp::FileTransferStateAccepted ||
            transfer.channel->state() == Tp::FileTransferStateOpen) {
            count++;
        }
    }
    return count;
}

int FileTransferAgent::pendingTransfers() const
{
    int count = 0;

    foreach (const IncomingTransfer &transfer, mIncomingTransfers) {
        if (transfer.channel->state() == Tp::FileTransferStatePending) {
            count++;
        }
    }

    foreach (const OutgoingTransfer &transfer, mOutgoingTransfers) {
        if (transfer.channel->state() == Tp::FileTransferStatePending) {
            count++;
        }
    }

    return count;
}

QString FileTransferAgent::lastPendingTransfer() const
{
    for (int i=mIncomingTransfers.count()-1; i >=0; --i) {
        if (mIncomingTransfers[i].channel->state() == Tp::FileTransferStatePending) {
            return mIncomingTransfers[i].channel->fileName();
        }
    }

    return QString();
}

QDateTime FileTransferAgent::lastPendingTransferDateTime() const
{
    // TODO: check how to implement that
    return QDateTime::currentDateTime();
}

void FileTransferAgent::setModel(QObject *model)
{
    IMConversationModel *conversationModel = qobject_cast<IMConversationModel*>(model);
    Q_ASSERT(conversationModel);

    // if we were already using this model, there is nothing to setup
    if (conversationModel == mModel) {
        return;
    }

    mModel = conversationModel;

    // append any available file transfers
    foreach(const IncomingTransfer &transfer, mIncomingTransfers) {
        mModel->notifyFileTransfer(mContact, this, transfer.channel);
    }

    foreach(const OutgoingTransfer &transfer, mOutgoingTransfers) {
        mModel->notifyFileTransfer(mContact, this, transfer.channel);
    }
}

void FileTransferAgent::sendFile(const QString &filePath)
{
    QFile file(filePath);

    if (!file.exists()) {
        // TODO: report
        return;
    }

    QFileInfo fileInfo(filePath);
    qDebug() << "Going to send file " << filePath;
    Tp::FileTransferChannelCreationProperties properties(fileInfo.fileName(),
                                                         "",
                                                         file.size());

    Tp::PendingChannelRequest *pendingRequest = mAccount->createFileTransfer(mContact, properties);
    pendingRequest->setProperty("filePath", filePath);
    pendingRequest->setProperty("fileName", fileInfo.fileName());

    connect(pendingRequest,
            SIGNAL(channelRequestCreated(Tp::ChannelRequestPtr)),
            SLOT(onChannelRequestCreated(Tp::ChannelRequestPtr)));
}

void FileTransferAgent::acceptTransfer(Tp::IncomingFileTransferChannelPtr channel)
{
    QList<IncomingTransfer>::iterator it = mIncomingTransfers.begin();
    while (it != mIncomingTransfers.end()) {
        IncomingTransfer &transfer = *it;
        if (transfer.channel == channel) {
            // TODO: check how to get the internationalized name of the directory
            QDir dir(QDir::homePath() + "/Downloads/");
            if (!dir.exists()) {
                dir.mkpath(dir.absolutePath());
            }
            QFileInfo info(dir.absoluteFilePath(channel->property("fileName").toString()));
            QString baseName = info.baseName();
            QString suffix = info.completeSuffix();

            // get a unique filename
            int count = 1;
            while (info.exists()) {
                info.setFile(dir, baseName + QString("_%1.").arg(count) + suffix);
                count++;
            }

            qDebug() << "Unique name:" << info.absoluteFilePath();
            channel->setProperty("fileName", info.fileName());
            channel->setProperty("filePath", info.absoluteFilePath());

            transfer.file = new QFile(info.absoluteFilePath());
            if (transfer.file->exists()) {
                transfer.file->remove();
            }

            transfer.file->open(QIODevice::WriteOnly);
            channel->acceptFile(0, transfer.file);
            break;
        }
        ++it;
    }
}

void FileTransferAgent::cancelTransfer(Tp::FileTransferChannelPtr channel)
{
    Tp::IncomingFileTransferChannelPtr incomingChannel;
    incomingChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(channel);

    Tp::OutgoingFileTransferChannelPtr outgoingChannel;
    outgoingChannel = Tp::OutgoingFileTransferChannelPtr::dynamicCast(channel);

    if (!incomingChannel.isNull()) {
        QList<IncomingTransfer>::iterator it = mIncomingTransfers.begin();
        while (it != mIncomingTransfers.end()) {
            IncomingTransfer &transfer = *it;
            if (transfer.channel == incomingChannel) {
                channel->cancel();
                break;
            }
            ++it;
        }
    }

    if (!outgoingChannel.isNull()) {
        QList<OutgoingTransfer>::iterator it = mOutgoingTransfers.begin();
        while (it != mOutgoingTransfers.end()) {
            OutgoingTransfer &transfer = *it;
            if (transfer.channel == outgoingChannel) {
                channel->cancel();
                break;
            }
            ++it;
        }
    }
}

Tp::AccountPtr FileTransferAgent::account() const
{
    return mAccount;
}

Tp::ContactPtr FileTransferAgent::contact() const
{
    return mContact;
}

void FileTransferAgent::onChannelRequestCreated(Tp::ChannelRequestPtr request)
{
    qDebug() << "FileTransferAgent::onChannelRequestCreated";
    Tp::PendingChannelRequest *pch = qobject_cast<Tp::PendingChannelRequest*>(sender());
    if (!pch) {
        // TODO: report error
        return;
    }

    request->setProperty("filePath", pch->property("filePath"));
    request->setProperty("fileName", pch->property("fileName"));
    qDebug() << "Request: " << request;
    mPendingRequests.append(request);
}
