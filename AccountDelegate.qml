/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1
import MeeGo.Labs.Components 0.1 as Labs
import MeeGo.App.IM 0.1
import TelepathyQML 0.1


Item {
    id: mainArea

    property int accountStatus: model.statusType

    height: childrenRect.height

    ContentRow {
        id: contentRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Image {
            id: serviceIcon
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            source: accountFactory.accountIcon(model.icon, model.connectionStatus)
            opacity: loadingIcon.visible ? 0 : 1
            smooth: true

        }

        Labs.Spinner {
            id: loadingIcon
            anchors.centerIn: serviceIcon
            visible: model.connectionStatus == TelepathyTypes.ConnectionStatusConnecting

            spinning: visible

            onSpinningChanged: {
                if(loadingIcon.visible && !spinning) {
                    spinning = true;
                }
            }
        }

        Text {
            id: accountTypeName
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: serviceIcon.right
            anchors.right: parent.right
            anchors.margins: 10
            text: model.displayName

            elide: Text.ElideRight
            font.weight: Font.Bold
            color: theme_fontColorNormal
            font.pixelSize: theme_fontPixelSizeLargest
        }

        Connections {
            target: confirmationDialogItem
            onAccepted: {
                if (confirmationDialogItem.instanceReason != "account-delegate-single-instance") {
                    return;
                }

                // if the dialog was accepted we should disconnect all other accounts
                // of the same type
                accountFactory.disconnectOtherAccounts(model.icon, model.id);

                // and set the account online
                model.item.setRequestedPresence(TelepathyTypes.ConnectionPresenceTypeAvailable,
                                                "available", // i18n ok
                                                model.item.data(AccountsModel.CurrentPresenceStatusMessageRole));
            }

            // no need to do anything if the dialog is rejected
            // onRejected:
        }

        ContextMenu {
            id: contextMenu

            // if we don't change the parent here, the maximum height of the context menu is that of the account row,
            // and not of the whole list
            parent: accountsListView
            content:  ActionMenu {
                id: actionMenu

                Labs.ApplicationsModel {
                    id: appModel
                }

                onTriggered: {
                    if (index == 0)
                    {
                        if(payload.data(AccountsModel.ConnectionStatusRole) == TelepathyTypes.ConnectionStatusConnected) {
                            payload.setRequestedPresence(TelepathyTypes.ConnectionPresenceTypeOffline,
                                                "offline", // i18n ok
                                                payload.data(AccountsModel.RequestedPresenceStatusMessageRole));
                        } else {
                            var icon = payload.data(AccountsModel.IconRole);
                            var id = payload.data(AccountsModel.IdRole);
                            var serviceName = protocolsModel.titleForId(icon);

                            // if the protocol only allows to have one account connected at a time,
                            // ask the user if he really wants to do that
                            if (protocolsModel.isSingleInstance(icon) &&
                                accountFactory.otherAccountsOnline(icon, id)) {
                                // show the dialog asking the user if he really wants to connect the account

                                confirmationDialogItem.title = qsTr("Multiple accounts connected");
                                confirmationDialogItem.mainText = qsTr("Do you really want to connect this account? By doing this all other %1 accounts will be disconnected.").arg(serviceName);
                                confirmationDialogItem.instanceReason = "account-delegate-single-instance"; // i18n ok
                                confirmationDialogItem.show();
                            } else {
                                if(payload.data(AccountsModel.AutomaticPresenceTypeRole) != TelepathyTypes.ConnectionPresenceTypeOffline) {
                                    payload.setRequestedPresence(payload.data(AccountsModel.AutomaticPresenceTypeRole),
                                                                 payload.data(AccountsModel.AutomaticPresenceRole),
                                                                 payload.data(AccountsModel.AutomaticPresenceStatusMessageRole));
                                } else {
                                    payload.setRequestedPresence(TelepathyTypes.ConnectionPresenceTypeAvailable,
                                                                 payload.data(AccountsModel.AutomaticPresenceRole),
                                                                 payload.data(AccountsModel.AutomaticPresenceStatusMessageRole));
                                }
                            }
                        }

                    }
                    else if (index == 1)
                    {
                        // Account settings
                        var cmd = "/usr/bin/meego-qml-launcher --app meego-ux-settings --opengl --fullscreen --cmd showPage --cdata \"IM\"";  //i18n ok
                        appModel.launch(cmd);
                    }
                    contextMenu.hide();
                }
            }
        }

        CallCountIcon {
            id: chatIcon

            messageCount: model.pendingMessages
            missedAudioCalls: model.missedAudioCalls
            missedVideoCalls: model.missedVideoCalls
            openChat: model.chatOpened

            anchors.margins: 5
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            visible: (model.chatOpened
                      || model.missedAudioCalls > 0
                      || model.missedVideoCalls > 0)
        }

        ListModel { id: menu}

        MouseArea {


            anchors.fill: parent
            onClicked: {
                window.currentAccountId = model.id;
                window.addPage(contactsScreenContent);
            }

            onPressAndHold: {
                menu.clear();

                if(model.connectionStatus == TelepathyTypes.ConnectionStatusConnected) {
                    menu.append({"modelData":qsTr("Log out")});
                } else {
                    menu.append({"modelData":qsTr("Log in to %1").arg(telepathyManager.accountServiceName(model.icon))})
                }
                menu.append({"modelData":qsTr("Settings")});

                var map = mapToItem(window, mouseX, mouseY);
                contextMenu.setPosition( map.x, map.y);
                actionMenu.model = menu;
                actionMenu.payload = model.item;
                contextMenu.show();

            }
        }
    }
}
