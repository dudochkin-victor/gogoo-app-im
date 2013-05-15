/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "accountssortfilterproxymodel.h"

#include <TelepathyQt4Yell/Models/AccountsModel>

AccountsSortFilterProxyModel::AccountsSortFilterProxyModel(IMAccountsModel *model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(model);
    setSortRole(Tpy::AccountsModel::DisplayNameRole);
    setDynamicSortFilter(true);
    sort(0, Qt::AscendingOrder);
    invalidate();
    connect(model, SIGNAL(accountCountChanged()), SIGNAL(lengthChanged()));
}

AccountsSortFilterProxyModel::~AccountsSortFilterProxyModel()
{
}

bool AccountsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QString leftAlias = sourceModel()->data(left, Tpy::AccountsModel::DisplayNameRole).toString();
    QString rightAlias = sourceModel()->data(right, Tpy::AccountsModel::DisplayNameRole).toString();
    return (leftAlias.toUpper() < rightAlias.toUpper());
}

int AccountsSortFilterProxyModel::length() const
{
    IMAccountsModel *model = qobject_cast<IMAccountsModel *>(sourceModel());
    if (model) {
        return model->accountCount();
    }
    return 0;
}

QVariant AccountsSortFilterProxyModel::dataByRow(const int row, const int role)
{
    QModelIndex modelIndex = index(row, 0, QModelIndex());
    return data(modelIndex, role);
}
