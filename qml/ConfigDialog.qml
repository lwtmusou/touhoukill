import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2

Dialog {
    FontMetrics {
        id: fontMetrics
    }

    TabView {
        id: tabView
        anchors.fill: parent

        property int tab1ImplicitWidth: 0
        property int tab1ImplicitHeight: 0
        property int tab2ImplicitWidth: 0
        property int tab2ImplicitHeight: 0

        implicitHeight: Math.max(tab1ImplicitHeight, tab2ImplicitHeight) + fontMetrics.height * 1.7
        implicitWidth: Math.max(tab1ImplicitWidth, tab2ImplicitWidth)

        Tab {
            title: qsTr("Environment")
            active: true

            onImplicitWidthChanged: {
                tabView.tab1ImplicitWidth = implicitWidth
            }

            onImplicitHeightChanged: {
                tabView.tab1ImplicitHeight = implicitHeight
            }

            GridLayout {
                columns: 3
                Item {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    implicitHeight: 3
                }

                Item {
                    implicitWidth: 3
                    Layout.fillHeight: true
                }


                GridLayout {
                    columns: 4

                    Text {
                        text: qsTr("Setup background")
                    }

                    TextField {
                        Layout.fillWidth: true
                        id: backgroundEdit
                    }

                    Button {
                        text: qsTr("Browse...")
                    }

                    Button {
                        text: qsTr("Reset")
                    }

                    Text {
                        text: qsTr("Setup tableBg")
                    }

                    TextField {
                        Layout.fillWidth: true
                        id: tableBgEdit
                    }

                    Button {
                        text: qsTr("Browse...")
                    }

                    Button {
                        text: qsTr("Reset")
                    }

                    RowLayout {
                        Layout.columnSpan: 4
                        CheckBox {
                            Layout.fillWidth: true
                            id: noIndicatorCheckBox
                            text: qsTr("No indicator")
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            id: noEquipAnimCheckBox
                            text: qsTr("No equip animation")
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            id: noLordBackdropCheckBox
                            text: qsTr("No lord backdrop")
                        }
                    }

                    Text {
                        text: qsTr("Setup background music")
                    }

                    TextField {
                        Layout.fillWidth: true
                        id: bgmEdit
                    }

                    Button {
                        text: qsTr("Browse...")
                    }

                    Button {
                        text: qsTr("Reset")
                    }

                    GridLayout {
                        Layout.columnSpan: 4
                        columns: 2
                        CheckBox {
                            Layout.fillWidth: true
                            id: enableLastWordCheckBox
                            text: qsTr("Enable last word")
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            id: enableBackgroundMusicCheckBox
                            text: qsTr("Enable background music")
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            id: enableEffectsCheckBox
                            text: qsTr("Enable Effects")
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            id: enableLordBackgroundMusicCheckBox
                            text: qsTr("Enable lord background music")
                        }
                    }

                    Text {
                        text: "Bgm volume"
                    }

                    Slider {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        id: bgmVolumeSlider
                    }

                    Text {
                        text: "Effect volume"
                    }

                    Slider {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        id: effectVolumeSlider
                    }

                    Text {
                        text: qsTr("Application font")
                    }

                    TextField {
                        Layout.fillWidth: true
                        id: applicationFontTextEdit
                    }

                    Button {
                        Layout.columnSpan: 2
                        text: qsTr("Set application font")
                    }

                    Text {
                        text: qsTr("Text edit font")
                    }

                    TextField {
                        Layout.fillWidth: true
                        id: textEditFontTextEdit
                    }

                    Button {
                        text: qsTr("Font...")
                    }

                    Button {
                        text: qsTr("Color...")
                    }
                }

                Item {
                    implicitWidth: 3
                    Layout.fillHeight: true
                }

                Item {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    implicitHeight: 3
                }
            }
        }


        Tab {
            title: qsTr("Game")

            onImplicitWidthChanged: {
                tabView.tab2ImplicitWidth = implicitWidth
            }

            onImplicitHeightChanged: {
                tabView.tab2ImplicitHeight = implicitHeight
            }

            GridLayout {
                columns: 3
                Item {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    implicitHeight: 3
                }

                Item {
                    implicitWidth: 3
                    Layout.fillHeight: true
                }


                GridLayout {
                    // TODO: add items here
                }

                Item {
                    implicitWidth: 3
                    Layout.fillHeight: true
                }

                Item {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    implicitHeight: 3
                }
            }
        }
    }



    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {

    }
}
