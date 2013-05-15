/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.App.IM 0.1
import TelepathyQML 0.1

Item {
    id: container

    height: (visible? panel.height : 0)
    InfoPanel {
        id: panel

        width: parent.width

        Text {
            id: accountOfflineText

            text: qsTr("Account is offline")
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            color: theme_fontColorHighlight
            font.pixelSize: theme_fontPixelSizeLarge
            font.weight: Font.Bold
        }
    }


}
