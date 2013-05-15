/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "components.h"
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/qdeclarative.h>
#include <QtDebug>

#include <TelepathyQt4Yell/Models/AccountsModel>
#include "telepathytypes.h"
#include "callagent.h"
#include "addcontacthelper.h"

void Components::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    Q_UNUSED(engine);

    qDebug() << "telepathy-qml initializeEngine" << uri;

    //Q_ASSERT(engine);
    //QDeclarativeContext *rootContext = engine->rootContext();
    //Q_ASSERT(rootContext);
}

void Components::registerTypes(const char *uri)
{
    qmlRegisterType<TelepathyTypes>(uri, 0, 1, "TelepathyTypes");
    qmlRegisterType<AddContactHelper>(uri, 0, 1, "AddContactHelper");
    qmlRegisterUncreatableType<Tpy::AccountsModel>(uri, 0, 1, "AccountsModel", "This is a read-only type");
    qmlRegisterUncreatableType<Tpy::AccountsModelItem>(uri, 0, 1, "AccountsModelItem", "This is a read-only type");
    qmlRegisterUncreatableType<CallAgent>(uri, 0, 1, "CallAgent", "Cannot be instanced from QML");
}

Q_EXPORT_PLUGIN2(components, Components);
