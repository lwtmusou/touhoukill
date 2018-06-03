import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2

Dialog {
    title: qsTr("Config dialog")

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


                ColumnLayout {
                    GroupBox {
                        title: qsTr("Animation")
                        Layout.fillWidth: true

                        RowLayout {
                            CheckBox {
                                id: noIndicatorCheckBox
                                text: qsTr("No indicator")
                            }

                            CheckBox {
                                id: noEquipAnimCheckBox
                                text: qsTr("No equip animation")
                            }

                            CheckBox {
                                id: noLordBackdropCheckBox
                                text: qsTr("No lord backdrop")
                            }
                        }

                    }

                    GroupBox {
                        title: qsTr("Audio")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            RowLayout {
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
                            }

                            RowLayout {
                                CheckBox {
                                    id: enableLastWordCheckBox
                                    text: qsTr("Enable last word")
                                }

                                CheckBox {
                                    id: enableBackgroundMusicCheckBox
                                    text: qsTr("Enable background music")
                                }

                                CheckBox {
                                    id: enableEffectsCheckBox
                                    text: qsTr("Enable effects")
                                }

                                CheckBox {
                                    id: enableLordBackgroundMusicCheckBox
                                    text: qsTr("Enable lord background music")
                                }
                            }


                            RowLayout {
                                Text {
                                    text: qsTr("Bgm volume")
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    id: bgmVolumeSlider

                                    minimumValue: 0
                                    maximumValue: 100
                                }
                            }

                            RowLayout {
                                Text {
                                    text: qsTr("Effect volume")
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    id: effectVolumeSlider

                                    minimumValue: 0
                                    maximumValue: 100
                                }
                            }
                        }
                    }

                    GroupBox{
                        title: qsTr("Font")
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 4
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
                    }

                    Item {
                        Layout.fillHeight: true
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


                ColumnLayout {
                    CheckBox {
                        id: neverNullifyMyTrickCheckBox
                        text: qsTr("Never nullify my signle target trick")
                    }

                    CheckBox {
                        id: enableAutoTargetCheckBox
                        text: qsTr("Enable auto target")
                    }

                    CheckBox {
                        id: enableIntellectualSelectionCheckBox
                        text: qsTr("Enable intellectual selection")
                    }

                    CheckBox {
                        id: enableDoubleClickCheckBox
                        text: qsTr("Enable double-click")
                    }

                    CheckBox {
                        id: useDefaultHeroSkinCheckBox
                        text: qsTr("Use default hero skin")
                    }

                    CheckBox {
                        id: enableAutoUpdateCheckBox
                        text: qsTr("Enable auto update")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Bubble chat box delay(seconds)")
                        }

                        SpinBox {
                            Layout.fillWidth: true
                            id: bubbleChatBoxDelaySpinBox
                            minimumValue: 0
                            maximumValue: 5
                        }
                    }

                    GroupBox {
                        title: qsTr("Game record")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent

                            CheckBox {
                                id: enableAutoSaveCheckBox
                                text: qsTr("Enable auto save")
                            }

                            CheckBox {
                                id: networkGameOnlyCheckBox
                                text: qsTr("Network game only")
                                enabled: enableAutoSaveCheckBox.checked
                            }

                            RowLayout {
                                Text {
                                    text: qsTr("Setup record paths")
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    id: recordPathTextEdit
                                }

                                Button {
                                    text: qsTr("Browse...")
                                }

                                Button {
                                    text: qsTr("Reset")
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
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
    }



    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {

    }
}
