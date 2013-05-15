import Qt 4.7

Image {
    source: (header ? "image://meegotheme/widgets/common/menu/menu-item-separator-header" :
                      "image://meegotheme/widgets/common/menu/menu-item-separator")
    anchors.left: parent.left
    anchors.right: parent.right

    property bool header: false
}
