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

Column {
    id: mainArea
    anchors.left:  parent.left
    anchors.right: parent.right
    anchors.margins: 10
    height: childrenRect.height
    spacing: 10

    property int animationDuration: 100
    property alias contentGrid: contentGrid

    move: Transition {
        NumberAnimation {
            properties: "y"
            duration: mainArea.animationDuration
        }
    }

    add: Transition {
        NumberAnimation {
            properties: "opacity"
            duration: mainArea.animationDuration
        }
    }

    property alias accountHelper: accountHelperItem
    property alias connectionManager: accountHelperItem.connectionManager
    property alias protocol: accountHelperItem.protocol
    property alias icon: accountHelperItem.icon
    property string accountId: ""
    property variant accountItem: accountsModel !== null ? accountsModel.accountItemForId(accountId) : null
    property string serviceName: protocolsModel.titleForId(icon)

    property alias advancedOptionsComponent: advancedOptions.sourceComponent
    property alias advancedOptionsItem: advancedOptions.item

    property bool edit: accountId != ""

    property bool duplicated: false
    property string oldLogin: ""

    signal aboutToCreateAccount()
    signal aboutToEditAccount()
    signal finished()
    signal accountCreationAborted()

    AccountHelper {
        id: accountHelperItem

        displayName: loginBox.text
        password: passwordBox.text
        model: accountsModel

        onAccountSetupFinished: {
            mainArea.finished();
        }
    }

    Connections {
        target: confirmationDialogItem
        onAccepted: {
            if (confirmationDialogItem.instanceReason != "account-setup-single-instance") {
                return;
            }

            // if the dialog was accepted we should disconnect all other accounts
            // of the same type
            accountFactory.disconnectOtherAccounts(icon, (edit ? accountId : ""));

            // and finally create the account
            accountHelper.createAccount();
        }
        onRejected: {
            if (confirmationDialogItem.instanceReason != "account-setup-single-instance") {
                return;
            }

            // ask the account helper not to connect the account
            accountHelper.connectAfterSetup = false;

            // and create the account
            accountHelper.createAccount();
        }
    }

    function createAccount() {
        // emit the aboutToCreate signal so that accounts that need proper setup
        aboutToCreateAccount();

        if (accountsModel !== null && accountsModel.isAccountRegistered(connectionManager, protocol, loginBox.text)
            && oldLogin != loginBox.text) {
            duplicated = true;
            accountCreationAborted();
            loginBox.focus = true;
            return;
        }

        // check if the service allows for more than one account to be used at the same time
        if (protocolsModel.isSingleInstance(icon)) {
            // check if there is any other account of type online
            if (accountFactory.otherAccountsOnline(icon, (edit ? accountId : "")) > 0) {
                // TODO: show the dialog asking if the other accounts should be signed off
                confirmationDialogItem.instanceReason = "account-setup-single-instance"; // i18n ok
                confirmationDialogItem.title = qsTr("Multiple accounts connected");
                confirmationDialogItem.text = qsTr("Do you really want to connect this account? By doing this all other %1 accounts will be disconnected.").arg(serviceName);
                confirmationDialogItem.show();
                return;
            }
        }

        // and then creates the real account
        accountHelper.createAccount();
    }

    function removeAccount() {
        // just ask the account helper to remove the account
        accountHelper.removeAccount();
    }

    function prepareAccountEdit()
    {
        var item = accountsModel.accountItemForId(accountId);
        loginBox.text = item.data(AccountsModel.DisplayNameRole);
        oldLogin = loginBox.text
        accountHelperItem.setAccount(item);
        passwordBox.text = accountHelperItem.password;

        aboutToEditAccount();
    }

    function wrapUndefined(value, type)
    {
        if (typeof(value) == 'undefined') {
            if (type == "string")
                return "";
            else if (type == "number")
                return 0;
            else if (type == "bool")
                return false;
        }
        return value;
    }

    InfoPanel {
        id: accountError

        property int connectionStatus: (accountItem != undefined) ? accountItem.data(AccountsModel.ConnectionStatusRole) : TelepathyTypes.ConnectionStatusDisconnected
        property int connectionStatusReason: (accountItem != undefined) ? accountItem.data(AccountsModel.ConnectionStatusReasonRole) : TelepathyTypes.ConnectionStatusReasonNoneSpecified

        height: childrenRect.height + 20

        Connections {
            target: (accountItem != undefined) ? accountItem : null
            onConnectionStatusChanged: {
                accountError.connectionStatus = accountItem.data(AccountsModel.ConnectionStatusRole);
                accountError.connectionStatusReason = accountItem.data(AccountsModel.ConnectionStatusReasonRole);
            }
        }

        enabled: accountItem !== null
        visible: ((connectionStatus == TelepathyTypes.ConnectionStatusDisconnected) &&
                 ((connectionStatusReason == TelepathyTypes.ConnectionStatusReasonAuthenticationFailed) ||
                  (connectionStatusReason == TelepathyTypes.ConnectionStatusReasonNameInUse)))
                   || duplicated
        // TODO: check for another reasons
        Text {
            id: errorLabel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10

            font.pixelSize: theme_fontPixelSizeLarge
            color: theme_fontColorHighlight

            text: duplicated ?
                      qsTr("There is already an account configured using this login. Please check your username.") :
                      qsTr("Sorry, there was a problem logging in. Please check your username and password.")
        }
    }

    Grid {
        id: contentGrid
        columns: 2

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        Text {
            id: loginLabel
            anchors.margins: 10
            text: qsTr("Username:")
            verticalAlignment: Text.AlignVCenter
            font.bold: true
            height: loginBox.height
        }

        TextEntry {
            id: loginBox
            anchors.margins: 10
            width: parent.width / 2
            inputMethodHints: Qt.ImhNoAutoUppercase
            defaultText: qsTr("Name / ID")

            onTextChanged: duplicated = false
        }

        Text {
            id: passwordLabel
            anchors.margins: 10
            text: qsTr("Password:")
            verticalAlignment: Text.AlignVCenter
            font.bold: true
            height: passwordBox.height
        }

        TextEntry {
            id: passwordBox
            anchors.margins: 10
            width: parent.width / 2
            echoMode: TextInput.Password
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhHiddenText
            defaultText: qsTr("Password")
        }
    }

    BorderImage {
        id: advancedHandler
        source: "image://meegotheme/widgets/common/header/header"

        property bool expanded: false
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32

        visible: false

        Text {
            id: advancedOptionsLabel
            text: qsTr("Advanced settings")
            anchors.margins: 10
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            font.bold: true
            color: theme_fontColorHighlight
        }

        Image {
            id: arrowIcon
            source: "image://theme/panels/pnl_icn_arrow" + (advancedHandler.expanded ? "right" : "down")

            anchors.topMargin: 5
            anchors.bottomMargin: 5
            anchors.rightMargin: 15
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            smooth: true
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            onClicked: {
                advancedHandler.expanded = !advancedHandler.expanded;
            }
        }
    }

    Item {
        id: advancedOptionsContainer
        height: visible ? childrenRect.height : 0
        anchors.left: parent.left
        anchors.right: parent.right
        visible: opacity != 0
        opacity: advancedHandler.expanded ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: mainArea.animationDuration
            }
        }

        Behavior on height {
            NumberAnimation {
                duration: mainArea.animationDuration
            }
        }

        Loader {
            id: advancedOptions
            anchors.left: parent.left
            anchors.right: parent.right

        }
    }

    states: [
        State {
            name: "editState"
            when: accountId != ""
            PropertyChanges {
                target: advancedHandler
                // the advanced options are only available to some accounts
                visible: advancedOptionsComponent != null
            }

            StateChangeScript {
                name: "prepareEdit"
                script: mainArea.prepareAccountEdit();
            }
        }
    ]
}
