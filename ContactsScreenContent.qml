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

AppPage {
    id: contactsScreenPage

    anchors.fill: parent
    enableCustomActionMenu: true
    
    property int count: listView.count
    property int accountStatus: 0
    property bool showAccountOffline: (accountStatus == TelepathyTypes.ConnectionPresenceTypeOffline
                                       || accountStatus == TelepathyTypes.ConnectionPresenceTypeUnset
                                       || accountStatus == TelepathyTypes.ConnectionPresenceTypeUnknown
                                       || accountStatus == TelepathyTypes.ConnectionPresenceTypeError)
    property bool showAddFriends: !count && !showAccountOffline
    property int requestedStatusType: 0
    property string requestedStatus: ""
    property string requestedStatusMessage: ""

    Component.onCompleted: {
        pageTitle = window.currentAccountName;
        accountStatus = window.accountItem.data(AccountsModel.CurrentPresenceTypeRole);
    }

    onAccountStatusChanged: {
        contactsModel.filterByAccountId(currentAccountId);
        contactRequestModel.filterByAccountId(currentAccountId);
    }

    onActionMenuIconClicked: {
        contactContentMenu.setPosition( mouseX, mouseY);
        contactContentMenu.show();
    }

    Connections {
        target: window.accountItem

        onChanged: {
            accountStatus = window.accountItem.data(AccountsModel.CurrentPresenceTypeRole);
        }
    }

    Connections {
        target: window

        onCurrentAccountNameChanged: {
            pageTitle = window.currentAccountName;
        }

        onSearch: {
            contactsModel.filterByString(needle);
        }
    }

    // this connection is to handle
    Connections {
        target: confirmationDialogItem
        onAccepted: {

            var icon = window.accountItem.data(AccountsModel.IconRole);

            if (confirmationDialogItem.instanceReason != "contact-menu-single-instance") {
                return;
            }

            // if the dialog was accepted we should disconnect all other accounts
            // of the same type
            accountFactory.disconnectOtherAccounts(icon, window.currentAccountId);

            // and set the account online
            window.accountItem.setRequestedPresence(requestedStatusType, requestedStatus, requestedStatusMessage);
            window.accountItem.setAutomaticPresence(requestedStatusType, requestedStatus, requestedStatusMessage);
        }

        // no need to do anything if the dialog is rejected
        // onRejected:
    }

    Item {
        id: pageContent
        anchors.fill: parent

        NoNetworkHeader {
            id: noNetworkItem
        }

        AccountOffline {
            id: accountOfflineInfo

            anchors {
                top: noNetworkItem.bottom
                left: parent.left
                right: parent.right
            }
            visible: showAccountOffline
        }

        NoFriends {
            id: noFriendsInfo

            anchors {
                top: accountOfflineInfo.bottom
                left: parent.left
                right: parent.right
            }
            visible: showAddFriends
        }

        Component {
            id: requestsViewComponent

            ListView {
                id: requestsView
                interactive: false
                property int itemHeight: theme_commonBoxHeight;

                height: itemHeight * count

                anchors {
                    left: parent.left
                    right: parent.right
                }
                visible: listView.visible
                model: contactRequestModel
                delegate: ContactRequestDelegate {
                    itemHeight: requestsView.itemHeight
                }
            }
        }

        ListView {
            id: listView

            anchors {
                top: noFriendsInfo.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            visible: (!showAccountOffline)

            model: contactsModel
            delegate: ContactDelegate {}
            clip: true

            header: requestsViewComponent
            interactive: contentHeight > height
        }

        Title {
            id: friendsTitle
            anchors.top: noFriendsInfo.bottom
            text: qsTr("Add a friend")
            visible: showAddFriends
        }

        AddAFriend {
            id: addAFriendItem

            visible: showAddFriends
            width: 200
            anchors.top: friendsTitle.bottom
            anchors.margins: 10
            anchors.left: parent.left
        }
    }

    function hideActionMenu()
    {
        contactContentMenu.hide();
    }

    ContextMenu {
        id: contactContentMenu

        width: 200
        forceFingerMode: 2

        onVisibleChanged: {
            actionMenuOpen = visible
        }

        content: ContactContentMenu {
            currentPage: contactsScreenPage;
        }
    }
}
