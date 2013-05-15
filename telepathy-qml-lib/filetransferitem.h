/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FILETRANSFERITEM_H
#define FILETRANSFERITEM_H

#include <TelepathyQt4Yell/Models/EventItem>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/Contact>

#include "filetransferagent.h"

class FileTransferItem : public Tpy::EventItem
{
    Q_OBJECT
    Q_PROPERTY(bool incomingTransfer READ incomingTransfer)
    Q_PROPERTY(QString fileName READ fileName)
    Q_PROPERTY(QString filePath READ filePath)
    Q_PROPERTY(float percentTransferred READ percentTransferred)
public:
    explicit FileTransferItem(Tp::ContactPtr sender,
                              Tp::ContactPtr receiver,
                              FileTransferAgent *agent,
                              Tp::FileTransferChannelPtr channel,
                              QObject *parent = 0);

    bool incomingTransfer() const;

    QString fileName() const;
    QString filePath() const;
    qulonglong fileSize() const;
    int transferState() const;
    int transferStateReason() const;
    int percentTransferred() const;

    Q_INVOKABLE void acceptTransfer();
    // this one acts as a cancel and reject operations
    Q_INVOKABLE void cancelTransfer();


Q_SIGNALS:
    void itemChanged();

private:
    bool mIncoming;
    Tp::IncomingFileTransferChannelPtr mIncomingChannel;
    Tp::OutgoingFileTransferChannelPtr mOutgoingChannel;
    Tp::FileTransferChannelPtr mChannel;
    FileTransferAgent *mAgent;
};

#endif // FILETRANSFERITEM_H
