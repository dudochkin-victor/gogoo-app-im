/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMCONVERSATIONMODEL_H
#define IMCONVERSATIONMODEL_H

#include <TelepathyLoggerQt4/Models/LoggerConversationModel>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/Types>
#include <TelepathyQt4Yell/Models/SessionConversationModel>
#include <TelepathyQt4Yell/Models/EventItem>
#include "mergedmodel.h"
#include "callagent.h"

class FileTransferAgent;

class IMConversationModel : public MergedModel
{
    Q_OBJECT

    Q_PROPERTY(int numMatchesFound READ numMatchesFound NOTIFY numMatchesFoundChanged)
    Q_PROPERTY(bool olderActive READ olderActive NOTIFY olderActiveChanged)
    Q_PROPERTY(bool newerActive READ newerActive NOTIFY newerActiveChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(int currentRowMatch READ currentRowMatch NOTIFY currentRowMatchChanged)
    Q_PROPERTY(int numPendingMessages READ numPendingMessages NOTIFY numPendingMessagesChanged)

public:
    enum Role {
        StatusRole = Qt::UserRole + 100,
        DateStringRole,
        // true if the line comes from telepathy-logger
        FromLoggerRole,
        BubbleColorRole,
        IncomingEventRole,

        // file transfer related roles
        IncomingTransferRole,
        FileNameRole,
        FilePathRole,
        FileSizeRole,
        TransferStateRole,
        TransferStateReasonRole,
        PercentTransferredRole
    };

    explicit IMConversationModel(const Tp::AccountPtr &account,
        const Tp::ContactPtr &self,
        const Tp::ContactPtr &contact,
        const Tp::TextChannelPtr &channel,
        QObject *parent = 0);
    virtual ~IMConversationModel();

    virtual QVariant data(const QModelIndex &index, int role) const;
    QString searchString() const;
    void notifyCallStatusChanged(CallAgent *callAgent, CallAgent::CallStatus oldCallStatus, CallAgent::CallStatus newCallStatus);
    void notifyCallError(Tp::ContactPtr contact, const QString & errorString);
    void notifyFileTransfer(Tp::ContactPtr contact, FileTransferAgent *agent, Tp::FileTransferChannelPtr channel);

    static QString friendlyFileSize(qulonglong size);

    Q_INVOKABLE bool canFetchMoreBack() const;
    Q_INVOKABLE void fetchMoreBack();
    Q_INVOKABLE void clearLog();

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void disconnectChannelQueue();
    Q_INVOKABLE void connectChannelQueue();
    Q_INVOKABLE bool channelQueueConnected() const;

    int numMatchesFound() const;
    bool olderActive() const;
    bool newerActive() const;
    bool searching() const;
    int currentRowMatch() const;
    int numPendingMessages() const;

Q_SIGNALS:
    void backFetchable();
    void backFetched(int numItems);
    void numMatchesFoundChanged();
    void olderActiveChanged();
    void newerActiveChanged();
    void searchingChanged();
    void currentRowMatchChanged();
    void numPendingMessagesChanged();

public Q_SLOTS:
    void searchByString(const QString &search);
    void slotResetModel();
    void newerMatch();
    void olderMatch();

protected Q_SLOTS:
    virtual void onChatStateChanged(const Tp::ContactPtr &contact, Tp::ChannelChatState state);
    void onItemChanged();
    void calculateMatches();
    void continueSearch();
    void onBackFetched();

protected:
    QString contactColor(const QString &id) const;

private:
    QString mSearchString;
    QList<Tp::ContactPtr> mContactsTyping;
    QDateTime mContactsTypingTime;
    QList<Tpy::EventItem*> mChatRunningItems;
    Tpy::EventItem *mCallRunningItem;
    QStringList mContactsList;
    QStringList mBubbleColor;
    Tpl::LoggerConversationModel *mLoggerConversationModel;
    Tpy::SessionConversationModel *mSessionConversationModel;
    Tp::ContactPtr mSelf;
    Tp::AccountPtr mAccount;
    int mNumDuplicatedMessages;

    // matches are sorted reverse chronological order, 0 is newest
    int mCurrentMatch;
    int mCurrentRowMatch;
    QList<QModelIndex> mMatchesFound;
    bool mSearching;
};

#endif // IMCONVERSATIONMODEL_H
