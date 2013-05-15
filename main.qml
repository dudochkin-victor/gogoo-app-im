/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1
import MeeGo.App.IM 0.1
import TelepathyQML 0.1

Window {
    id: window

    // FIXME remove once migration to Meego UX components is completed
    signal orientationChangeFinished();

    toolBarTitle: qsTr("Chat")
    fullScreen: true
    customActionMenu: true
    automaticBookSwitching: false

    property int animationDuration: 250

    property string currentAccountId: ""
    property int    currentAccountStatus: -1 // will get filled when accountItem gets filled
    property string currentAccountName: "" // will get filled when accountItem gets filled
    property variant accountItem
    property variant contactItem
    property variant callAgent
    property variant fileTransferAgent
    property variant chatAgent
    property variant incomingContactItem
    property variant incomingCallAgent
    property variant currentPage
    property string cmdCommand: ""
    property string cmdAccountId: ""
    property string cmdContactId: ""
    property variant accountFilterModel: [ ]

    // this property will be set right before opening the conversation screen
    // TODO: check how can we do that on group chat
    property string currentContactId: ""

    signal componentsLoaded

    FuzzyDateTime {
        id: fuzzyDateTime
    }

    Timer {
        id: fuzzyDateTimeUpdater
        interval: /*1 */ 60 * 1000 // 1 min
        repeat: true
        running: true
    }

    onCurrentAccountIdChanged: {
        contactsModel.filterByAccountId(currentAccountId);
        contactRequestModel.filterByAccountId(currentAccountId);
        accountItem = accountsModel.accountItemForId(window.currentAccountId);
        currentAccountStatus = accountItem.data(AccountsModel.CurrentPresenceTypeRole);
        currentAccountName = accountItem.data(AccountsModel.DisplayNameRole);
        notificationManager.currentAccount = currentAccountId;
        accountItemConnections.target = accountItem;
    }

    onCurrentContactIdChanged: {
        notificationManager.currentContact = currentContactId;
    }

    onIsActiveWindowChanged: {
        notificationManager.applicationActive = isActiveWindow;
    }

    Component.onCompleted: {
        notificationManager.applicationActive = isActiveWindow;
        switchBook(accountScreenContent);
    }

    onBookMenuTriggered: {
        if(selectedItem != "") {
            currentAccountId = selectedItem;
            accountItem = accountsModel.accountItemForId(currentAccountId);
            window.switchBook(accountScreenContent);
            componentsLoaded();
            window.addPage(contactsScreenContent);
        } else {
            window.switchBook(accountScreenContent);
            componentsLoaded();
        }
    }

    Connections {
        target:  null;
        id: accountItemConnections

        onChanged: {
            currentAccountStatus = accountItem.data(AccountsModel.CurrentPresenceTypeRole);
            currentAccountName = accountItem.data(AccountsModel.DisplayNameRole);
            notificationManager.currentAccount = currentAccountId;
        }
    }

    Connections {
        // this signal is not created yet, so wait till it is, then the target will be set
        // and the connection will be made then.
        target: null;
        id: contactsModelConnections;
        onOpenLastUsedAccount: {
            // If there command line parameters,those take precedence over opening the last
            // used account
            parseWindowParameters(mainWindow.call);
            if(cmdCommand == "") {
                currentAccountId = accountId;
                accountItem = accountsModel.accountItemForId(window.currentAccountId);
                currentAccountId = accountItem.data(AccountsModel.IdRole);
                addPage(contactsScreenContent);
            } else {
                if(cmdCommand == "show-chat" || cmdCommand == "show-contacts") {
                    currentAccountId = cmdAccountId;
                    accountItem = accountsModel.accountItemForId(currentAccountId);

                    if(cmdCommand == "show-chat") {
                        currentContactId = cmdContactId;
                        contactItem = accountsModel.contactItemForId(currentAccountId, currentContactId);
                        startConversation(currentContactId);
                    } else if(cmdCommand == "show-contacts") {
                        addPage(contactsScreenContent);
                    }
                }
            }
        }
    }

    Connections {
        // the onComponentsLoaded signal is not created yet, but we can't use the same trick as the other
        // signals to connect them later since this is the signal that gets used to connect the others.
        // We then choose to ignore the runtime error for just this signal.
        target: accountsModel
        ignoreUnknownSignals: true
        onComponentsLoaded: {
            // the other signals should now be set up, so plug them in
            contactsModelConnections.target = contactsModel;
            accountsModelConnections.target = accountsModel;

            telepathyManager.registerClients();
            reloadFilterModel();
            componentsLoaded();
            buildBookMenuPayloadModel();
        }
    }

    Connections {
        // those signals are not created yet, so wait till they are, then the target will be set
        // and the connections will be made then.
        target: null
        id: accountsModelConnections

        onAccountConnectionStatusChanged: {
            var item = accountsModel.accountItemForId(accountId);
            if(accountId == currentAccountId) {
                contactsModel.filterByAccountId(currentAccountId);
                contactRequestModel.filterByAccountId(currentAccountId);
                accountItem = item;
                currentAccountStatus = accountItem.data(AccountsModel.CurrentPresenceTypeRole);
            }

            // check if there is a connection error and show the config dialog
            var connectionStatus = item.data(AccountsModel.ConnectionStatusRole)
            var connectionStatusReason = item.data(AccountsModel.ConnectionStatusReasonRole)

            if ((connectionStatus == TelepathyTypes.ConnectionStatusDisconnected) &&
                ((connectionStatusReason == TelepathyTypes.ConnectionStatusReasonAuthenticationFailed) ||
                 (connectionStatusReason == TelepathyTypes.ConnectionStatusReasonNameInUse))) {
                window.addPage(accountFactory.componentForAccount(accountId, window));
            }

        }

        onIncomingFileTransferAvailable: {
            if (!notificationManager.chatActive) {
                var oldContactId = currentContactId;
                var oldAccountId = currentAccountId;

                // set the current contact property
                currentContactId = contactId;
                currentAccountId = accountId;
                contactItem = accountsModel.contactItemForId(accountId, contactId);
                callAgent = accountsModel.callAgent(accountId, contactId);
                fileTransferAgent = accountsModel.fileTransferAgent(window.currentAccountId, contactId);
                accountsModel.startChat(window.currentAccountId, contactId);
                chatAgent = accountsModel.chatAgentByKey(window.currentAccountId, contactId);

                window.addPage(messageScreenContent);
            }
        }

        onIncomingCallAvailable: {
            window.incomingContactItem = accountsModel.contactItemForId(accountId, contactId);
            window.incomingCallAgent = accountsModel.callAgent(accountId, contactId)
            incomingCallDialog.accountId = accountId;
            incomingCallDialog.contactId = contactId;
            incomingCallDialog.statusMessage = (window.incomingContactItem.data(AccountsModel.PresenceMessageRole) != "" ?
                                            window.incomingContactItem.data(AccountsModel.PresenceMessageRole) :
                                            window.presenceStatusText(window.incomingContactItem.data(AccountsModel.PresenceTypeRole)));
            incomingCallDialog.connectionTarget = window.incomingCallAgent;
            incomingCallDialog.start();
        }

        onRequestedGroupChatCreated: {
            window.chatAgent = agent;

            window.currentContactId = "";
            window.contactItem = undefined;
            window.callAgent = undefined;

            window.addPage(messageScreenContent);
            accountsModel.startGroupChat(window.currentAccountId, window.chatAgent.channelPath)
        }

        onPasswordRequestRequired: {
            window.addPage(accountFactory.componentForAccount(accountId, window));
        }

        onDataChanged: {
            reloadFilterModel();
        }

        onAccountCountChanged: {
            buildBookMenuPayloadModel();
        }
    }

    bookMenuModel: accountFilterModel

    function buildBookMenuPayloadModel()
    {
        var payload = new Array();
        for (var i = 0; i < accountsSortedModel.length; ++i) {
            payload[i] = accountsSortedModel.dataByRow(i, AccountsModel.IdRole );
        }
        payload[accountsSortedModel.length] = "";
        bookMenuPayload = payload;
    }

    function startConversation(contactId)
    {
        // set the current contact property
        currentContactId = contactId;
        contactItem = accountsModel.contactItemForId(window.currentAccountId, window.currentContactId);
        callAgent = accountsModel.callAgent(window.currentAccountId, contactId);
        fileTransferAgent = accountsModel.fileTransferAgent(window.currentAccountId, contactId);

        // and start the conversation
        if (notificationManager.chatActive) {
            window.popPage();
        }

        window.addPage(messageScreenContent);
        accountsModel.startChat(window.currentAccountId, contactId);

        chatAgent = accountsModel.chatAgentByKey(window.currentAccountId, contactId);
    }

    function startGroupConversation(channelPath)
    {
        window.currentContactId = "";
        window.contactItem = undefined;
        window.callAgent = undefined;

        window.chatAgent = accountsModel.chatAgentByKey(window.currentAccountId, channelPath);

        // and start the conversation
        if (notificationManager.chatActive) {
            window.popPage();
        }
        window.addPage(messageScreenContent);
        accountsModel.startGroupChat(window.currentAccountId, window.chatAgent.channelPath)
    }

    function acceptCall(accountId, contactId)
    {
        if (notificationManager.chatActive) {
            window.popPage();
        }

        // set the current contact property
        window.currentContactId = contactId;
        window.currentAccountId = accountId;
        window.contactItem = accountsModel.contactItemForId(accountId, contactId);
        window.callAgent = accountsModel.callAgent(accountId, contactId);
        window.fileTransferAgent = accountsModel.fileTransferAgent(window.currentAccountId, contactId);

        // and start the conversation
        window.addPage(messageScreenContent);
        accountsModel.startChat(accountId, contactId);
        chatAgent = accountsModel.chatAgentByKey(accountId, contactId);
    }

    function startAudioCall(contactId, page)
    {
        // set the current contact property
        currentContactId = contactId;
        contactItem = accountsModel.contactItemForId(window.currentAccountId, window.currentContactId);

        //create the audio call agent
        //the message screen will then get the already created agent
        callAgent = accountsModel.callAgent(window.currentAccountId, contactId);
        fileTransferAgent = accountsModel.fileTransferAgent(window.currentAccountId, contactId);
        accountsModel.startChat(window.currentAccountId, contactId);
        chatAgent = accountsModel.chatAgentByKey(window.currentAccountId, contactId);
        callAgent.audioCall();

        // and start the conversation
        if (notificationManager.chatActive) {
            window.popPage();
        }
        window.addPage(messageScreenContent);
    }

    function startVideoCall(contactId, page)
    {
        // set the current contact property
        currentContactId = contactId;
        contactItem = accountsModel.contactItemForId(window.currentAccountId, window.currentContactId);

        //create the audio call agent
        //the message screen will then get the already created agent
        callAgent = accountsModel.callAgent(window.currentAccountId, contactId);
        fileTransferAgent = accountsModel.fileTransferAgent(window.currentAccountId, contactId);
        accountsModel.startChat(window.currentAccountId, contactId);
        chatAgent = accountsModel.chatAgentByKey(window.currentAccountId, contactId);
        callAgent.videoCall();

        // and start the conversation
        if (notificationManager.chatActive) {
            window.popPage()
        }
        window.addPage(messageScreenContent);
    }

    function pickContacts()
    {
        window.addPage(contactPickerContent)
    }

    function reloadFilterModel()
    {
        // do not do anything if accountsSortedModel is not created yet
        if (typeof(accountsSortedModel) == 'undefined')
            return;

        accountFilterModel = [];

        var accountsList = new Array();
        for (var i = 0; i < accountsSortedModel.length; ++i) {
            accountsList[i] = accountsSortedModel.dataByRow(i, AccountsModel.DisplayNameRole);
        }
        accountsList[accountsList.length] = qsTr("Account switcher");
        accountFilterModel = accountsList;
    }

    function presenceStatusText(type)
    {
        if(type == TelepathyTypes.ConnectionPresenceTypeAvailable) {
            return qsTr("Available");
        } else if(type == TelepathyTypes.ConnectionPresenceTypeBusy) {
            return qsTr("Busy");
        } else if(type == TelepathyTypes.ConnectionPresenceTypeAway) {
            return qsTr("Away");
        } else if(type == TelepathyTypes.ConnectionPresenceTypeExtendedAway) {
            return qsTr("Extended away");
        } else if(type == TelepathyTypes.ConnectionPresenceTypeOffline) {
            return qsTr("Offline");
        } else if(type == TelepathyTypes.ConnectionPresenceTypeHidden) {
            return qsTr("Invisible");
        } else {
            return "";
        }
    }

    function parseWindowParameters(parameters)
    {
        // only parse if not empty
        if (parameters.length == 0) {
            return;
        }

        var cmd = parameters[0];
        var cdata = parameters[1];

        if (cmd == "show-chat" || cmd == "show-contacts") {
            cmdCommand = cmd;
            var parsedParameter = cdata;
            if (parsedParameter.indexOf("&") > 0) {
                cmdAccountId = parsedParameter.substr(0, parsedParameter.indexOf("&"));
            } else {
                cmdAccountId = cdata;
            }

            //message type
            if (cmd == "show-chat") {
                //also get the contact id to open a chat with
                cmdContactId = parsedParameter.substr(parsedParameter.indexOf("&") + 1, parsedParameter.length - 1);
            }
        }
    }

    AccountContentFactory {
        id: accountFactory
    }

    Component {
        id: contactsScreenContent
        ContactsScreenContent {
            id: contactsScreenItem
            anchors.fill: parent
        }
    }

    Component {
        id: accountScreenContent
        AccountScreenContent {
            id: accountScreenItem
            anchors.fill: parent
        }
    }

    Component {
        id: messageScreenContent
        MessageScreenContent {
            id: messageScreenItem
            anchors.fill: parent
        }
    }


    IncomingCall {
        id: incomingCallDialog
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }
    }

    Component {
        id: contactPickerContent
        ContactPickerContent {
            id: contactPickerItem
            anchors.fill: parent
        }
    }

    ConfirmationDialog {
        id: confirmationDialogItem
    }
}



