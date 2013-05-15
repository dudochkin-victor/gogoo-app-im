/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "imfeedmodelfilter.h"

#include <QDebug>

#include <QSettings>
#include <QTimer>


//
// public methods
//

IMFeedModelFilter::IMFeedModelFilter(QAbstractItemModel *source, QObject *parent):
        QSortFilterProxyModel(parent)
{
    setSourceModel(source);
    m_source = source;
}

IMFeedModelFilter::~IMFeedModelFilter()
{
    delete m_source;
}

void IMFeedModelFilter::setSearchText(const QString &text)
{
    m_searchText = text;
    invalidateFilter();
}

bool IMFeedModelFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)

    if (m_searchText.isEmpty())
        return false;

    QModelIndex sourceIndex = m_source->index(source_row, 0);
    QString title = m_source->data(sourceIndex, McaFeedModel::GenericTitleRole).toString();
    if (title.contains(m_searchText, Qt::CaseInsensitive))
        return true;

    QString content = m_source->data(sourceIndex, McaFeedModel::GenericContentRole).toString();
    if (content.contains(m_searchText, Qt::CaseInsensitive))
        return true;

    return false;
}
