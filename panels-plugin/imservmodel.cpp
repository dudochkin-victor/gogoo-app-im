/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <actions.h>

#include "imservmodel.h"

IMServiceModel::IMServiceModel(TelepathyManager *tpManager, IMProtocolsModel *protoModel, QObject *parent)
    : McaServiceModel(parent),
      m_tpManager(tpManager),
      m_protocolsModel(protoModel)
{
    m_actions = new McaActions;
}

IMServiceModel::~IMServiceModel()
{
    delete m_actions;
}

int IMServiceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_names.size();
}

QVariant IMServiceModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= m_names.size())
        return QVariant();

    switch (role) {
    case CommonDisplayNameRole:
        // This display name is a localized name for your service -- if you
        //   provide more than one service, each should have a distinct name
        //   for example, identifying the account. But it should be a title,
        //   preferably under ~32 characters.
        return m_tpManager->accountDisplayName(m_accounts.at(row)->iconName(), m_accounts.at(row)->displayName());

    case CommonIconUrlRole:
        // Here you can provide a small icon that identifies the service.
        //   This icon would probably be the same for each account if you
        //   provide multiple accounts as separate "services".
        return m_icons.at(row);

    case RequiredCategoryRole:
        // Currently we define three categories: "social", "email", and "im"
        //   that will be pulled into the Friends panel. In the future we
        //   may extend the categories for other panels and purposes.
        return m_categories.at(row);

    case RequiredNameRole:
        // This field is a unique name for the service within this plugin
        //   If you provide multiple accounts each should have its own
        //   unique id. This is not user-visible.
        return m_names.at(row);

    case CommonActionsRole:
        // This is required if you will ever return true for
        //   CommonConfigErrorRole.
        return QVariant::fromValue<McaActions*>(m_actions);

    case CommonCapFlagsRole:
        return McaServiceModel::ProvidesFeed;

    case CommonConfigErrorRole:
        return false;

    default:
        // There is also the CommonConfigErrorRole which is a bool and just
        //   requests the UI to alert the user to misconfiguration, and gives
        //   them an opportunity to configure your service. If you return
        //   true here, you must also provide the CommonActionsRole above
        //   with a handler watching for a "configure" action.
        qWarning() << "IM Plugin Service model Unhandled data role requested! Role: " << role ;
        return QVariant();
    }
}

void IMServiceModel::performAction(QString action, QString uniqueid)
{
    // The mtfcontent sample application provides Configure buttons for each
    //   service you report so you can test that you are receiving the
    //   configure signal properly. In the real application, we plan to only
    //   provide this option to the user if you report that there is a
    //   configuration error through CommonConfigErrorRole.
    if (action == "configure")
        configure(uniqueid);
}

void IMServiceModel::configure(QString serviceName)
{
    Q_UNUSED(serviceName);
    qDebug() << "Configure option not available";
}

void IMServiceModel::onAccountAvailable(Tp::AccountPtr account)
{
    // check if it already exists
    foreach(QString id, m_names) {
        if(id == account->uniqueIdentifier()) {
            return;
        }
    }

    connect(account.data(), SIGNAL(removed()),
            SLOT(onAccountRemoved()));

    // add if new
    beginInsertRows(QModelIndex(), m_names.size(), m_names.size());
    //m_displayNames.append(tr("%1 - %2").arg(account->displayName(), accountServiceName(account->iconName()))); // account and service name

    m_accounts.append(account.data());
    m_icons.append(m_protocolsModel->iconForId(account->iconName()));
    m_categories.append("im");
    m_names.append(account->uniqueIdentifier());
    onItemChanged(m_names.size() - 1);
    endInsertRows();

}

void IMServiceModel::onItemChanged(int row)
{
    QModelIndex rowIndex = index(row, 0);
    emit dataChanged(rowIndex, rowIndex);
}

void IMServiceModel::onAccountRemoved()
{
    Tp::Account *account = qobject_cast<Tp::Account *>(sender());

    // make sure we are not acting on a null object
    if (account) {
        QString id = account->uniqueIdentifier();
        if (!id.isEmpty()) {
            foreach(QString name, m_names) {
                if (name == id) {
                    int i = m_names.indexOf(name);

                    //remove the item in all lists
                    beginRemoveRows(QModelIndex(), i, i);
                    m_accounts.removeAt(i);
                    m_icons.removeAt(i);
                    m_categories.removeAt(i);
                    m_names.removeAt(i);
                    endRemoveRows();
                    break;
                }
            }
        }
    }
}
