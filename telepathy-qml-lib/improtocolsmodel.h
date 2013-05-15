/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef IMPROTOCOLSMODEL_H
#define IMPROTOCOLSMODEL_H

#include <QAbstractListModel>
#include <QMap>
#include <meegotouch/MDesktopEntry>

class IMProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString modulePath READ modulePath NOTIFY modulePathChanged)
public:
    enum Roles {
        TitleRole = Qt::UserRole,
        IconRole,
        IdRole,
        ConnectionManagerRole,
        ProtocolRole,
        SingleInstanceRole
    };

    explicit IMProtocolsModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    Q_INVOKABLE QString iconForId(const QString &id) const;
    Q_INVOKABLE QString contentForId(const QString &id) const;
    Q_INVOKABLE QString titleForId(const QString &id) const;
    Q_INVOKABLE bool isSingleInstance(const QString &id) const;

    QMap<QString, QString> protocolNames() const;

    QString modulePath() const;

Q_SIGNALS:
    void modulePathChanged();

private:
    QList<MDesktopEntry*> mProtocolList;
    QMap<QString, MDesktopEntry*> mProtocolMap;
};

#endif // IMPROTOCOLSMODEL_H
