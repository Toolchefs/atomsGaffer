##########################################################################
#
#  Copyright (c) 2018, Toolchefs Ltd. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#
#      * Neither the name of John Haddon nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import IECore

import Gaffer

import AtomsGaffer

from . import DocumentationAlgo

Gaffer.Metadata.registerNode(

    AtomsGaffer.AtomsCrowdGenerator,

    "description",
    """
    Generates a fully deformed crowd of agents by combining the input crowd points
    from the primary scene, and the specified agents from the right-hand scene.
    """,

    "icon", "atoms_logo.png",
    "documentation:url", DocumentationAlgo.documentationURL,

    plugs = {

        "parent" : [

            "description",
            """
			The location of the crowd points in the input scene. The
			per-vertex primitive variable "\agentType\" will be used
			to determine which agents to instantiate. It must contain
			indexed string data (ie. as generated by AtomsCrowdReader).
			"""

        ],

        "name" : [

            "description",
            """
            The name of the new location containing the generated agents.
            """,

        ],

        "destination" : [

            "description",
            """
            The location where the agents will be placed in the output scene.
            When the destination is evaluated, the `${scene:path}` variable holds
            the location of the crowd points, so the default value parents the agents
            under the points.
            """

        ],

        "variations" : [

            "description",
            """
			The scene containing the agent variation mesh to be applied to each agent skeleton of
			the crowd. Specify multiple variation by parenting them at the right agent type:

			- /soldier/soldierSword
			- /soldier/soldierSwordShield
			- /archer/defaultArcher
			
			Set up the lod using ':' after the variation name. Eg. /soldier/soldierSword:A

			Note that the variations are not limited to being a
			single object : they can each have arbitrary child
			hierarchies.
			""",

            "plugValueWidget:type", "",

        ],

        "useInstances" : [

            "description",
            """
            Turn on agent instancing. Agents with the same pose and metadata values are instanced.
            """,

        ],

        "boundingBoxPadding" : [

            "description",
            """
            Bounding box padding
            """,
            "layout:section", "Bounding Box",
            "label", "Padding"
        ],

		"clothCache" : [

			"description",
			"""
			The scene containing the cloth cache metadata.
			""",

		],

    },

)
