import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    property string kingdom

    width: 56
    height: 56

    onKingdomChanged: {
        source = G.getUrl("image/kingdom/icon/" + kingdom + ".png");
    }
}
