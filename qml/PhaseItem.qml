import QtQuick 6.5
import rocks.touhousatsu 1.0

Image {
    property int phase: Player.NotActive

    clip: true
    fillMode: Image.PreserveAspectCrop
    height: 20
    width: 209

    onPhaseChanged: {
        if (G.isPlayerMainPhase(phase)) {
            opacity = 1;
            source = G.getUrl("image/system/phase/" + G.playerPhaseToString(phase) + ".png");
        } else {
            opacity = 0;
        }
    }
}
