import QtQuick 2.0
import QtQuick.Controls 1.0
import videostream 1.0

Item {
    id: streamScene
    anchors.fill: parent
    property int loaderBottomMargin: 50
    property int loaderRightMargin: 50
    property bool isTransmitRequest: false
    VideoStreamer {
        id: streamWidget
        anchors.fill: parent
        visible: status === VideoStreamer.Receiving ||
                 status === VideoStreamer.Transmitting
        port: 50000
        frameFormat: "jpg"
        onStatusChanged: {
            switch(status) {
            case VideoStreamer.OutgoingConnection:
                setNotConnectedSize()
                loader.sourceComponent = stopPanel
                break;
            case VideoStreamer.Receiving:
                setConnectedSize()
                loader.sourceComponent = stopPanel
                break;
            case VideoStreamer.Transmitting:
                setConnectedSize()
//                setNotConnectedSize()
                loader.sourceComponent = stopPanel
                break;
            case VideoStreamer.NotConnected:
            case VideoStreamer.ConnectionFaild:
                setNotConnectedSize()
                loader.sourceComponent = userControlPanel
                break;
            case VideoStreamer.IncomingReseiveRequest:
            case VideoStreamer.IncomingTransmitRequest:
                setNotConnectedSize()
                loader.sourceComponent = incomingRequestHint
                break;
            }
        }
    }

    Loader {
        id: loader
        sourceComponent: userControlPanel
        anchors {
            right: parent.right
            bottom: parent.bottom
            rightMargin: loaderRightMargin
            bottomMargin: loaderBottomMargin
        }
    }

    Component {
        id: incomingRequestHint
        Item {
            width: 600
            height: 300
            Text {
                anchors.centerIn: parent
                text: streamWidget.acceptTime
                color: "white"
            }

            Button {
                id: accept
                text: "accept"
                anchors.left: parent.left
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    switch(streamWidget.status) {
                    case VideoStreamer.IncomingTransmitRequest:
                        streamWidget.acceptTransmitRequest()
                        break;
                    case VideoStreamer.IncomingReseiveRequest:
                        streamWidget.acceptReceiveRequest()
                        break;
                    }
                }
            }
            Button {
                id: deny
                text: "deny"
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    switch(streamWidget.status) {
                    case VideoStreamer.IncomingTransmitRequest:
                        streamWidget.denyTransmitRequest()
                        break;
                    case VideoStreamer.IncomingReseiveRequest:
                        streamWidget.denyReceiveRequest()
                        break;
                    }
                }
            }
        }
    }
    Component {
        id: userControlPanel
        Item {
            width: 600
            height: 300
            Button {
                id: transmitRequest
                text: "Transmit"
                anchors.left: parent.left
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    loader.sourceComponent = hostsPanel
                    isTransmitRequest = true
                }
            }
            Button {
                id: reseiveRequest
                text: "Receive"
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                onClicked: loader.sourceComponent = hostsPanel
            }
        }
    }
    Component {
        id: stopPanel
        Item {
            width: 300
            height: 100
            Button {
                id: reseiveRequest
                text: "Stop"
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    switch(streamWidget.status) {
                    case VideoStreamer.Transmitting:
                        streamWidget.stopTransmitting();
                        break;
                    case VideoStreamer.Receiving:
                        streamWidget.stopReceiving();
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    Component {
        id: hostsPanel
        Column {
            id: col
            width: 300
            Text {
                id: localName
                text: addressesModel.localName
                color: "white"
                Binding {
                    target: localName
                    property: "text"
                    value: addressesModel.localName
                }
            }
            Text {
                id: txtStatus
                text: textByStatus(streamWidget.status)
                color: "white"
            }
            ListModel {
                id: addressesModel
                ListElement {
                    name: "local_host"
                    address: "127.0.0.1"
                }
                ListElement {
                    name: "book_machine"
                    address: "192.168.0.15"
                }
            }

            ListView {
                id: addressesList
                width: parent.width
                height: 100
                model: addressesModel
                delegate: Button {
                    width: parent.width
                    height: 30
                    text: name
                    onClicked: {
                        if(isTransmitRequest)
                            streamWidget.transmittingRequest(address)
                        else
                            streamWidget.receivingRequest(address)
                    }
                }
            }
        }

    }

    function textByStatus(status) {
        switch(status) {
        case VideoStreamer.NotConnected:
            return "NOT CONNECTED"
        case VideoStreamer.OutgoingConnection:
            return "OUT CONNECTION"
        case VideoStreamer.Receiving:
            return "RECEIVE"
        case VideoStreamer.Transmitting:
            return "TRANSMIT"
        case VideoStreamer.ConnectionFaild:
            return "CONNECTION FAILD"
        case VideoStreamer.IncomingReseiveRequest:
            return "RECEIVE REQUEST"
        case VideoStreamer.IncomingTransmitRequest:
            return "TRANSMIT REQUEST"
        default:
            return "UNNOWN STATUS"
        }
    }
    function setNotConnectedSize() {
        streamScene.width = loader.width + loaderRightMargin
        streamScene.height = loader.height + loaderBottomMargin
        streamScene.anchors.rightMargin = 0
        streamScene.anchors.bottomMargin = 0
    }
    function setConnectedSize() {
        streamScene.width = streamScene.parent.width
        streamScene.height = streamScene.parent.height
        streamScene.anchors.rightMargin = 0
        streamScene.anchors.bottomMargin = 0
    }
}
