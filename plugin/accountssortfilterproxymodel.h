/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef ACCOUNTSSORTFILTERPROXYMODEL_H
#define ACCOUNTSSORTFILTERPROXYMODEL_H

#include "imaccountsmodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

class AccountsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    // we need this so that the filter menu in QML can tell the size of the account switcher menu
    Q_PROPERTY(int length READ length NOTIFY lengthChanged)
public:
    AccountsSortFilterProxyModel(IMAccountsModel *model, QObject *parent = 0);
    ~AccountsSortFilterProxyModel();

    int length() const;

    /**
      * This allows to access account data by row number
      */
    Q_INVOKABLE QVariant dataByRow(const int row, const int role);

Q_SIGNALS:
    void lengthChanged();


protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

};

#endif // ACCOUNTSSORTFILTERPROXYMODEL_H
