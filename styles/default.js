function DefaultStyle()
{
	this.name = "default"
}

function StartSceneStyle()
{
    this.buttonFont = Qt.font({})
    this.bgImage = "backdrop/hall/gensoukyou_" + toString(Math.floor(Math.random() * 8 + 1)) + ".jpg"
}

DefaultStyle.start = new StartSceneStyle()


new DefaultStyle();
