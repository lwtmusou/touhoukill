local record sgs
	userdata
	record QVariant
		userdata
	end

	record qrc
		userdata
		contents: function(string): string
		contains: function(string): boolean
	end

	record QObject
		userdata
		objectName: function(sgs.QObject): string
		setObjectName: function(sgs.QObject, string)
		inherits: function(sgs.QObject, string): boolean
		setProperty: function(sgs.QObject, string, sgs.QVariant): boolean
		property: function(sgs.QObject, string): sgs.QVariant
		setParent: function(sgs.QObject, sgs.QObject)
		deleteLater: function(sgs.QObject)
	end

	record QWidget
		userdata
	end

	information: function(sgs.QWidget, string, string)
end

return sgs
