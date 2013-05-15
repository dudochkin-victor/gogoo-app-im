/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMGROUPCHATMODEL_H
#define IMGROUPCHATMODEL_H

#include "imgroupchatmodelitem.h"

#include <QAbstractListModel>
#include <TelepathyQt4/TextChannel>


class IMGroupChatModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit IMGroupChatModel(QObject *parent = 0);

    int rowCount(const QModelIndex & parent) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:

public Q_SLOTS:
    void onTextChannelAvailable(const QString &accountId, Tp::TextChannelPtr channel);

protected Q_SLOTS:
    void onChannelInvalidated();
    void onItemChanged(IMGroupChatModelItem *);

protected:
    QModelIndex index(IMGroupChatModelItem *item);

private:
    QList<IMGroupChatModelItem *> mChildren;

};

#endif // IMGROUPCHATMODEL_H
