import QtQuick 6.5
import rocks.touhousatsu 1.0

Image {
    property int seat: 0

    clip: true
    fillMode: Image.PreserveAspectCrop
    width: height * 23 / 16

    onSeatChanged: {
        if (seat >= 1 && seat <= 10) {
            opacity = 1;
            source = G.getAssetUrl("image/system/seat-num/photo/" + seat.toString() + ".png");
        } else {
            opacity = 0;
        }
    }
}
