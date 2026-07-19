import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    property string kingdom

    height: 56
    width: 56

    onKingdomChanged: {
        source = G.getAssetUrl("image/kingdom/icon/" + kingdom + ".png");
    }
}
