/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMTEXTCHANNELOBSERVER_H
#define IMTEXTCHANNELOBSERVER_H

#include <TelepathyQt4/AbstractClientObserver>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/Types>

#include <TelepathyQt4Yell/CallChannel>


#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

class PanelsChannelObserver : public QObject, public Tp::AbstractClientObserver
{
    Q_OBJECT
public:
    PanelsChannelObserver(const Tp::ChannelClassSpecList &channelFilter, bool shouldRecover = false);
    ~PanelsChannelObserver();

    void observeChannels(const Tp::MethodInvocationContextPtr<> &context,
                    const Tp::AccountPtr &account,
                    const Tp::ConnectionPtr &connection,
                    const QList<Tp::ChannelPtr> &channels,
                    const Tp::ChannelDispatchOperationPtr &dispatchOperation,
                    const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                    const Tp::AbstractClientObserver::ObserverInfo &observerInfo);

Q_SIGNALS:
    void newTextChannel(const Tp::AccountPtr &account, const Tp::TextChannelPtr &channel);
    void newCallChannel(const Tp::AccountPtr &account, const Tpy::CallChannelPtr &channel);
    void newFileTransferChannel(const Tp::AccountPtr &account, const Tp::IncomingFileTransferChannelPtr &channel);

public slots:
            void onTextChannelReady(Tp::PendingOperation *op);

private:
            QList<Tp::TextChannelPtr> mTextChannels;
            QList<Tpy::CallChannelPtr> mCallChannels;
            QList<Tp::IncomingFileTransferChannelPtr>  mFileTransferChannels;
};

#endif // PANELSCHANNELOBSERVER_H
