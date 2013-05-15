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
    id: messageScreenPage
    anchors.fill: parent
    enableCustomActionMenu: true

    property string contactId: window.currentContactId
    property string contactName: window.contactItem.data(AccountsModel.AliasRole);
    /* Video Window position for the four corners, numbered like this:
       0 1
       2 3 */
    property int videoWindowPosition : 2
    // next variable contains target while drag and drop operation (-1 is no target)
    property int videoWindowPositionHighlight : -1
    // when true, the small video window is used for the camera video
    property bool cameraWindowSmall : true
    // videoWindowSwap is updated for drag and drop operation (not related with swap camera feature)
    property bool videoWindowSwap : false
    property bool videoWasSent : false

    Component.onCompleted: {
        if(window.chatAgent != undefined && window.chatAgent.isConference) {
            pageTitle = qsTr("Group conversation");
        } else {
            pageTitle = qsTr("Chat with %1").arg(window.contactItem.data(AccountsModel.AliasRole));
        }

        if(window.chatAgent != undefined && window.chatAgent.existsChat) {
            if(window.chatAgent.isConference) {
                conversationView.model = accountsModel.groupConversationModel(window.currentAccountId,
                                                                              window.chatAgent.channelPath);
                if (conversationView.model != undefined) {
                    window.fileTransferAgent.setModel(conversationView.model);
                }
            } else {
                conversationView.model = accountsModel.conversationModel(window.currentAccountId,
                                                                         window.currentContactId);
                if (conversationView.model != undefined) {
                    window.fileTransferAgent.setModel(conversationView.model);
                }
            }
        }

        conversationView.positionViewAtIndex(conversationView.count - 1, ListView.End);
        notificationManager.chatActive = true;
        if(window.callAgent != undefined) {
            var status = window.callAgent.callStatus;
            if (status == CallAgent.CallStatusIncomingCall || window.callAgent.existingCall) {
                videoWindow.opacity = 1;
                window.callAgent.setOutgoingVideo(cameraWindowSmall ? videoOutgoing : videoIncoming);
                window.callAgent.onOrientationChanged(window.orientation);
                window.callAgent.setIncomingVideo(cameraWindowSmall ? videoIncoming : videoOutgoing);
            }
        }

        // just to be sure, set the focus on the text editor
        textEdit.textFocus = true;
        window.callAgent.resetMissedCalls()
    }

    Component.onDestruction: {
        if(!window.chatAgent.isConference) {
            accountsModel.disconnectConversationModel(window.currentAccountId,
                                                      contactId);
        } else {
            accountsModel.disconnectGroupConversationModel(window.currentAccountId,
                                                           window.chatAgent.channelPath);
        }

        if(window.callAgent != undefined) {
            window.callAgent.setOutgoingVideo(null);
            window.callAgent.setIncomingVideo(null);
        }
        notificationManager.chatActive = false;
    }

    onActionMenuIconClicked: {
        messageContentMenu.setPosition( mouseX, mouseY);
        messageContentMenu.show();
    }

    // small trick to reload the data() role values when the item changes
    Connections {
        target: window.contactItem
        onChanged: window.contactItem = window.contactItem
    }

    Connections {
        target: accountsModel
        onChatReady:  {
            if (accountId == window.currentAccountId && contactId == window.currentContactId) {
                conversationView.model = accountsModel.conversationModel(window.currentAccountId,
                                                                         contactId);
                window.fileTransferAgent.setModel(conversationView.model);
            }
        }

        onGroupChatReady:  {
            if (accountId == window.currentAccountId && channelPath == window.chatAgent.channelPath) {
                conversationView.model = accountsModel.groupConversationModel(window.currentAccountId,
                                                                              channelPath);
                window.fileTransferAgent.setModel(conversationView.model);
            }
        }
    }

    Connections {
        target: callAgent
        onCallStatusChanged: {
            // Several sounds might play at once here. Should be prioritize, make a queue, or let them all play ?
            if (callAgent.callStatus == CallAgent.CallStatusNoCall) {
                window.fullScreen = false;
                window.fullContent = false;
                videoWindow.opacity = 0;
            }
            // activate call ringing
            if(callAgent.callStatus == CallAgent.CallStatusRinging) {
                outgoingCallSound.playSound();
            } else {
                outgoingCallSound.stopSound();
            }
            // connection established
            if (callAgent.callStatus == CallAgent.CallStatusTalking) {
                callConnectedSound.playSound();
            }
            if (callAgent.callStatus == CallAgent.CallStatusHangingUp) {
                callHangupSound.playSound();
            }
            if (callAgent.error) {
                errorSound.playSound();
                // do not clear error on purpose, so other components do not miss it;
                // we might see the error twice, and cannot really tell if it is the same error or not,
                // but we don't really care as we'd just play the error sound twice,
                // which is not a bad idea if the error state persists anyway
            }
        }
    }

    Connections {
        target: window
        onOrientationChanged: {
            window.callAgent.onOrientationChanged(window.orientation);
        }

        onSearch: {
            conversationView.model.searchByString(needle);
            searchHeader.searchActive = (needle != "");
        }
    }

    Connections {
        target: window.callAgent
        onVideoSentChanged: {
            var sent = window.callAgent.videoSentOrAboutTo;
            if (sent != videoWasSent) {
                if (sent) {
                    recordingStartSound.playSound();
                }
                else {
                    recordingStopSound.playSound();
                }
            }
            videoWasSent = sent;
        }
    }

    Item {
        id: pageContent
        parent: messageScreenPage
        anchors.fill: parent
        // if the messages roll over the main bar, uncomment this line to
        // force clipping
        //clip: true

        SearchHeader {
            id: searchHeader
            searchActive: window.showToolBarSearch
            searching: conversationView.model.searching
            olderActive : conversationView.model.olderActive
            newerActive : conversationView.model.newerActive
            numMatchesFound: conversationView.model.numMatchesFound
            onOlderClicked: {
                conversationView.model.olderMatch();
            }
            onNewerClicked: {
                conversationView.model.newerMatch();
            }
        }

        Connections {
            target: conversationView.model
            onCurrentRowMatchChanged: {
                conversationView.positionViewAtIndex(conversationView.model.currentRowMatch, ListView.Center);
            }
        }

        NoNetworkHeader {
            id: noNetworkItem
            anchors.top: searchHeader.bottom
        }

        LoadingConversationHistory {
            id: loadingConversation
            visible: historyFeeder.fetching
            z: 10
            anchors {
                top: noNetworkItem.bottom
                left: parent.left
                right: parent.right
            }
        }

        Component {
            id: sectionDateDelegate
            Item {
                width: conversationView.width
                height: 50

                Text {
                    id: dateText
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    color: theme_fontColorHighlight
                    font.pixelSize: theme_fontPixelSizeLarge
                    //verticalAlignment: Text.AlignVCenter
                    //horizontalAlignment: Text.AlignHCenter
                    text: section
                }

                Image {
                    anchors.top: dateText.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "image://meegotheme/images/dialog-separator"
                }
            }
        }

        ListView {
            id: conversationView
            anchors {
                top: loadingConversation.bottom
                left: parent.left
                right: parent.right
                bottom: window.fullScreen ? imToolBar.top : textBar.top
                margins: 10
            }

            delegate: MessageDelegate { }
            highlightFollowsCurrentItem: true
            currentIndex: count - 1
            clip: true

            section.property : "dateString"
            section.criteria : ViewSection.FullString
            section.delegate : sectionDateDelegate

            interactive: contentHeight > height

            onCountChanged: {
                textSound.playSound();
            }
        }

        /*
          Timer used to feed history from the logger at the beginning of the view
        */
        Timer {
            id: historyFeeder
            interval: 1000
            running: false
            repeat: true

            property bool fetching : false
            property int oldIndex : -1

            onTriggered: {
                if (conversationView.atYBeginning) {
                    if (!fetching && conversationView.model.canFetchMoreBack()) {
                        fetching = true;
                        oldIndex = 0;
                        conversationView.model.fetchMoreBack();
                    }
                }
            }
        }

        Connections {
            target: typeof(conversationView.model) != 'undefined' ? conversationView.model : null
            ignoreUnknownSignals: true
            onBackFetchable: {
                if (conversationView.model.canFetchMoreBack()) {
                    historyFeeder.running = true;
                }
            }
            onBackFetched: {
                historyFeeder.fetching = false;
                if (historyFeeder.oldIndex != -1) {
                    conversationView.positionViewAtIndex(historyFeeder.oldIndex + numItems, ListView.Beginning);
                } else {
                    conversationView.positionViewAtIndex(0, ListView.End);
                }

                if (!conversationView.model.canFetchMoreBack()) {
                    historyFeeder.running = false;
                }
            }
        }

        IMSound {
            id: textSound
            soundSource: "/usr/share/sounds/meego/stereo/chat-fg.wav"
        }

        IMSound {
            id: outgoingCallSound
            soundSource: "/usr/share/sounds/meego/stereo/ring-4.wav"
            repeat: true
        }

        IMSound {
            id: callConnectedSound
            soundSource: "/usr/share/sounds/meego/stereo/connect.wav"
        }

        IMSound {
            id: callHangupSound
            soundSource: "/usr/share/sounds/meego/stereo/disconnect.wav"
        }

        IMSound {
            id: recordingStartSound
            soundSource: "/usr/share/sounds/meego/stereo/rec-start.wav"
        }

        IMSound {
            id: recordingStopSound
            soundSource: "/usr/share/sounds/meego/stereo/rec-stop.wav"
        }

        IMSound {
            id: errorSound
            soundSource: "/usr/share/sounds/meego/stereo/error.wav"
        }

        Image {
            id: textBar
            source: "image://meegotheme/widgets/common/action-bar/action-bar-background"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: imToolBar.top
            height: textBox.height
            visible: !window.fullScreen

            Item {
                id: textBox
                anchors.left: parent.left
                anchors.right: sendMessageButton.left
                anchors.rightMargin: 5

                height: textEdit.height + 2 * textEdit.anchors.margins

                TextField {
                    id: textEdit
                    anchors.fill: parent
                    anchors.margins: 10
                    textFormat: Text.RichText
                    font.pixelSize: theme_fontPixelSizeLarge
                    height: 34
                    //wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFocus: true
                    Keys.onEnterPressed: {
                        if(parseChatText(textEdit.text) != "") {
                            conversationView.model.sendMessage(parseChatText(textEdit.text));
                            textEdit.text = "";
                        }
                    }
                    Keys.onReturnPressed: {
                        if(parseChatText(textEdit.text) != "") {
                            conversationView.model.sendMessage(parseChatText(textEdit.text));
                            textEdit.text = "";
                        }
                    }

                    function isEmpty() {
                        var parsedText = messageScreenPage.parseChatText(textEdit.text);
                        return (parsedText.trim() == "");
                    }
                }
            }

            Button {
                id: sendMessageButton
                visible: !window.fullScreen
                anchors {
                    margins: 10
                    right: parent.right
                    verticalCenter: textBox.verticalCenter
                    topMargin: 0
                }
                height: 40
                text: qsTr("Send")
                textColor: theme_buttonFontColor

                onClicked: {
                    if(!textEdit.isEmpty()) {
                        conversationView.model.sendMessage(parseChatText(textEdit.text));
                        textEdit.text = "";
                    }
                }
            }
        }

        Item {
            id: videoWindow
            anchors.right: parent.right
            anchors.rightMargin: window.fullScreen ? 0 : 20
            anchors.top: parent.top
            anchors.topMargin: window.fullScreen ? 0 : 20
            width: getVideoWidth(window, pageContent)
            height: getVideoHeight(window, pageContent)
            opacity: 0

            function getVideoWidth(full, window) {
                if (full.fullScreen) {
                    if (window.orientation == 1 || window.orientation == 3) {
                        return window.height;
                    }
                    return window.width;
                }
                var cameraAspectRatio = messageScreenPage.getCameraAspectRatio();
                var width = window.width * 0.4;
                var height = window.height * 0.4;
                if (width / height > cameraAspectRatio) {
                    width = height * cameraAspectRatio;
                }
                return width;
            }

            function getVideoHeight(full, window) {
                if (full.fullScreen) {
                    if (window.orientation == 1 || window.orientation == 3) {
                        return window.width - imToolBar.height;
                    }
                    return window.height - imToolBar.height;
                }
                var cameraAspectRatio = messageScreenPage.getCameraAspectRatio();
                var width = window.width * 0.4;
                var height = window.height * 0.4;
                if (width / height < cameraAspectRatio) {
                    height = width / cameraAspectRatio;
                }
                return height;
            }

            Behavior on anchors.rightMargin {
                NumberAnimation {
                    duration: 500
                }
            }

            Behavior on anchors.topMargin {
                NumberAnimation {
                    duration: 500
                }
            }

            Behavior on width {
                NumberAnimation {
                    duration: 500
                }
            }

            Behavior on height {
                NumberAnimation {
                    duration: 500
                }
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: 500
                }
            }

            property bool showCameraVideo : true

            states: [
                State {
                    name: "fullscreen"
                    when: window.fullScreen
                    PropertyChanges {
                        target: window
                        showToolBarSearch: false
                        fullContent: true
                    }
                }
            ]

            Rectangle {
                anchors.fill: parent
                color: "#e6e6e6"
                
                BorderImage {
                    source: "image://themedimage/widgets/common/menu/menu-background-shadow"
                    anchors.margins: -4
                    anchors.fill: parent
                    border.left: 11
                    border.top: 11
                    border.bottom: 11
                    border.right: 11
                    visible: !scene.fullscreen
                }
            }

            VideoItem {
                id: videoIncoming
                size: Qt.size(parent.width, parent.height)
            }

            Avatar {
                id: avatar
                visible: (window.callAgent != undefined && !window.callAgent.remoteVideoRender)
                active: (window.callAgent != undefined && window.callAgent.existingCall)
                source: window.contactItem.data(AccountsModel.AvatarRole)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height / 3
            }

            Item {
                id: videoOutgoingContainer
                /*
                x: videoAtRight() ? parent.width - width - 20 : 20
                y: videoAtBottom() ? parent.height - height - 20 : 20
                */
                anchors.bottom: videoAtBottom() ? parent.bottom : undefined
                anchors.bottomMargin: 20
                anchors.top: !videoAtBottom() ? parent.top : undefined
                anchors.topMargin: 20
                anchors.left: !videoAtRight() ? parent.left : undefined
                anchors.leftMargin: 20
                anchors.right: videoAtRight() ? parent.right: undefined
                anchors.rightMargin: 20
                //width: 176//160
                //height: 144//140
                //width: videoWindow.width / 4
                //height: videoWindow.height / 4
                width: getVideoWidth(window, videoWindow)
                height: getVideoHeight(window, videoWindow)
                visible : videoWindow.showCameraVideo

                function getVideoWidth(full, window) {
                    if (!window.fullScreen) {
                        return window.width / 4.0;
                    }
                    var cameraAspectRatio = messageScreenPage.getCameraAspectRatio();
                    var width = window.width / 4.0;
                    if (window.width / window.height > cameraAspectRatio) {
                        width = window.height / 4.0 * cameraAspectRatio;
                    }
                    return width;
                }

                function getVideoHeight(full, window) {
                    if (!window.fullScreen) {
                        return window.height / 4.0;
                    }
                    var cameraAspectRatio = messageScreenPage.getCameraAspectRatio();
                    var height = window.height / 4.0;
                    if (window.width / window.height < cameraAspectRatio) {
                        height = window.width / (cameraAspectRatio * 4.0);
                    }
                    return height;
                }

                Rectangle {
                    anchors.margins: -3
                    anchors.fill: parent
                    color: "black"
                }

                VideoItem {
                    id: videoOutgoing
                    size: Qt.size(parent.width, parent.height)
                }

                MouseArea {
                    id: videoOutgoingDAD
                    anchors.fill: parent
                    drag.target: videoOutgoingContainer
                    drag.axis: Drag.XandYAxis
                    drag.minimumX: 0
                    drag.minimumY: 0
                    drag.maximumX: parent.parent.width - parent.width
                    drag.maximumY: parent.parent.height - parent.height
                    onPressed: {
                        videoWindowPositionHighlight = -1;
                        videoWindowSwap = false;
                        videoOutgoingContainer.anchors.left = undefined;
                        videoOutgoingContainer.anchors.right = undefined;
                        videoOutgoingContainer.anchors.bottom = undefined;
                        videoOutgoingContainer.anchors.top = undefined;
                    }
                    onMousePositionChanged: {
                        var tmppos = videoOutgoingDAD.mapToItem(window, mouseX, mouseY)
                        if (containedIn(tmppos.x, tmppos.y, moveMyVideoTargetTopLeftDropZone)) {
                            videoWindowSwap = false;
                            videoWindowPositionHighlight = 0;
                        } else if (containedIn(tmppos.x, tmppos.y, moveMyVideoTargetTopRightDropZone)) {
                            videoWindowSwap = false;
                            videoWindowPositionHighlight = 1;
                        } else if (containedIn(tmppos.x, tmppos.y, moveMyVideoTargetBottomLeftDropZone)) {
                            videoWindowSwap = false;
                            videoWindowPositionHighlight = 2;
                        } else if (containedIn(tmppos.x, tmppos.y, moveMyVideoTargetBottomRightDropZone)) {
                            videoWindowSwap = false;
                            videoWindowPositionHighlight = 3;
                        } else if (containedIn(tmppos.x, tmppos.y, moveMyVideoTargetCenterDropZone)) {
                            videoWindowSwap = true;
                            videoWindowPositionHighlight = -1;
                        } else {
                            videoWindowSwap = false;
                            videoWindowPositionHighlight = -1;
                        }
                    }
                    onReleased: {
                        if (videoWindowSwap && window.callAgent.canSwapVideos()) {
                            cameraWindowSmall  = !cameraWindowSmall;
                            videoWindowSwap = false;
                            window.callAgent.setOutgoingVideo(cameraWindowSmall ? videoOutgoing : videoIncoming);
                            window.callAgent.setIncomingVideo(cameraWindowSmall ? videoIncoming : videoOutgoing);
                        }

                        if (videoWindowPositionHighlight != -1) {
                            videoWindowPosition = videoWindowPositionHighlight;
                            videoWindowPositionHighlight = -1;
                        }

                        // set the anchors
                        if (videoAtBottom()) {
                            videoOutgoingContainer.anchors.bottom = videoOutgoingContainer.parent.bottom;
                        } else {
                            videoOutgoingContainer.anchors.top = videoOutgoingContainer.parent.top;
                        }
                        if (videoAtRight()) {
                            videoOutgoingContainer.anchors.right = videoOutgoingContainer.parent.right;
                        } else {
                            videoOutgoingContainer.anchors.left = videoOutgoingContainer.parent.left;
                        }
                    }

                    function containedIn(x, y, obj) {
                        var tmppos = obj.mapToItem(window, 0, 0);
                        if (x >= tmppos.x && x <= (tmppos.x + obj.width) &&
                            y >= tmppos.y && y <= (tmppos.y + obj.height)) {
                            return true;
                        }
                        return false;
                    }
                }
            }

            Image {
                id: moveMyVideoTargetTopLeft
                source: videoWindowPositionHighlight == 0 ?
                            "image://meegotheme/widgets/apps/chat/move-video-background-highlight" :
                            "image://meegotheme/widgets/apps/chat/move-video-background"
                width: videoOutgoingContainer.getVideoWidth(window, videoWindow)
                height: videoOutgoingContainer.getVideoHeight(window, videoWindow)
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.left: parent.left
                anchors.leftMargin: 20
                visible: videoOutgoingDAD.drag.active

                Item {
                    id: moveMyVideoTargetTopLeftDropZone
                    anchors.fill: parent
                    anchors.margins: -40
                    enabled: videoOutgoingDAD.drag.active
                }
            }

            Image {
                id: moveMyVideoTargetTopRight
                source: videoWindowPositionHighlight == 1 ?
                            "image://meegotheme/widgets/apps/chat/move-video-background-highlight" :
                            "image://meegotheme/widgets/apps/chat/move-video-background"
                width: videoOutgoingContainer.getVideoWidth(window, videoWindow)
                height: videoOutgoingContainer.getVideoHeight(window, videoWindow)
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.right: parent.right
                anchors.rightMargin: 20
                visible: videoOutgoingDAD.drag.active

                Item {
                    id: moveMyVideoTargetTopRightDropZone
                    anchors.fill: parent
                    anchors.margins: -20
                    enabled: videoOutgoingDAD.drag.active
                }
            }

            Image {
                id: moveMyVideoTargetBottomLeft
                source: videoWindowPositionHighlight == 2 ?
                            "image://meegotheme/widgets/apps/chat/move-video-background-highlight" :
                            "image://meegotheme/widgets/apps/chat/move-video-background"
                width: videoOutgoingContainer.getVideoWidth(window, videoWindow)
                height: videoOutgoingContainer.getVideoHeight(window, videoWindow)
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.left: parent.left
                anchors.leftMargin: 20
                visible: videoOutgoingDAD.drag.active

                Item {
                    id: moveMyVideoTargetBottomLeftDropZone
                    anchors.fill: parent
                    anchors.margins: -20
                    enabled: videoOutgoingDAD.drag.active
                }
            }

            Image {
                id: moveMyVideoTargetBottomRight
                source: videoWindowPositionHighlight == 3 ?
                            "image://meegotheme/widgets/apps/chat/move-video-background-highlight" :
                            "image://meegotheme/widgets/apps/chat/move-video-background"
                width: videoOutgoingContainer.getVideoWidth(window, videoWindow)
                height: videoOutgoingContainer.getVideoHeight(window, videoWindow)
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.right: parent.right
                anchors.rightMargin: 20
                visible: videoOutgoingDAD.drag.active

                Item {
                    id: moveMyVideoTargetBottomRightDropZone
                    anchors.fill: parent
                    anchors.margins: -20
                    enabled: videoOutgoingDAD.drag.active
                }
            }

            Image {
                id: moveMyVideoTargetCenter
                source: videoWindowSwap ?
                            "image://meegotheme/widgets/apps/chat/move-video-background-highlight" :
                            "image://meegotheme/widgets/apps/chat/move-video-background"
                anchors.fill: avatar
                visible: videoOutgoingDAD.drag.active

                Item {
                    id: moveMyVideoTargetCenterDropZone
                    anchors.fill: parent
                    anchors.margins: -20
                    enabled: videoOutgoingDAD.drag.active
                }
            }

            IconButton {
                id: videoOutInfo

                visible: window.callAgent != undefined
                anchors.bottom: videoOutgoingContainer.bottom
                anchors.left: videoOutgoingContainer.left
                icon: "image://meegotheme/widgets/common/button/button-info"
                iconDown: icon + "-pressed"
                width: 28
                height: 44
                hasBackground: false
                onClicked: {
                    var map = mapToItem(window, 0, 0);
                    var menu;
                    var op1 = videoWindow.showCameraVideo ? qsTr("Minimize me") : qsTr("Maximize me");
                    var op2 = window.callAgent.videoSentOrAboutTo ? qsTr("Disable camera") : qsTr("Enable camera");
                    var op3 = window.callAgent.cameraSwappable() ? qsTr("Swap camera") : null;
                    if (op3 == null) {
                        menu = [op1, op2];
                    } else {
                        menu = [op1, op2, op3]
                    }

                    actionMenu.model = menu;
                    actionMenu.payload = videoWindow;
                    contextMenu.setPosition(map.x, map.y);
                    contextMenu.show();
                }
            }
        }

        IMToolBar {
            id: imToolBar
            parent: pageContent

            onChatTextEnterPressed: {
                if(textEdit.text != "") {
                    conversationView.model.sendMessage(parseChatText(textEdit.text));
                    textEdit.text = "";
                }
            }

            onSmileyClicked: {
                // save the cursor position
                var position = textEdit.cursorPosition;
                textEdit.text = textEdit.text + "<img src=\"" + sourceName + "\" >";

                // give the focus back to the text editor
                textEdit.textFocus = true;
                textEdit.cursorPosition = position + 1;
            }
        }
    }

    function parseChatText(message) {
        var parsedMessage;

        // first remove the head
        var index = message.indexOf("</head>");
        parsedMessage = message.substr(index + 8, message.length);

        // remove the body tag
        index = parsedMessage.indexOf(">");
        parsedMessage = parsedMessage.substr(index + 1, message.length);

        // remove the end body tag
        index = parsedMessage.indexOf("</body>");
        parsedMessage = parsedMessage.substr(0, index);

        // remove paragraph tag
        index = parsedMessage.indexOf(">");
        parsedMessage = parsedMessage.substr(index + 1, message.length);

        //remove end paragraph tag
        index = parsedMessage.indexOf("</p>");
        parsedMessage = parsedMessage.substr(0, index);

        //recurse as long as there is an image to replace

        while(parsedMessage.indexOf("<img") > -1) {
            var imgIndex = parsedMessage.indexOf("<img");
            var srcIndex = parsedMessage.indexOf("src");
            var endIndex = parsedMessage.indexOf("/>");

            var emoticonName = parsedMessage.substr(srcIndex + 5, parsedMessage.length);
            emoticonName = emoticonName.substr(0, emoticonName.indexOf("/>") - 6);

            var nameIndex = emoticonName.indexOf("emote-");
            emoticonName = emoticonName.substr(nameIndex + 6, emoticonName.length);

            //replace the image name with ascii chars
            var asciiEmo;
            if(emoticonName == "angry") {
                asciiEmo = ":-&";
            } else if (emoticonName == "confused") {
                asciiEmo = ":-S";
            } else if (emoticonName == "embarressed") {
                asciiEmo = ":-[";
            } else if (emoticonName == "happy") {
                asciiEmo = ":-)";
            } else if (emoticonName == "love") {
                asciiEmo = "<3";
            } else if (emoticonName == "sad") {
                asciiEmo = ":'(";
            } else if (emoticonName == "star") {
                asciiEmo = "(*)";
            } else if (emoticonName == "tired") {
                asciiEmo = "|-(";
            } else if (emoticonName == "wink") {
                asciiEmo = ";-)";
            }

            parsedMessage = parsedMessage.substr(0, imgIndex) + asciiEmo + parsedMessage.substr(endIndex + 2);
        }

        while(parsedMessage.indexOf("&lt;") > -1) {
            var ltIndex = parsedMessage.indexOf("&lt;");
            parsedMessage = parsedMessage.substr(0, ltIndex) + "<" + parsedMessage.substr(ltIndex + 4);
        }

        while(parsedMessage.indexOf("&gt;") > -1) {
            var ltIndex = parsedMessage.indexOf("&gt;");
            parsedMessage = parsedMessage.substr(0, ltIndex) + ">" + parsedMessage.substr(ltIndex + 4);
        }

        while(parsedMessage.indexOf("&amp;") > -1) {
            var ltIndex = parsedMessage.indexOf("&amp;");
            parsedMessage = parsedMessage.substr(0, ltIndex) + "&" + parsedMessage.substr(ltIndex + 5);
        }

        while(parsedMessage.indexOf("&quot;") > -1) {
            var ltIndex = parsedMessage.indexOf("&quot;");
            parsedMessage = parsedMessage.substr(0, ltIndex) + "\"" + parsedMessage.substr(ltIndex + 6);
        }

        return parsedMessage;
    }

    function hideActionMenu()
    {
        messageContentMenu.hide();
    }

    function videoAtBottom() {
        return videoWindowPosition & 2;
    }

    function videoAtRight() {
        return videoWindowPosition & 1;
    }

    function closeConversation()
    {
        // assuming we need to end the chat session when close is pressed
        if(window.chatAgent.isConference) {
            accountsModel.endChat(window.currentAccountId, window.chatAgent.channelPath);
        } else {
            accountsModel.endChat(window.currentAccountId, contactId);
        }
        window.popPage();
    }

    function getCameraAspectRatio() {
        //var cameraAspectRatio = 4.0 / 3.0;
        //var cameraAspectRatio = 352.0 / 288.0;
        var cameraAspectRatio = 320.0 / 240.0;
        if (window.orientation == 0 || window.orientation == 2) {
            cameraAspectRatio = 1.0 / cameraAspectRatio;
        }
        return cameraAspectRatio;
    }

    ContextMenu {
        id: messageContentMenu

        width: 200
        forceFingerMode: 2

        onVisibleChanged: {
            actionMenuOpen = visible
        }

        content: MessageContentMenu {
            currentPage: messageScreenPage;
        }
    }

    ContextMenu {
        id: contextMenu
        width: 350
        content: ActionMenu {
            id: actionMenu
            onTriggered: {
                if (index == 0) {
                    //videoWindow.showCameraVideo = !videoWindow.showCameraVideo
                    payload.showCameraVideo = !payload.showCameraVideo
                } else if (index == 1) {
                    window.callAgent.videoSent = !window.callAgent.videoSentOrAboutTo;
                } else if (index == 2) {
                    window.callAgent.swapCamera();
                }

                // By setting the sourceComponent of the loader to undefined,
                // then the QML engine will destruct the context menu element
                // much like doing a c++ delete
                contextMenu.hide();
            }
        }
    }

}
