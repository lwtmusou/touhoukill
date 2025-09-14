import QtQuick 6.5
import rocks.touhousatsu 1.0

Image {
    property int seat: 0

    fillMode: Image.PreserveAspectCrop
    clip: true

    width: height * 23 / 16

    onSeatChanged: {
        if (seat >= 1 && seat <= 10) {
            opacity = 1;
            source = G.getUrl("image/system/seat-num/photo/" + seat.toString() + ".png");
        } else {
            opacity = 0;
        }
    }
}
