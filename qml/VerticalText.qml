import QtQuick 6.5

Text {
    property string verticalText

    text: Array.from(verticalText).join("\n")
    textFormat: Text.PlainText
}
