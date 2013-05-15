import Qt 4.7

Item {
    id: contactPickerPage

    height: blockedColumn.height

    signal itemSelected(string parentId, string itemId);
    signal itemDeselected(string parentId, string itemId);

    // either way, if the item is clicked, it should be unblocked
    onItemSelected: {
        accountsModel.unblockContact(parentId, itemId);
    }

    onItemDeselected: {
        accountsModel.unblockContact(parentId, itemId);
    }

    function isItemSelected(contactId)
    {
        return true;
    }

    Column {
        id: blockedColumn

        anchors.left: parent.left
        anchors.right: parent.right
        height: childrenRect.height

        Item {
            width: 10
            height: 10
        }

        ListView {
            id: blockedContactsList
            model: contactsModel
            delegate: ContactPickerDelegate {}

            height: 121 * contactsModel.rowCount
            anchors.left: parent.left
            anchors.right: parent.right
            interactive: false

            section.property: "parentDisplayName"
            section.criteria: ViewSection.FullString
            section.delegate: Image {
                width: blockedContactsList.width
                source: "image://theme/settings/subheader"
                height: childrenRect.height + 10

                Text{
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: section
                    font.pixelSize: theme_fontPixelSizeLarge
                    width: parent.width
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
