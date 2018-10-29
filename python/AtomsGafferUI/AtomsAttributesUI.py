import IECore

import Gaffer
import AtomsGaffer

import DocumentationAlgo

Gaffer.Metadata.registerNode(

    AtomsGaffer.AtomsAttributes,

    "description",
    """
    Applies Atoms attributes to locations in the scene.
    """,

    "icon", "atoms_logo.png",
    "documentation:url", DocumentationAlgo.documentationURL,

    plugs = {

        "attributes.agentType" : [

            "description",
            """
            Used to override the Atoms agent type.
            """,

            "layout:section", "Agents",

        ],

        "attributes.tint" : [

            "description",
            """
            Used to override the Atoms tint variation.
            """,

            "layout:section", "Variations",

        ],

    },

)
