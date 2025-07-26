import QtQuick 2.15
import QtGraphicalEffects 1.15
import QtQuick.Controls 2.15

Item {
  // size of a card is determined outside so we don't care.
  // width: 200;
  // height: 400;

  id: root

  property string card_name: "slash"; //
  property string card_suit: "heart"; // spade, diamond, heart, club
  property int card_number: 1;        // 0, 1-13

  Image {
    source: "../image/card/" + card_name + ".png";
    anchors.fill: parent;
  }

  Image { // suit
    id: suit_pict
    anchors.left: parent.left;
    anchors.leftMargin: parent.width * 0.1;

    anchors.top: parent.top;
    anchors.topMargin: parent.height * 0.05;

    width: parent.width * 0.1;
    height: width;

    source: "../image/system/cardsuit/" + card_suit + ".svg";
  }

  DropShadow {
    anchors.fill: suit_pict;
    radius: 4;
    samples: 9;
    color: "yellow";
    source: suit_pict;
  }

  Text { // card number
    anchors.horizontalCenter: suit_pict.horizontalCenter;

    anchors.top: suit_pict.bottom;
    anchors.topMargin: parent.height * 0.01;

    font.pixelSize: parent.width * 0.1;

    color: {
      if(parent.card_suit == "diamond" || parent.card_suit == "heart"){
        return "red";
      } else if(parent.card_suit == "spade" || parent.card_suit == "club"){
        return "black";
      }
    }

    text: {
      console.assert(card_number >= 0 && card_number <= 13, "Please keep a valid number for card.");
      switch(card_number){
        case 0:   return "";
        case 1:   return "A";
        case 11:  return "J";
        case 12:  return "Q";
        case 13:  return "K";
        default:  return card_number;
      }
    }
  }

  Column {
    id: left_name
    anchors.verticalCenter: parent.verticalCenter

    anchors.left: parent.left
    anchors.leftMargin: parent.width * 0.05;

    Repeater {
      model: qsTr(card_name).split("");
      Text { // name on the left side
        font.pixelSize: root.width * 0.1;
        text: modelData;
        
      }
    }
    spacing: -parent.width * 0.01;
  }
  
  DropShadow {
    anchors.fill: left_name;
    radius: 8;
    samples: 17;
    color: "white";
    source: left_name
  }

  MouseArea{
    id: mouse_area
    anchors.fill: parent;
    hoverEnabled: true;

    onEntered: ParallelAnimation {
      PropertyAnimation {
        target: root;
        property: "scale";
        from: 1;
        to: 1.05;
      }

      PropertyAnimation {
        target: root;
        property: "opacity";
        from: 0.8;
        to: 1;
      }
    }
     

    onExited: ParallelAnimation {
      PropertyAnimation{
        target: root;
        property: "scale";
        from: 1.05;
        to: 1;
      }

      PropertyAnimation {
        target: root;
        property: "opacity";
        from: 1;
        to: 0.8;
      }
    }
  }

  ToolTip {
    visible: mouse_area.containsMouse;
    delay: 300;
    text: qsTr("Very strange tooltip.")
    x: root.width / 3.0
    y: root.height / 3.0
  }

  
}
