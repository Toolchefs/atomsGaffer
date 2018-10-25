import IECore

import Gaffer
import AtomsGaffer

import DocumentationAlgo

Gaffer.Metadata.registerNode(

    AtomsGaffer.AtomsAgentReader,

    "description",
    """
    Loads Atoms Agent variations in a default pose.
    """,

    "icon", "atoms_logo.png",
    "documentation:url", DocumentationAlgo.documentationURL,

    plugs = {

        "atomsAgentFile" : [

            "description",
            """
            The full path to an Atoms Agent variation file.
            """,

            "plugValueWidget:type", "GafferUI.FileSystemPathPlugValueWidget",
            "path:leaf", True,
            "path:valid", True,
            "path:bookmarks", "atoms",
            "fileSystemPath:extensions", "atoms",
            "fileSystemPath:extensionsLabel", "Show only Atoms files",
            "layout:index", 0,

        ],

        "refreshCount" : [

            "description",
            """
            May be incremented to force a reload if the file has
			changed on disk - otherwise old contents may still
			be loaded via Gaffer's cache.
            """,

            "plugValueWidget:type", "GafferUI.RefreshPlugValueWidget",
            "layout:label", "",
            "layout:accessory", True,
            "layout:index", 1,
        ],

    },

)
