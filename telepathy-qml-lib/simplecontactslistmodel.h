/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef SIMPLECONTACTSLISTMODEL_H
#define SIMPLECONTACTSLISTMODEL_H

#include <TelepathyQt/Contact>
#include <TelepathyQt4Yell/Models/ContactModelItem>

#include <QAbstractListModel>

class SimpleContactsListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged())
public:
    explicit SimpleContactsListModel(const QList<Tp::ContactPtr> contacts, QObject *parent = new QObject());
    ~SimpleContactsListModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

Q_SIGNALS:
    void rowCountChanged();

private:
    QList<Tpy::ContactModelItem *> mItems;
};

#endif // SIMPLECONTACTSLISTMODEL_H
