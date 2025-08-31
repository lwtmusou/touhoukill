import QtQuick 6.5
import rocks.touhousatsu 1.0

Image {
    property int phase: Player.NotActive

    fillMode: Image.PreserveAspectCrop
    clip: true

    width: 209
    height: 20

    onPhaseChanged: {
        if (G.isPlayerMainPhase(phase)) {
            opacity = 1;
            source = G.getUrl("image/system/phase/" + G.playerPhaseToString(phase) + ".png");
        } else {
            opacity = 0;
        }
    }
}
