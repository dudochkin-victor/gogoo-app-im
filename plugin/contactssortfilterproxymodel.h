/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CONTACTSSORTFILTERPROXYMODEL_H
#define CONTACTSSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "../telepathy-qml-lib/telepathymanager.h"

class ContactsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(QString accountId READ accountId NOTIFY accountIdChanged)

public:
    ContactsSortFilterProxyModel(TelepathyManager *manager, QAbstractItemModel *model, QObject *parent = 0);
    ~ContactsSortFilterProxyModel();

    Q_INVOKABLE bool  haveConnection() const;
    Q_INVOKABLE QString serviceName(void) const;
    Q_INVOKABLE void setShowOffline(bool toggle);
    Q_INVOKABLE bool isShowOffline(void) const;
    Q_INVOKABLE void setContactsOnly(bool toggle);
    Q_INVOKABLE bool isContactsOnly() const;
    Q_INVOKABLE void setRequestsOnly(bool toggle);
    Q_INVOKABLE bool isRequestsOnly() const;
    Q_INVOKABLE void setBlockedOnly(bool toggle);
    Q_INVOKABLE bool isBlockedOnly() const;
    Q_INVOKABLE QString accountId() const;

    void filterByLastUsedAccount(const QString &accountId);

    Q_INVOKABLE void skipContacts(const QStringList &contactsList);
    Q_INVOKABLE void clearSkippedContacts();

public Q_SLOTS:
    void filterByConnection(Tp::ConnectionPtr connection);
    void filterByAccountId(const QString id);
    void filterByString(const QString filter);
    void slotResetModel(void);

Q_SIGNALS:
    void rowCountChanged();
    void openLastUsedAccount(const QString &accountId);
    void accountIdChanged(const QString &accountId);

protected:
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private Q_SLOTS:
    void slotSortByPresence(void);

private:
    TelepathyManager *mManager;
    QAbstractItemModel *mModel;
    Tp::ConnectionPtr mConnection;
    QString mServiceName;
    QString mAccountId;
    bool mHaveConnection;
    QString mStringFilter;
    Tp::Features mContactFeatures;
    bool mShowOffline;
    bool mContactsOnly;
    bool mRequestsOnly;
    bool mBlockedOnly;
    QStringList mSkippedContacts;
};

#endif
