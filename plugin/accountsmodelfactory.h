/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef ACCOUNTSMODELFACTORY_H
#define ACCOUNTSMODELFACTORY_H

#include "imaccountsmodel.h"

class TelepathyManager;

class AccountsModelFactory : public QObject
{
    Q_OBJECT

public:
    AccountsModelFactory(TelepathyManager *tm);

public Q_SLOTS:
    void onAccountManagerReady();

Q_SIGNALS:
    void modelCreated(IMAccountsModel *model);

private:
    TelepathyManager *mTpManager;
    IMAccountsModel *mModel;
};

#endif // ACCOUNTSMODELFACTORY_H
