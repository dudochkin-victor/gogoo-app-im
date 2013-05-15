/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "mergedmodel.h"
#include <QDebug>

MergedModel::MergedModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

MergedModel::~MergedModel()
{
    qDeleteAll(mModels);
    mModels.clear();
}

void MergedModel::addModel(QAbstractItemModel *model)
{
    if (mModels.indexOf(model) >= 0) {
        qDebug() << "MergedModel::addModel: WARNING: trying to add a model we were already handling:" << model;
        return;
    }

    int offset = modelOffset();

    beginInsertRows(QModelIndex(), offset, offset + model->rowCount() - 1);

    connect(model,
            SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)),
            SLOT(onRowsAboutToBeInserted(const QModelIndex&, int, int)));
    connect(model,
            SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            SLOT(onRowsInserted(const QModelIndex&, int, int)));
    connect(model,
            SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
            SLOT(onRowsAboutToBeRemoved(const QModelIndex&, int, int)));
    connect(model,
            SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
            SLOT(onRowsRemoved(const QModelIndex&, int, int)));
    connect(model,
            SIGNAL(rowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
            SLOT(onRowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)));
    connect(model,
            SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
            SLOT(onRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)));
    connect(model,
            SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
    connect(model,
            SIGNAL(modelAboutToBeReset()),
            SLOT(onModelAboutToBeReset()));
    connect(model,
            SIGNAL(modelReset()),
            SLOT(onModelReset()));
    connect(model,
            SIGNAL(layoutAboutToBeChanged()),
            SIGNAL(layoutAboutToBeChanged()));
    connect(model,
            SIGNAL(layoutChanged()),
            SIGNAL(layoutChanged()));

    mModels.append(model);
    updateRoleNames();

    endInsertRows();
}

void MergedModel::removeModel(QAbstractItemModel *model)
{
    if (mModels.indexOf(model) < 0) {
        qDebug() << "WARNING: trying to remove a model we were not handling:" << model;
        return;
    }

    int offset = modelOffset(model);

    beginRemoveRows(QModelIndex(), offset, offset + model->rowCount() -1);

    disconnect(model,
               SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)),
               this,
               SLOT(onRowsAboutToBeInserted(const QModelIndex&, int, int)));
    disconnect(model,
               SIGNAL(rowsInserted(const QModelIndex&, int, int)),
               this,
               SLOT(onRowsInserted(const QModelIndex&, int, int)));
    disconnect(model,
               SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
               this,
               SLOT(onRowsAboutToBeRemoved(const QModelIndex&, int, int)));
    disconnect(model,
               SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
               this,
               SLOT(onRowsRemoved(const QModelIndex&, int, int)));
    disconnect(model,
               SIGNAL(rowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
               this,
               SLOT(onRowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)));
    disconnect(model,
               SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
               this,
               SLOT(onRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)));
    disconnect(model,
               SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
               this,
               SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
    disconnect(model,
               SIGNAL(modelAboutToBeReset()),
               this,
               SLOT(onModelAboutToBeReset()));
    disconnect(model,
               SIGNAL(modelReset()),
               this,
               SLOT(onModelReset()));
    disconnect(model,
               SIGNAL(layoutAboutToBeChanged()),
               this,
               SIGNAL(layoutAboutToBeChanged()));
    disconnect(model,
               SIGNAL(layoutChanged()),
               this,
               SIGNAL(layoutChanged()));

    mModels.removeAll(model);
    updateRoleNames();

    endRemoveRows();
}

int MergedModel::rowCount(const QModelIndex &parent) const
{
    // we don't support tree models yet
    if (parent.isValid()) {
        return 0;
    }

    int count = 0;
    foreach (const QAbstractItemModel *model, mModels) {
        count += model->rowCount();
    }

    return count;
}

int MergedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    // we don't support multi-columns yet
    return 1;
}

QVariant MergedModel::data(const QModelIndex &index, int role) const
{
    QModelIndex sourceIndex = mapToSource(index);

    if(!sourceIndex.isValid()) {
        qDebug() << "MergedModel::data: Invalid index returned";
        return QVariant();
    }

    return sourceIndex.data(role);
}

bool MergedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QModelIndex sourceIndex = mapToSource(index);
    QAbstractItemModel *sourceModel = static_cast<QAbstractItemModel*>(sourceIndex.internalPointer());
    if (!sourceModel) {
        return false;
    }

    return sourceModel->setData(sourceIndex, value, role);
}

QModelIndex MergedModel::index(int row, int column, const QModelIndex &parent) const
{
    // we don't support multi-level models yet
    if (parent.isValid()) {
        return QModelIndex();
    }

    int sourceRow = row;

    foreach(QAbstractItemModel *model, mModels) {
        // if the model's rowCount is less than the sourceRow,
        // we should skip to the next one
        if (model->rowCount() <= sourceRow) {
            sourceRow -= model->rowCount();
            continue;
        }

        return createIndex(row, column, model);
    }

    // this one shouldn't happen
    qDebug() << "MergedModel::index: WARNING: Could not create a valid index at row" << row;
    return QModelIndex();
}

QModelIndex MergedModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)

    // we don't support multi-level models yet
    return QModelIndex();
}

QModelIndex MergedModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    QAbstractItemModel *sourceModel = const_cast<QAbstractItemModel*>(sourceIndex.model());

    if (mModels.indexOf(sourceModel) < 0) {
        qDebug() << "MergedModel::mapFromSource: WARNING: index from a model we are not handling:" << sourceIndex;
        return QModelIndex();
    }

    return createIndex(sourceIndex.row() + modelOffset(sourceModel), 0, sourceModel);
}

QModelIndex MergedModel::mapToSource(const QModelIndex &index) const
{
    int sourceRow = index.row();

    foreach(QAbstractItemModel *model, mModels) {
        // if the model's rowCount is less than the sourceRow,
        // we should skip to the next one
        if (model->rowCount() <= sourceRow) {
            sourceRow -= model->rowCount();
            continue;
        }

        return model->index(sourceRow, 0);
    }

    qDebug() << "MergedModel::mapToSource: WARNING: Could not map the following index to any source model:" << index;
    return QModelIndex();
}

int MergedModel::modelOffset(const QAbstractItemModel *sourceModel) const
{
    int offset = 0;
    foreach(QAbstractItemModel *model, mModels) {
        if (sourceModel != model) {
            offset += model->rowCount();
            continue;
        }
        break;
    }

    return offset;
}

void MergedModel::updateRoleNames()
{
    QHash<int, QByteArray> roles;

    foreach (const QAbstractItemModel *model, mModels) {
        const QHash<int, QByteArray> &modelRoles = model->roleNames();
        QHash<int, QByteArray>::const_iterator it = modelRoles.constBegin();

        while (it != modelRoles.constEnd()) {
            roles[it.key()] = it.value();
            ++it;
        }
    }

    setRoleNames(roles);
}

void MergedModel::onRowsAboutToBeInserted(const QModelIndex &sourceParent, int start, int end)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    int offset = modelOffset(sourceModel);
    beginInsertRows(QModelIndex(), start + offset, end + offset);
}

void MergedModel::onRowsInserted(const QModelIndex &sourceParent, int start, int end)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    Q_UNUSED(start)
    Q_UNUSED(end)

    endInsertRows();
}

void MergedModel::onRowsAboutToBeRemoved(const QModelIndex &sourceParent, int start, int end)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    int offset = modelOffset(sourceModel);
    beginRemoveRows(QModelIndex(), start + offset, end + offset);
}

void MergedModel::onRowsRemoved(const QModelIndex &sourceParent, int start, int end)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    Q_UNUSED(start)
    Q_UNUSED(end)

    endRemoveRows();
}

void MergedModel::onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                       const QModelIndex &targetParent, int destinationRow)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid() || targetParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    int offset = modelOffset(sourceModel);
    beginMoveRows(QModelIndex(), sourceStart + offset, sourceEnd + offset,
                            QModelIndex(), destinationRow + offset);
}

void MergedModel::onRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                 const QModelIndex &targetParent, int destinationRow)
{
    // we don't support multi-level models yet
    if (sourceParent.isValid() || targetParent.isValid()) {
        return;
    }

    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationRow)

    endMoveRows();
}

void MergedModel::onDataChanged(const QModelIndex &sourceTopLeft, const QModelIndex &sourceBottomRight)
{
    QAbstractItemModel *sourceModel = qobject_cast<QAbstractItemModel*>(sender());
    if (!sourceModel) {
        return;
    }

    QModelIndex topLeft = mapFromSource(sourceTopLeft);
    QModelIndex bottomRight = mapFromSource(sourceBottomRight);

    if (topLeft.isValid() && bottomRight.isValid()) {
        emit dataChanged(topLeft, bottomRight);
    }
}

void MergedModel::onModelAboutToBeReset()
{
    beginResetModel();
}

void MergedModel::onModelReset()
{
    endResetModel();
}
