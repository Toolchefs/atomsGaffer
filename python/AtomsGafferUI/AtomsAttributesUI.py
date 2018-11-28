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

        "agentIds" : [

            "description",
            """
            Filter agents using their indices.
            Use ',' to set multiple indices (eg. 10,15,20).
            Use '-' to set a range of indices (eg. 2-5, 10-20).
            Use '!' to exclude some indices (eg. !5, !11).
            """,
            "label", "Agent Indices",
        ],

        "metadata" : [

            "description",
            """
            Used to override or create agents metadata.
            """,
            "label", "Metadata",

        ],

    },

)
