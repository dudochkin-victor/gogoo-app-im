/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QDebug>

#include <QtPlugin>

#include "implugin.h"

#include "imfeedmodelfilter.h"
#include "imservmodel.h"

#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/Types>
#include <TelepathyQt/Debug>
#include <TelepathyQt4Yell/CallChannel>
#include <TelepathyQt4Yell/ChannelClassSpec>

IMPlugin::IMPlugin(QObject *parent): QObject(parent), McaFeedPlugin()
{
    Tp::registerTypes();


    qDebug() << "IMPlugin constructor";

    m_tpManager = new TelepathyManager(false);
    m_protocolsModel = new IMProtocolsModel(this);
    m_serviceModel = new IMServiceModel(m_tpManager, m_protocolsModel, this);
    connect(m_tpManager,
            SIGNAL(accountManagerReady()),
            SLOT(onAccountManagerReady()));

    m_tpManager->setProtocolNames(m_protocolsModel->protocolNames());

    mClientRegistrar = Tp::ClientRegistrar::create();

    initializeChannelObserver();
    initializeChannelApprover();

    connect(m_tpManager, SIGNAL(accountAvailable(Tp::AccountPtr)),
            m_serviceModel, SLOT(onAccountAvailable(Tp::AccountPtr)));
    connect(m_tpManager, SIGNAL(connectionAvailable(Tp::ConnectionPtr)),
            this, SLOT(onConnectionAvailable(Tp::ConnectionPtr)));

    // install translation catalogs
    loadTranslator();
    qApp->installTranslator(&appTranslator);

}

IMPlugin::~IMPlugin()
{
    qDebug() << "IMPlugin::~IMPlugin() terminating";
    mClientRegistrar->unregisterClients();
    delete m_serviceModel;
    delete m_tpManager;
    delete m_protocolsModel;
    mClientRegistrar->deleteLater();
    qDebug() << "IMPlugin::~IMPlugin() done terminating";
}

QAbstractItemModel *IMPlugin::serviceModel()
{
    return m_serviceModel;
}

QAbstractItemModel *IMPlugin::createFeedModel(const QString &service)
{
    qDebug() << "IMPlugin::createFeedModel: " << service;

    foreach (Tp::AccountPtr account, m_tpManager->accounts()) {
        if (account->uniqueIdentifier() == service) {
            IMFeedModel *model = new IMFeedModel(mObserver, account, this);

            connect(model, SIGNAL(applicationRunningChanged(bool)),
                    mChannelApprover, SLOT(setApplicationRunning(bool)));
            mFeedModels[service] = model;
            if (!account->connection().isNull()
                    && account->connection()->isValid()) {
                onConnectionAvailable(account->connection());
            }
            return model;
        }
    }
    return 0;
}

void IMPlugin::initializeChannelObserver()
{
    // setup the channel filters
    Tp::ChannelClassSpecList channelSpecList;

    channelSpecList.append(Tp::ChannelClassSpec::textChat());
    channelSpecList.append(Tp::ChannelClassSpec::textChatroom());
    channelSpecList.append(Tp::ChannelClassSpec::unnamedTextChat());
    channelSpecList.append(Tpy::ChannelClassSpec::mediaCall());
    channelSpecList.append(Tpy::ChannelClassSpec::audioCall());
    channelSpecList.append(Tpy::ChannelClassSpec::videoCall());
    channelSpecList.append(Tpy::ChannelClassSpec::videoCallWithAudio());
    channelSpecList.append(Tp::ChannelClassSpec::incomingFileTransfer());
    channelSpecList.append(Tp::ChannelClassSpec::outgoingFileTransfer());

    // create the channel observer
    mObserver = new PanelsChannelObserver(channelSpecList, true);
    Tp::AbstractClientPtr observer(mObserver);

    // register the observer
    mClientRegistrar->registerClient(observer, "MeeGoIMPanelsObserver");
}

void IMPlugin::initializeChannelApprover()
{
    // create the channel approver
    mChannelApprover = new IMChannelApprover();
    Tp::AbstractClientPtr approver(mChannelApprover);

    // register the approver
    mClientRegistrar->registerClient(approver, "MeeGoIMPanelsApprover");
}

void IMPlugin::onConnectionAvailable(Tp::ConnectionPtr conn)
{
    // find the account related to the connection and connect
    // the contact manager to the publication request slot on the model
    foreach (IMFeedModel *model, mFeedModels) {
        if (!model->account()->connection().isNull()
                && model->account()->connection()->isValid()) {
            if (model->account()->connection() == conn) {
                Tp::ContactManagerPtr manager = model->account()->connection()->contactManager();
                connect(manager.data(), SIGNAL(presencePublicationRequested(Tp::Contacts)),
                        model, SLOT(onPresencePublicationRequested(Tp::Contacts)));
                connect(manager.data(), SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)),
                        model, SLOT(onAllKnownContactsChanged(Tp::Contacts,Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)));
                connect(model, SIGNAL(acceptContact(Tp::AccountPtr,QString)), SLOT(onAcceptContact(Tp::AccountPtr,QString)));
                connect(model,SIGNAL(rejectContact(Tp::AccountPtr,QString)), SLOT(onRejectContact(Tp::AccountPtr,QString)));

                // Look for friend requests and listen for publish changes
                Tp::Contacts contacts = manager->allKnownContacts();
                QList<Tp::ContactPtr> friendRequests;
                foreach (Tp::ContactPtr contact, contacts) {
                    // if a friend request
                    if (contact->publishState() == Tp::Contact::PresenceStateAsk) {
                        friendRequests.append(contact);
                    }

                    // connect to publish changes
                    connect(contact.data(), SIGNAL(publishStateChanged(Tp::Contact::PresenceState,QString)),
                            model, SLOT(onPublishStateChanged(Tp::Contact::PresenceState)));
                }
                // Add the friend requests to the model
                if (friendRequests.count() > 0) {
                    model->onPresencePublicationRequested(Tp::Contacts().fromList(friendRequests));
                }
            }
        }
    }
}

void IMPlugin::onAcceptContact(Tp::AccountPtr account, QString contactId)
{
    foreach (IMFeedModel *model, mFeedModels) {
        if (model->account()->uniqueIdentifier() == account->uniqueIdentifier()) {
            if (!model->account()->connection().isNull()) {
                Tp::ContactManagerPtr manager = model->account()->connection()->contactManager();

                Tp::Contacts contacts = manager->allKnownContacts();
                QList<Tp::ContactPtr> contactsToAppend;
                QList<Tp::ContactPtr>  contactsList = contacts.toList();
                foreach (Tp::ContactPtr contact, contactsList) {
                    if (contact->id() == contactId) {
                        contactsToAppend.append(contact);
                        break;
                    }
                }
                manager->authorizePresencePublication(contactsToAppend);
            }
        }
    }
}

void IMPlugin::onRejectContact(Tp::AccountPtr account, QString contactId)
{
    foreach (IMFeedModel *model, mFeedModels) {
        if (model->account()->uniqueIdentifier() == account->uniqueIdentifier()) {
            if (!model->account()->connection().isNull()) {
                Tp::ContactManagerPtr manager = model->account()->connection()->contactManager();

                Tp::Contacts contacts = manager->allKnownContacts();
                QList<Tp::ContactPtr> contactsToAppend;
                QList<Tp::ContactPtr>  contactsList = contacts.toList();
                foreach (Tp::ContactPtr contact, contactsList) {
                    if (contact->id() == contactId) {
                        contactsToAppend.append(contact);
                        break;
                    }
                }
                manager->removePresencePublication(contactsToAppend);
            }
        }
    }
}

McaSearchableFeed *IMPlugin::createSearchModel(const QString &service, const QString &searchText)
{
    qDebug() << "IMPlugin::createSearchModel: " << service << "searchText: " << searchText;

    if (mFeedModels.contains(service)) {
        IMFeedModelFilter *filter = new IMFeedModelFilter(mFeedModels[service], this);
        filter->setSearchText(searchText);
        return filter;
    }
    return 0;
}

void IMPlugin::loadTranslator()
{
    appTranslator.load("meego-app-im_" + QLocale::system().name() + ".qm",
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

Q_EXPORT_PLUGIN2(im_panels_plugin, IMPlugin)
