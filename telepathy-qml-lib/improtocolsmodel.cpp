/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "improtocolsmodel.h"
#include <QDir>
#include <QDebug>

IMProtocolsModel::IMProtocolsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;

    roles[TitleRole] = "title";
    roles[IconRole] = "icon";
    roles[IdRole] = "id";
    roles[ConnectionManagerRole] = "connectionManager";
    roles[ProtocolRole] = "protocol";
    roles[SingleInstanceRole] = "singleInstance";
    setRoleNames(roles);

    QDir dir(modulePath());
    if (!dir.exists() || !dir.isReadable()) {
        return;
    }

    foreach (const QString &entry, dir.entryList(QStringList() << "*.desktop")) {
        MDesktopEntry *desktopEntry = new MDesktopEntry(dir.absoluteFilePath(entry));
        if (desktopEntry->value("Desktop Entry", "Type") != "IMProtocol") {
            delete desktopEntry;
            continue;
        }
        mProtocolList.append(desktopEntry);
        mProtocolMap[desktopEntry->value("MTI", "Id")] = desktopEntry;
    }
}


int IMProtocolsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return mProtocolList.count();
}

QVariant IMProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mProtocolList.count()) {
        return QVariant();
    }

    switch (role) {
    case TitleRole:
        return mProtocolList[index.row()]->name();
    case IconRole:
        return mProtocolList[index.row()]->value("MTI", "Icon");
    case IdRole:
        return mProtocolList[index.row()]->value("MTI", "Id");
    case ConnectionManagerRole:
        return mProtocolList[index.row()]->value("MTI", "ConnectionManager");
    case ProtocolRole:
        return mProtocolList[index.row()]->value("MTI", "Protocol");
    case SingleInstanceRole:
        if (mProtocolList[index.row()]->contains("MTI", "SingleInstance")) {
            return (mProtocolList[index.row()]->value("MTI", "SingleInstance") == "true");
        }
        return false;
    }

    return QVariant();
}

QString IMProtocolsModel::iconForId(const QString &id) const
{
    if (!mProtocolMap.contains(id)) {
        return QString::null;
    }

    return mProtocolMap[id]->value("MTI", "Icon");
}

QString IMProtocolsModel::contentForId(const QString &id) const
{
    if (!mProtocolMap.contains(id)) {
        return QString::null;
    }

    return mProtocolMap[id]->value("MTI", "Content");
}

QString IMProtocolsModel::titleForId(const QString &id) const
{
    if (!mProtocolMap.contains(id)) {
        return QString::null;
    }

    return mProtocolMap[id]->name();
}

bool IMProtocolsModel::isSingleInstance(const QString &id) const
{
    if (!mProtocolMap.contains(id) ||
        !mProtocolMap[id]->contains("MTI", "SingleInstance")) {
        return false;
    }

   return (mProtocolMap[id]->value("MTI", "SingleInstance") == "true");
}

QMap<QString, QString> IMProtocolsModel::protocolNames() const
{
    QMap<QString,QString> names;
    foreach (MDesktopEntry *entry, mProtocolList) {
        names[entry->value("MTI", "Id")] = entry->name();
    }

    return names;
}

QString IMProtocolsModel::modulePath() const
{
    return QString::fromLatin1("/usr/share/meego-app-im/protocols/");
}
