import QtQuick 2.6
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

ApplicationWindow {
    id: mainWindow
    visible: true

    minimumWidth: 1000
    minimumHeight: 550

    title: qsTr("TouhouSatsu") + "      " + Sanguosha.getVersionNumber()

    menuBar: MenuBar {
        Menu {
            title: qsTr("Game")

            MenuItem {
                text: qsTr("Start game")
                shortcut: "Ctrl+G"
                onTriggered: mainWindow.startGame();
            }

            MenuItem {
                text: qsTr("Start server")
                shortcut: "Ctrl+S"
                onTriggered: mainWindow.startServer();
            }

            MenuItem {
                text: qsTr("PC console start")
                onTriggered: mainWindow.pcConsoleStart();
            }

            MenuItem {
                text: qsTr("Replay")
                onTriggered: mainWindow.replay();
            }

            MenuItem {
                text: qsTr("Exit")
                onTriggered: mainWindow.close();
            }
        }

        Menu {
            title: qsTr("View")

            MenuItem {
                text: qsTr("Show/Hide menu")
                shortcut: "Alt+M"
            }

            MenuItem {
                text: qsTr("Full screen")
                shortcut: "Alt+Return"
            }

            MenuItem {
                text: qsTr("Minimize to system tray")
                shortcut: "Alt+S"
            }
        }

        Menu {
            title: qsTr("Tools")

            MenuItem {
                text: qsTr("Configure")
                onTriggered: mainWindow.configure();
            }

            MenuItem {
                text: qsTr("General overview")
                shortcut: "Ctrl+E"
                onTriggered: mainWindow.generalOverview();
            }

            MenuItem {
                text: qsTr("Card overview")
                shortcut: "Ctrl+R"
                onTriggered: mainWindow.cardOverview();
            }

            MenuItem {
                text: qsTr("View ban list")
            }

            MenuItem {
                text: qsTr("Broadcast...")
            }

            MenuItem {
                text: qsTr("Replay file convert...")
            }

            MenuItem {
                text: qsTr("Record analysis")
            }

            MenuSeparator {

            }

            MenuItem {
                text: qsTr("Enable hotkey")
                checkable: true
            }

            MenuItem {
                text: qsTr("Never nullify my trick")
                checkable: true
            }

            MenuItem {
                text: qsTr("View discarded...")
            }

            MenuItem {
                text: qsTr("View distance")
            }

            MenuItem {
                text: qsTr("Server information...")
            }

            MenuItem {
                text: qsTr("Surrender")
            }

            MenuItem {
                text: qsTr("Save battle record...")
            }
        }

        Menu {
            title: qsTr("Cheat")
            enabled: false

            MenuItem {
                text: qsTr("Damage maker...")
            }

            MenuItem {
                text: qsTr("Death note...")
            }

            MenuItem {
                text: qsTr("Revive wand...")
            }

            MenuItem {
                text: qsTr("Execute script at server side...")
            }
        }

        Menu {
            title: qsTr("Help")

            MenuItem {
                text: qsTr("About")
            }

            MenuItem {
                text: qsTr("About Us")
                onTriggered: mainWindow.aboutUs();
            }

            MenuItem {
                text: qsTr("About Qt")
            }

            MenuItem {
                text: qsTr("About GPLv3")
            }

            MenuItem {
                text: qsTr("About Lua")
            }

//            MenuItem {
//                text: qsTr("About fmod")
//            }

            MenuSeparator {

            }

            MenuItem {
                text: qsTr("Acknowledgement")
            }

            MenuSeparator {

            }

            MenuItem {
                text: qsTr("Role assign table")
            }
        }
    }

    Item {
        id: sceneArea
        anchors.fill: parent

        Component.onCompleted: {
            var component = Qt.createComponent("StartScene.qml")
            if (component.status === Component.Ready) {
                var item = component.createObject(sceneArea)
            } else {
                console.log(component.errorString())
            }
        }
    }

    MessageDialog {
        id: confirmExitDialog

        title: qsTr("Confirm exit")
        icon: StandardIcon.Question
        text: qsTr("Exit TouhouSatsu?")
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            mainWindow.closeConfirmed = true;
            mainWindow.close();
        }
    }

    ConfigDialog {
        id: configDialog
    }

    property bool closeConfirmed: false

    onClosing: {
        if (!closeConfirmed) {
            close.accepted = false;
            confirmExitDialog.open();
        } else {

        }
    }

    function startGame() {
        console.log("startGame")
    }

    function startServer() {
        console.log("startServer")
    }

    function pcConsoleStart() {
        console.log("pcConsoleStart")
    }

    function replay() {
        console.log("replay")
    }

    function configure() {
        configDialog.open();
    }

    function generalOverview() {
        console.log("generalOverview")
    }

    function cardOverview() {
        console.log("cardOverview")
    }

    function aboutUs() {
        console.log("aboutUs")
    }
}
