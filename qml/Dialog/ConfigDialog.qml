import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3

Window {
    id: configDialog
    ColumnLayout {
        anchors.fill: parent
        visible: true
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "red"
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "green"
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "blue"
        }
    }

    // Qt Creator report M16 error on onClosing, ignoring
    onClosing: {
        configDialog.destroy()
    }
}
