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
import MeeGo.Settings 0.1
import MeeGo.App.IM 0.1

Labs.ApplicationPage {
    id: container
    //pageTitle: qsTr("Instant Messaging Settings")
    title: qsTr("Instant Messaging Settings")
    anchors.fill: parent

    //property alias window: scene

    Connections {
        target: accountsModel
        onComponentsLoaded: {
            contactsModel.setBlockedOnly(true);
        }
    }

    Translator {
        catalog: "meego-app-im"
    }

    AccountContentFactory {
        id: accountFactory
    }

    // the account setup page
    Component {
        id: accountSetupComponent
        AppPage {
            id: accountSetupPage
            anchors.fill: parent

            Flickable {
                id: accountSetupArea
                parent: accountSetupPage.content
                anchors.fill: parent
                clip: true

                flickableDirection: "VerticalFlick"
                contentHeight: accountSetupItem.height

                AccountSetupContent {
                    id: accountSetupItem
                    anchors.left: parent.left
                    anchors.right: parent.right
                }
            }
        }
    }

    ConfirmationDialog {
        id: confirmationDialogItem
    }

    Flickable {
        parent: container.content
        anchors.fill: parent
        flickableDirection: Flickable.VerticalFlick
        contentHeight: contentColumn.height
        clip: true

        interactive: contentHeight > height

        Column {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            height: childrenRect.height

            Image {
                id: accountSettingsLabel
                width: parent.width
                source: "image://theme/settings/subheader"

                Text{
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: qsTr("Accounts");
                    font.pixelSize: theme_fontPixelSizeLarge
                    height: parent.height
                    width: parent.width
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Repeater {
                    id: accountsView
                    model: accountsSortedModel

                    AccountSetupDelegate {
                        parent: contentColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                    }
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: childrenRect.height + 20
                Button {
                    id: addAccountButton
                    anchors.centerIn: parent

                    text: qsTr("Add another account")
                    textColor: theme_buttonFontColor
                    bgSourceUp: "image://meegotheme/widgets/common/button/button-default"
                    bgSourceDn: "image://meegotheme/widgets/common/button/button-default-pressed"

                    onClicked: window.addPage(accountSetupComponent)
                }
            }

            Image {
                id: generalSettingsLabel
                width: parent.width
                source: "image://theme/settings/subheader"

                Text{
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: qsTr("General Settings");
                    font.pixelSize: theme_fontPixelSizeLarge
                    height: parent.height
                    width: parent.width
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                width: 10
                height: 10
            }

            Item {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                height: childrenRect.height

                Text {
                    anchors.left: parent.left
                    text: qsTr("Show offline contacts")
                    font.pixelSize: theme_fontPixelSizeLarge
                }

                ToggleButton {
                    id: offlineContactsToggle
                    on: settingsHelper.showOfflineContacts
                    onToggled: settingsHelper.showOfflineContacts = isOn;
                    anchors.margins: 10
                    anchors.right: parent.right
                }
            }

            Item {
                width: 10
                height: 10
            }


            /*Text {
                    anchors.margins: 10
                    text: qsTr("Audio alert on new message")
                    font.pixelSize: theme_fontPixelSizeLarge
                }

                ToggleButton {
                    id: audioAlertToggle
                    on: settingsHelper.enableAudioAlerts
                    onToggled: settingsHelper.enableAudioAlerts = isOn;
                }*/

            Item {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                height: childrenRect.height

                Text {
                    anchors.left: parent.left
                    text: qsTr("Notification on new message")
                    font.pixelSize: theme_fontPixelSizeLarge
                }

                ToggleButton {
                    id: notificationToggle
                    on: settingsHelper.enableNotifications;
                    onToggled: settingsHelper.enableNotifications = isOn;
                    anchors.margins: 10
                    anchors.right: parent.right
                }
            }

                /*Text {
                    anchors.margins: 10
                    text: qsTr("Vibrate on new message")
                    font.pixelSize: theme_fontPixelSizeLarge
                }

                ToggleButton {
                    id: vibrateToggle
                    on: settingsHelper.enableVibrate
                    onToggled: settingsHelper.enableVibrate = isOn;
                }*/

            Item {
                width: 10
                height: 10
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: childrenRect.height + 20
                Button {
                    id: clearHistoryButton
                    anchors.centerIn: parent

                    text: qsTr("Clear chat history")
                    textColor: theme_buttonFontColor
                    bgSourceUp: "image://meegotheme/widgets/common/button/button-default"
                    bgSourceDn: "image://meegotheme/widgets/common/button/button-default-pressed"

                    onClicked: accountsModel.clearHistory();
                }
            }

            Item {
                width: 10
                height: 10
            }

            Image {
                id: blockedContactsLabel
                width: parent.width
                source: "image://theme/settings/subheader"

                visible: contactsModel.rowCount > 0

                Text{
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: qsTr("Blocked contacts");
                    font.pixelSize: theme_fontPixelSizeLarge
                    height: parent.height
                    width: parent.width
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                id: blockSpacer
                width: 10
                height: 10
            }

            ContactPickerView {
                id: blockedList
                anchors.margins: 10
                width: parent.width
                visible: contactsModel.rowCount > 0
            }

            Item {
                width: 10
                height: 10
                visible: contactsModel.rowCount > 0
            }
        }
    }
}
