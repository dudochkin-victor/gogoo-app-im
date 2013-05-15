/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Components 0.1

Item {
    id: accountTypeItem
    height: childrenRect.height

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        height: childrenRect.height

        Repeater {
            model: protocolsModel

            Item {
                id: accountSetupItem
                height: contentRow.childrenRect.height
                width: parent.width

                ExpandingBox {
                    id: contentRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: theme_commonBoxHeight

                    detailsComponent: accountFactory.embeddedNewAccountContent(model.id, contentRow)

                    Connections {
                        target: contentRow.detailsItem != null ? contentRow.detailsItem.accountContent : null

                        onFinished: {
                            contentRow.expanded = false
                            contentRow.detailsComponent = accountFactory.embeddedNewAccountContent(model.id, contentRow);
                        }
                    }

                    Image {
                        id: serviceIcon
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        source: model.icon
                    }

                    Text {
                        id: accountSetupLabel
                        anchors.margins: 10
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: serviceIcon.right
                        text: model.title
                        font.pixelSize: theme_fontPixelSizeLargest
                        font.bold: true
                    }
                }
            }
        }
    }
}
