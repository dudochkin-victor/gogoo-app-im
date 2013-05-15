/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

/*
 * MeeGo Chat Content Aggregator
 * Copyright © 2010, Intel Corporation.
 */

#ifndef IMFEEDMODELFILTER_H
#define IMFEEDMODELFILTER_H

#include <QSortFilterProxyModel>

#include <feedmodel.h>

class McaActions;

class IMFeedModelFilter: public QSortFilterProxyModel, public McaSearchableFeed
{
    Q_OBJECT

public:
    IMFeedModelFilter(QAbstractItemModel *source, QObject *parent = NULL);
    ~IMFeedModelFilter();

    void setSearchText(const QString& text);

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QAbstractItemModel *m_source;
    QString m_searchText;
};

#endif // IMFEEDMODELFILTER_H
