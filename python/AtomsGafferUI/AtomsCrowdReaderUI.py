import IECore

import Gaffer

import AtomsGaffer

import DocumentationAlgo

Gaffer.Metadata.registerNode(

    AtomsGaffer.AtomsCrowdReader,

    "description",
    """
    Loads Atoms Crowd sim files and generates points representing the crowd.
    Each point represents one agent and contains PrimitiveVariables describing
    per agent Atoms metadata.
    """,

    "icon", "atoms_logo.png",
    "documentation:url", DocumentationAlgo.documentationURL,

    plugs = {

        "atomsSimFile" : [

            "description",
            """
            The full path to an Atoms simulation file sequence.
            """,

            "plugValueWidget:type", "GafferUI.FileSystemPathPlugValueWidget",
            "path:leaf", True,
            "path:valid", True,
            "path:bookmarks", "atoms",
            "fileSystemPath:extensions", "atoms",
            "fileSystemPath:extensionsLabel", "Show only Atoms files",
            "fileSystemPath:includeSequences", True,
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
