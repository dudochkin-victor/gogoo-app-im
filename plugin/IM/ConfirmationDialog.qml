import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.App.IM 0.1

ModalMessageBox {
    id: container

    property string mainText: ""
    property string subText: ""
    property string instanceReason: ""

    width: 420
    height: 480

    acceptButtonText: qsTr("Yes")
    acceptButtonImage: "image://meegotheme/images/btn_blue_up"
    acceptButtonImagePressed: "image://meegotheme/images/btn_blue_dn"
    cancelButtonText: qsTr("No")
    cancelButtonImage: "image://meegotheme/images/btn_red_up"
    cancelButtonImagePressed: "image://meegotheme/images/btn_red_dn"

}
