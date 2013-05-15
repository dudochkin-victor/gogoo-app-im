/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef __implugin_h
#define __implugin_h

#include "imfeedmodel.h"
#include "imservmodel.h"
#include "../telepathy-qml-lib/telepathymanager.h"
#include "../telepathy-qml-lib/improtocolsmodel.h"
#include "../telepathy-qml-lib/panelschannelobserver.h"
#include "../telepathy-qml-lib/imchannelapprover.h"

#include <feedplugin.h>
#include <TelepathyQt4/ClientRegistrar>

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTranslator>

class McaServiceModel;
class McaFeedModel;
class ServiceModel;

// An MCA plugin to implement the McaFeedPlugin interface
class IMPlugin: public QObject, public McaFeedPlugin
{
    Q_OBJECT
    Q_INTERFACES(McaFeedPlugin)

public:
    explicit IMPlugin(QObject *parent = NULL);
    virtual ~IMPlugin();

    // Your plugin must provide a service model, which is basically a list
    //   of different feeds you provide. Most plugins will only provide one
    //   "service", but you could provide one for each account, for example,
    //   if there is more than one.
    QAbstractItemModel *serviceModel();

    // Through the service model, you will provide a service id/name string
    //   for each item. These must be unique among the services your plugin
    //   provides. When this next function is called, you will be passed the
    //   unique service name and you create the actual feed model to provide
    //   its data.
    // You should be able to return multiple models for the same service.
    QAbstractItemModel *createFeedModel(const QString &service);

    // this plugin does not support search yet
    McaSearchableFeed *createSearchModel(const QString &service,
                                         const QString &searchText);

protected:
    void initializeChannelObserver();
    void initializeChannelApprover();

private Q_SLOTS:
    void onConnectionAvailable(Tp::ConnectionPtr conn);
    void onAcceptContact(Tp::AccountPtr account, QString contactId);
    void onRejectContact(Tp::AccountPtr account, QString contactId);
    void loadTranslator();

private:
    TelepathyManager *m_tpManager;
    IMProtocolsModel *m_protocolsModel;
    IMServiceModel *m_serviceModel;
    PanelsChannelObserver *mObserver;
    IMChannelApprover *mChannelApprover;
    Tp::ClientRegistrarPtr mClientRegistrar;
    QMap<QString, IMFeedModel *> mFeedModels;
    QTranslator appTranslator;
};

#endif  // __implugin_h
