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

    property bool pending: model.transferState == TelepathyTypes.FileTransferStatePending
    property bool active: model.transferState == TelepathyTypes.FileTransferStateOpen
    property bool finished: model.transferState == TelepathyTypes.FileTransferStateCompleted
    property bool canceled: model.transferState == TelepathyTypes.FileTransferStateCancelled
    property variant item: model.item

    height: childrenRect.height + 10

    Labs.ApplicationsModel {
        id: appModel
    }

    Behavior on height {
        NumberAnimation {
            duration: 250
        }
    }

    Item {
        id: messageHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: childrenRect.height

        PresenceIcon {
            id: presence
            anchors.left: parent.left
            anchors.verticalCenter: contact.verticalCenter
            anchors.margins: 5
            anchors.leftMargin: messageTop.border.left

            status: model.status
        }

        Text {
            id: contact

            anchors.left: presence.right
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            anchors.leftMargin:5
            anchors.right: time.left
            anchors.rightMargin: 10
            color: Qt.rgba(0.3,0.3,0.3,1)
            font.pixelSize: theme_fontPixelSizeSmall
            elide: Text.ElideRight

            text: {
                if (messageSent) {
                    if (canceled) {
                        qsTr("Upload canceled:");
                    } else if (finished) {
                        qsTr("Sent:");
                    } else {
                        qsTr("Uploading:");
                    }
                } else {
                    if (finished) {
                        qsTr("%1 has sent you:").arg(model.sender);
                    } else {
                        qsTr("%1 is sending you:").arg(model.sender);
                    }
                }
            }
        }

        Text {
            id: time
            anchors.right: parent.right
            anchors.bottom: contact.bottom
            anchors.rightMargin: messageTop.border.right
            color: Qt.rgba(0.3,0.3,0.3,1)
            font.pixelSize: theme_fontPixelSizeSmall

            text: fuzzyDateTime.getFuzzy(model.dateTime)

            Connections {
                target: fuzzyDateTimeUpdater
                onTriggered: {
                    time.text = fuzzyDateTime.getFuzzy(model.dateTime);
                }
            }
        }
    }

    Item {
        id: messageBody

        anchors.top: messageHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 15
        anchors.leftMargin: messageTop.border.left
        anchors.rightMargin: messageTop.border.right

        height: childrenRect.height

        Text {
            id: fileName
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 10
            anchors.leftMargin: 10

            text: model.fileName
            color: theme_fontColorNormal
        }

        Text {
            id: fileSize
            anchors.verticalCenter: fileName.verticalCenter
            anchors.left: fileName.right
            anchors.margins: 5
            text: qsTr("(%1)").arg(model.fileSize)
            color: theme_fontColorInactive
        }

        Button {
            id: openButton
            anchors.top: fileName.bottom
            anchors.left: fileName.left
            anchors.topMargin: 10

            text: qsTr("Open")
            textColor: theme_buttonFontColor
            bgSourceUp: "image://meegotheme/widgets/common/button/button-default"
            bgSourceDn: "image://meegotheme/widgets/common/button/button-default-pressed"

            onClicked: {
                var cmd = "xdg-open \"" + model.filePath + "\"";
                appModel.launch(cmd);
            }


            visible: finished && model.incomingTransfer
        }

        Text {
            id: errorText
            anchors.top: fileName.bottom
            anchors.left: fileName.left
            anchors.topMargin: 10
            text: {
                if (canceled &&
                        (model.transferStateReason == TelepathyTypes.FileTransferStateChangeReasonRemoteError
                         || model.transferStateReason == TelepathyTypes.FileTransferStateChangeReasonLocalError)) {
                    if (model.incomingTransfer) {
                        qsTr("There was a problem downloading");
                    } else {
                        qsTr("There was a problem uploading");
                    }
                } else {
                    qsTr("Canceled") // TODO: report other errors
                }
            }
            color: theme_fontColorHighlight

            visible: canceled
        }


        Item {
            id: buttonsParent
            width: childrenRect.width
            height: visible ? childrenRect.height + 20 : 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: fileName.bottom
            anchors.margins: 10

            visible: pending && model.incomingTransfer

            Button {
                id: saveButton
                anchors.top: parent.top
                anchors.left: parent.left

                text: qsTr("Save")
                textColor: theme_buttonFontColor
                bgSourceUp: "image://meegotheme/widgets/common/button/button-default"
                bgSourceDn: "image://meegotheme/widgets/common/button/button-default-pressed"

                onClicked: item.acceptTransfer();
            }

            Button {
                id: declineButton
                anchors.top: parent.top
                anchors.leftMargin: 10
                anchors.left: saveButton.right

                text: qsTr("Decline")
                textColor: theme_buttonFontColor
                bgSourceUp: "image://meegotheme/widgets/common/button/button-negative"
                bgSourceDn: "image://meegotheme/widgets/common/button/button-negative-pressed"

                onClicked: item.cancelTransfer()
            }
        }

        Item {
            id: progressItem
            anchors.topMargin: 10
            anchors.leftMargin: 10
            anchors.rightMargin: 10

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: fileName.bottom

            visible: mainArea.active || (!model.incomingTransfer && mainArea.pending)

            height: visible ? childrenRect.height + 10 : 0

            Button {
                id: cancelButton
                anchors.top: parent.top
                anchors.right: parent.right

                text: qsTr("Cancel")
                textColor: theme_buttonFontColor
                bgSourceUp: "image://meegotheme/widgets/common/button/button-negative"
                bgSourceDn: "image://meegotheme/widgets/common/button/button-negative-pressed"

                onClicked: item.cancelTransfer();
            }

            Item {
                id: progressBar

                anchors.left: parent.left
                anchors.right: cancelButton.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 10

                BorderImage {
                    id: backgroundBar

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    border {
                        top: 2
                        bottom: 2
                        left: 2
                        right: 2
                    }

                    source: "image://meegotheme/widgets/common/progress-bar/progress-bar-backgound"
                }

                BorderImage {
                    id: foregroundBar

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    border {
                        top: 2
                        bottom: 2
                        left: 2
                        right: 2
                    }

                    width: backgroundBar.width * (model.percentTransferred / 100.)
                    source: "image://meegotheme/widgets/common/progress-bar/progress-bar-fill"

                    visible: model.percentTransferred > 0

                    Behavior on width {
                        NumberAnimation {
                            duration: 500
                        }
                    }
                }
            }
        }
    }
}
