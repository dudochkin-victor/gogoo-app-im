/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef MERGEDMODEL_H
#define MERGEDMODEL_H

#include <QAbstractItemModel>

class MergedModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit MergedModel(QObject *parent = 0);
    virtual ~MergedModel();

    void addModel(QAbstractItemModel *model);
    void removeModel(QAbstractItemModel *model);

    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;

protected:
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &index) const;
    int modelOffset(const QAbstractItemModel *sourceModel = 0) const;
    void updateRoleNames();

private Q_SLOTS:
    void onRowsAboutToBeInserted(const QModelIndex &sourceParent, int start, int end);
    void onRowsInserted(const QModelIndex &sourceParent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &sourceParent, int start, int end);
    void onRowsRemoved(const QModelIndex &sourceParent, int start, int end);
    void onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                              const QModelIndex &targetParent, int destinationRow);
    void onRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                     const QModelIndex &targetParent, int destinationRow);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onModelAboutToBeReset();
    void onModelReset();

private:
    QList<QAbstractItemModel*> mModels;
};

#endif // MERGEDMODEL_H
