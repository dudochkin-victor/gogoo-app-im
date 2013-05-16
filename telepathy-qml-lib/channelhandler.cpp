/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "channelhandler.h"

#include <TelepathyQt/TextChannel>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/Types>
#include <TelepathyQt/Constants>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/ChannelInterfaceSASLAuthenticationInterface>
#include <TelepathyQt4Yell/ChannelClassSpec>

ChannelHandler::ChannelHandler(QObject *parent) :
    QObject(parent),
    Tp::AbstractClientHandler(
        channelFilters(),
        channelCapabilites())
{
}

ChannelHandler::~ChannelHandler()
{
}

void ChannelHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                                    const Tp::AccountPtr &account,
                                    const Tp::ConnectionPtr &connection,
                                    const QList<Tp::ChannelPtr> &channels,
                                    const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                    const QDateTime &userActionTime,
                                    const Tp::AbstractClientHandler::HandlerInfo &handlerInfo)
{
    qDebug() << "ChannelHandler::handleChannels: "
             << " account=" << account->uniqueIdentifier()
             << " connection=" << connection.data()
             << " numChannels=" << channels.size();
             //<< " handlerInfo=" << handlerInfo;

    Q_UNUSED(connection);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    foreach (Tp::ChannelPtr channel, channels) {

        qDebug() << "ChannelHandler::handleChannels: checking channel=" << channel.data();

        if (channel->channelType() == TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION) {
            emit serverAuthChannelAvailable(account->uniqueIdentifier(), channel);
            continue;
        }

        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);
        if (!textChannel.isNull()) {
            // todo fix hack: stick a property to have accountId handy when we get the stream
            textChannel->setProperty("accountId", QVariant(account->uniqueIdentifier()));
            qDebug() << "ChannelHandler::handleChannels: handling text channel - becomeReady "
                     << " immutableProperties=" << textChannel->immutableProperties();
            connect(textChannel->becomeReady(Tp::Features()
                                             << Tp::TextChannel::FeatureCore
                                             << Tp::TextChannel::FeatureChatState
                                             << Tp::TextChannel::FeatureMessageCapabilities
                                             << Tp::TextChannel::FeatureMessageQueue
                                             << Tp::TextChannel::FeatureMessageSentSignal),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onTextChannelReady(Tp::PendingOperation*)));
            //emit textChannelAvailable(account, textChannel);
            continue;
        }

        Tpy::CallChannelPtr callChannel = Tpy::CallChannelPtr::dynamicCast(channel);
        if (!callChannel.isNull()) {
            // todo fix hack: stick a property to have accountId handy when we get the stream
            callChannel->setProperty("accountId", QVariant(account->uniqueIdentifier()));
            qDebug() << "ChannelHandler::handleChannels: handling call channel - becomeReady"
                     << " immutableProperties=" << callChannel->immutableProperties();
            connect(callChannel->becomeReady(Tp::Features()
                                             << Tpy::CallChannel::FeatureCore
                                             << Tpy::CallChannel::FeatureContents),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onCallChannelReady(Tp::PendingOperation*)));
            continue;
        }

        Tp::IncomingFileTransferChannelPtr incomingFileTransferChannel = Tp::IncomingFileTransferChannelPtr::dynamicCast(channel);
        if (!incomingFileTransferChannel.isNull()) {
            // todo fix hack: stick a property to have accountId handy when we get the stream
            incomingFileTransferChannel->setProperty("accountId", QVariant(account->uniqueIdentifier()));
            qDebug() << "ChannelHandler::handleChannels: handling incoming file transfer channel - becomeReady"
                     << " immutableProperties=" << incomingFileTransferChannel->immutableProperties();
            connect(incomingFileTransferChannel->becomeReady(Tp::Features()
                                                             << Tp::IncomingFileTransferChannel::FeatureCore),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onIncomingFileTransferChannelReady(Tp::PendingOperation*)));
            continue;
        }

        Tp::OutgoingFileTransferChannelPtr outgoingFileTransferChannel = Tp::OutgoingFileTransferChannelPtr::dynamicCast(channel);
        if (!outgoingFileTransferChannel.isNull()) {
            // todo fix hack: stick a property to have accountId handy when we get the stream
            outgoingFileTransferChannel->setProperty("accountId", QVariant(account->uniqueIdentifier()));
            qDebug() << "ChannelHandler::handleChannels: handling outgoing file transfer channel - becomeReady"
                     << " immutableProperties=" << outgoingFileTransferChannel->immutableProperties();
            connect(outgoingFileTransferChannel->becomeReady(Tp::Features()
                                                             << Tp::OutgoingFileTransferChannel::FeatureCore),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onOutgoingFileTransferChannelReady(Tp::PendingOperation*)));
            if (requestsSatisfied.count()) {
                mOutgoingRequests[outgoingFileTransferChannel.data()] = requestsSatisfied.first();
            }

            continue;
        }

        // reached here without handling the channel, bit weird
        qDebug() << "ChannelHandler::handleChannels: unhandled channel " << "channelType=" << channel->channelType()
                    << "objectName=" << channel->objectName() << "objectPath=" << channel->objectPath();
    }

    context->setFinished();
}

bool ChannelHandler::bypassApproval() const
{
    return false;
}

Tp::AbstractClientHandler::Capabilities ChannelHandler::channelCapabilites()
{
    Tp::AbstractClientHandler::Capabilities capabilities;

    capabilities.setGTalkP2PNATTraversalToken();
    capabilities.setICEUDPNATTraversalToken();
    capabilities.setAudioCodecToken("speex");
    capabilities.setVideoCodecToken("theora");
    capabilities.setVideoCodecToken("h264");

    return capabilities;
}

Tp::ChannelClassSpecList ChannelHandler::channelFilters()
{
    Tp::ChannelClassSpecList specList;
    specList << Tp::ChannelClassSpec::textChat();
    specList << Tp::ChannelClassSpec::textChatroom();
    specList << Tpy::ChannelClassSpec::mediaCall();
    specList << Tpy::ChannelClassSpec::audioCall();
    specList << Tpy::ChannelClassSpec::videoCall();
    specList << Tpy::ChannelClassSpec::videoCallWithAudio();
    specList << Tp::ChannelClassSpec::incomingFileTransfer();
    specList << Tp::ChannelClassSpec::outgoingFileTransfer();

    QMap<QString, QDBusVariant> filter;
    filter.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".ChannelType"),
                  QDBusVariant(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION));
    filter.insert(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + QString::fromLatin1(".AuthenticationMethod"),
                  QDBusVariant(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION));
    filter.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".TargetHandleType"),
                  QDBusVariant(Tp::HandleTypeNone));
    specList << Tp::ChannelClassSpec(Tp::ChannelClass(filter));

    qDebug() << "Appending filter for type" << TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION;
    qDebug() << TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + ".AuthenticationMethod = " << TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION;
    return specList;
}

void ChannelHandler::onCallChannelReady(Tp::PendingOperation *op)
{
    qDebug() << "ChannelHandler::onCallChannelReady: channel ready";

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "ChannelHandler::onCallChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

    Tpy::CallChannelPtr callChannel = /*Tpy::CallChannelPtr::dynamicCast(pr->object());*/
            Tpy::CallChannelPtr(qobject_cast<Tpy::CallChannel *>(pr));
    if (callChannel.isNull()) {
        qDebug() << "ChannelHandler::onCallChannelReady: call channel invalid";
        return;
    }
    QString accountId = callChannel->property("accountId").toString();

    qDebug() << "ChannelHandler::onCallChannelReady: channel=" << callChannel.data();

    emit callChannelAvailable(accountId, callChannel);
}

void ChannelHandler::onTextChannelReady(Tp::PendingOperation *op)
{
    qDebug() << "ChannelHandler::onTextChannelReady: channel ready";

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "ChannelHandler::onTextChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

    Tp::TextChannelPtr textChannel = /*Tp::TextChannelPtr::dynamicCast(pr->object());*/
            Tp::TextChannelPtr(qobject_cast<Tp::TextChannel *>(pr));
    if (textChannel.isNull()) {
        qDebug() << "ChannelHandler::onTextChannelReady: stream invalid";
        return;
    }
    QString accountId = textChannel->property("accountId").toString();

    emit textChannelAvailable(accountId, textChannel);
}

void ChannelHandler::onIncomingFileTransferChannelReady(Tp::PendingOperation *op)
{
    qDebug() << "ChannelHandler::onIncomingFileTransferChannelReady: channel ready";

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "ChannelHandler::onFileTransferChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

    Tp::IncomingFileTransferChannelPtr fileTransferChannel = /*Tp::IncomingFileTransferChannelPtr::dynamicCast(pr->object());*/
            Tp::IncomingFileTransferChannelPtr(qobject_cast<Tp::IncomingFileTransferChannel *>(pr));
    if (fileTransferChannel.isNull()) {
        qDebug() << "ChannelHandler::onIncomingFileTransferChannelReady: channel invalid";
        return;
    }
    QString accountId = fileTransferChannel->property("accountId").toString();

    qDebug() << "File name:" << fileTransferChannel->fileName();

    emit incomingFileTransferChannelAvailable(accountId, fileTransferChannel);
}

void ChannelHandler::onOutgoingFileTransferChannelReady(Tp::PendingOperation *op)
{
    qDebug() << "ChannelHandler::onOutgoingFileTransferChannelReady: channel ready";

    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    if (!pr || pr->isError()) {
        qDebug() << "ChannelHandler::onFileTransferChannelReady: error "
                 << (op ? op->errorName() : "")
                 << (op ? op->errorMessage() : "");
        return;
    }

    Tp::OutgoingFileTransferChannelPtr fileTransferChannel = /*Tp::OutgoingFileTransferChannelPtr::dynamicCast(pr->object());*/
             Tp::OutgoingFileTransferChannelPtr(qobject_cast<Tp::OutgoingFileTransferChannel *>(pr));
    if (fileTransferChannel.isNull()) {
        qDebug() << "ChannelHandler::onOutgoingFileTransferChannelReady: channel invalid";
        return;
    }

    Tp::ChannelRequestPtr request;
    if (mOutgoingRequests.contains(fileTransferChannel.data())) {
        request = mOutgoingRequests.take(fileTransferChannel.data());
    }
    QString accountId = fileTransferChannel->property("accountId").toString();
    emit outgoingFileTransferChannelAvailable(accountId, fileTransferChannel, request);
}
