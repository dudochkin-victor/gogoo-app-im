/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.App.IM 0.1

Item {
    id: mainArea

    width: parent.width
    height: childrenRect.height

    anchors.leftMargin: 10
    anchors.rightMargin: 10
    anchors.topMargin: 5

    property bool eventItem: model.eventType == "Tpy::CustomEventItem"
    property bool fileTransferItem: model.eventType == "FileTransferItem"
    property bool messageItem: model.eventType == "Tpy::TextEventItem"
    property bool callItem: model.eventType == "Tpy::CallEventItem"
    property bool messageSent: !model.incomingEvent
    property string color: model.bubbleColor

    Component.onCompleted: {
        console.log("-------------------------------------------------------")
        console.log("model.eventType=" + model.eventType);
        console.log("model.incomingEvent=" + model.incomingEvent);
        console.log("model.sender=" + model.sender);
        console.log("model.senderAvatar=" + model.senderAvatar);
        console.log("model.receiver=" + model.receiver);
        console.log("model.receiverAvatar=" + model.receiverAvatar);
        console.log("model.dateTime=" + model.dateTime);
        console.log("model.item=" + model.item);
        console.log("model.dateTime=" + model.dateTime);
        console.log("model.messageText=" + model.messageText);
        console.log("model.messageType=" + model.messageType);
        console.log("model.callDuration=" + model.callDuration);
        console.log("model.callEndActor=" + model.callEndActor);
        console.log("model.callEndActorAvatar=" + model.callEndActorAvatar);
        console.log("model.callEndReason=" + model.callEndReason);
        console.log("model.callDetailedEndReason=" + model.callDetailedEndReason);
        console.log("model.missedCall=" + model.missedCall);
        console.log("model.rejectedCall=" + model.rejectedCall);
        console.log("model.customEventText=" + model.customEventText);
        console.log("model.customEventType=" + model.customEventType);
        console.log("callItem=" + callItem);
    }

    function messageAvatar() {
        var avatar = "";
        if (messageSent) {
            avatar = "image://avatars/" + window.currentAccountId + // i18n ok
                        "?" + accountFactory.avatarSerial;
        } else {
            avatar = model.senderAvatar;
        }

        return avatar;
    }

    Avatar {
        id: avatar
        visible: fileTransferItem || messageItem
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 8
        anchors.leftMargin: 5
        height: visible ? 100 : 0
        source: messageAvatar()

    }

    Item {
        id: messageBubble
        anchors.top: parent.top
        anchors.left: avatar.right
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.bottomMargin: 5
        anchors.leftMargin: -19
        anchors.rightMargin: 0
        smooth: true
        visible: messageItem || fileTransferItem

        height: visible ? Math.max(childrenRect.height, avatar.height) : 0

        BorderImage {
            id: messageTop
            source: "image://meegotheme/widgets/apps/chat/bubble-" + color + "-top"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            border.left: 40
            border.right: 10
            border.top: 5
        }

        BorderImage {
            id: messageCenter
            source: "image://meegotheme/widgets/apps/chat/bubble-" + color + "-middle"
            border.left: messageTop.border.left
            border.right: messageTop.border.right

            anchors.top: messageTop.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: messageBottom.top
        }

        BorderImage {
            id: messageBottom
            source: "image://meegotheme/widgets/apps/chat/bubble-" + color + "-bottom"
            border.left: messageTop.border.left
            border.right: messageTop.border.right
            border.bottom: 5

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
        }

        Item {
            id: textMessage
            visible: messageItem
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: visible ? childrenRect.height + 10 : 0

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

                    text: model.sender
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

            Text {
                id: messageBody
                anchors.top: messageHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 15
                anchors.leftMargin: messageTop.border.left
                anchors.rightMargin: messageTop.border.right

                text: parseChatText(model.messageText)
                wrapMode: Text.WordWrap
                textFormat: Text.RichText

                color: model.fromLogger ? theme_fontColorInactive : theme_fontColorNormal
            }
        }

        FileTransferDelegate {
            id: fileTransferMessage
            visible: fileTransferItem
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            height: visible ? childrenRect.height : 0
        }
    }

    Item {
        id: eventMessage
        visible: eventItem
        width: parent.width
        height: visible ? childrenRect.height : 0
        anchors.margins: 10

        Text {
            id: eventMessageText
            horizontalAlignment: Text.AlignHCenter
            color: theme_fontColorInactive
            font.pixelSize: theme_fontPixelSizeSmall
            // i18n: the first argument is the event itself, the second one is the fuzzy time
            text: qsTr("%1 - %2").arg(model.customEventText)
                                 .arg(fuzzyDateTime.getFuzzy(model.dateTime))
            wrapMode: Text.WordWrap

            Connections {
                target: fuzzyDateTimeUpdater
                onTriggered: {
                    eventMessageText.text = qsTr("%1 - %2").arg(model.customEventText)
                                                           .arg(fuzzyDateTime.getFuzzy(model.dateTime));
                }
            }
        }
    }

    Item {
        id: callMessage
        visible: callItem
        width: parent.width
        height: visible ? 30 : 0

        Image {
            id: callIcon
            anchors.right: callMessageText.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 20
            //source: "image://meegotheme/widgets/apps/chat/call-video-missed
            source: "image://meegotheme/widgets/apps/chat/call-audio-missed"
            visible: callItem && (model.missedCall || model.rejectedCall)
        }

        Text {
            id: callMessageText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: theme_fontColorInactive
            font.pixelSize: theme_fontPixelSizeSmall
            text: getCallMessageText()
            wrapMode: Text.WordWrap

            Connections {
                target: fuzzyDateTimeUpdater
                onTriggered: {
                    callMessageText.text = callMessageText.getCallMessageText();
                }
            }

            function getCallMessageText() {
                if (model.missedCall) {
                    return qsTr("%1 tried to call - %2").arg(model.sender).arg(fuzzyDateTime.getFuzzy(model.dateTime));
                } else if (model.rejectedCall) {
                    return qsTr("%1 rejected call - %2").arg(model.sender).arg(fuzzyDateTime.getFuzzy(model.dateTime));
                } else {
                    return qsTr("%1 called - duration %2 - %3").arg(model.sender).arg("" + model.callDuration).arg(fuzzyDateTime.getFuzzy(model.dateTime));
                }
            }
        }
    }

    function parseChatText(message) {
        var parsedMessage = message;

        //recurse as long as there is an image to replace
        while(parsedMessage.indexOf(":-{") > -1) {
            var index = parsedMessage.indexOf(":-{");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-angry.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":-&amp;") > -1) {
            var index = parsedMessage.indexOf(":-&amp;");
            var endIndex = index + 7;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-angry.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":S") > -1) {
            var index = parsedMessage.indexOf(":S");
            var endIndex = index + 2;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-confused.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":-S") > -1) {
            var index = parsedMessage.indexOf(":-S");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-confused.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":-[") > -1) {
            var index = parsedMessage.indexOf(":-[");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-embarressed.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":)") > -1) {
            var index = parsedMessage.indexOf(":)");
            var endIndex = index + 2;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-happy.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":-)") > -1) {
            var index = parsedMessage.indexOf(":-)");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-happy.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf("&lt;3") > -1) {
            var index = parsedMessage.indexOf("&lt;3");
            var endIndex = index + 5;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-love.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":(") > -1) {
            var index = parsedMessage.indexOf(":(");
            var endIndex = index + 2;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-sad.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(":'(") > -1) {
            var index = parsedMessage.indexOf(":'(");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-sad.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf("(*)") > -1) {
            var index = parsedMessage.indexOf("(*)");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-star.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf("|-(") > -1) {
            var index = parsedMessage.indexOf("|-(");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-tired.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(";)") > -1) {
            var index = parsedMessage.indexOf(";)");
            var endIndex = index + 2;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-wink.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        while(parsedMessage.indexOf(";-)") > -1) {
            var index = parsedMessage.indexOf(";-)");
            var endIndex = index + 3;

            var emoticonSource = "/usr/share/themes/" + theme_name + "/icons/emotes/emote-wink.png"
            var imageTag = imageTagging(emoticonSource);
            parsedMessage = parsedMessage.substr(0, index) + imageTag + parsedMessage.substr(endIndex);
        }

        return parsedMessage;
    }

    function imageTagging(sourceName) {
        return "<img src=\"" + sourceName + "\" >"
    }
}
