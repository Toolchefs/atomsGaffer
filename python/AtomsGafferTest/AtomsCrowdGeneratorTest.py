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

import unittest
import imath

import IECore
import IECoreScene

import GafferScene

import GafferTest
import GafferSceneTest

import AtomsGaffer

from AtomsTestData import buildCrowdTest, buildVariationTest

class AtomsCrowdGeneratorTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsCrowdGenerator()
		self.assertEqual( a.getName(), "AtomsCrowdGenerator" )

	def testChildNames( self ) :
		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )

		variations = AtomsGaffer.AtomsVariationReader()
		variations["atomsVariationFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/atomsRobot.json" )

		node = AtomsGaffer.AtomsCrowdGenerator()
		node["parent"].setValue( "/crowd" )
		node["in"].setInput( crowd_input["out"] )
		node["variations"].setInput( variations["out"] )

		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )
		node["clothCache"].setInput( cloth["out"] )

		names = node["out"].childNames( "/" )
		self.assertEqual( len(names), 1 )
		self.assertEqual( names[0], "crowd" )

		names = node["out"].childNames( "/crowd" )
		self.assertEqual( len(names), 1 )
		self.assertEqual( names[0], "agents" )

		names = node["out"].childNames( "/crowd/agents" )
		self.assertEqual( len(names), 2 )
		self.assertTrue( "atomsRobot" in names )
		self.assertTrue( "atoms2Robot" in names )

		names = node["out"].childNames( "/crowd/agents/atomsRobot" )
		self.assertEqual( len(names), 2 )
		self.assertTrue( "Robot1" in names )
		self.assertTrue( "Robot2" in names)

		names = node["out"].childNames( "/crowd/agents/atoms2Robot" )
		self.assertEqual( len(names), 3 )
		self.assertTrue( "RedRobot" in names )
		self.assertTrue( "PurpleRobot" in names )
		self.assertTrue( "YellowRobot" in names )

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot1" )
		self.assertTrue( "0" in names )

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot2" )
		self.assertTrue( "1" in names )

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/RedRobot" )
		self.assertTrue( "5" in names )

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/PurpleRobot" )
		self.assertTrue( "12" in names )

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot1/0" )
		self.assertTrue( "RobotSkin1" in names)

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1" )
		self.assertTrue( "arms" in names)
		self.assertTrue( "body" in names)
		self.assertTrue( "legs" in names)
		self.assertTrue( "head" in names)
		self.assertTrue( "flag_group" in names)

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head" )
		self.assertTrue( "robot1_head" in names)

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body" )
		self.assertTrue( "robot1_body" in names)

		names = node["out"].childNames( "/crowd/agents/atomsRobot/Robot2/1" )
		self.assertTrue( "robot2_body" in names)

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/RedRobot/5" )
		self.assertEqual( len(names), 1 )
		self.assertTrue( "Body" in names)

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/RedRobot/5/Body" )
		self.assertEqual( len(names), 1 )
		self.assertTrue( "RobotBody" in names)

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/PurpleRobot/12" )
		self.assertEqual( len(names), 1 )
		self.assertTrue( "Body" in names)

		names = node["out"].childNames( "/crowd/agents/atoms2Robot/PurpleRobot/12/Body" )
		self.assertEqual( len(names), 1 )
		self.assertTrue( "RobotBody" in names)

	def testAttributes( self ) :
		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )

		variations = AtomsGaffer.AtomsVariationReader()
		variations["atomsVariationFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/atomsRobot.json" )

		custom_attributes = GafferScene.CustomAttributes()
		custom_attributes["attributes"].addMember( "testAttribute", IECore.IntData( 5 ) )
		custom_attributes["in"].setInput( variations["out"] )

		filter = GafferScene.PathFilter()
		custom_attributes["filter"].setInput( filter["out"] )
		filter["paths"].setValue(IECore.StringVectorData(["atomsRobot","atoms2Robot/RedRobot"]))

		node = AtomsGaffer.AtomsCrowdGenerator()
		node["parent"].setValue( "/crowd" )
		node["variations"].setInput( custom_attributes["out"] )
		node["in"].setInput( crowd_input["out"] )

		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )
		node["clothCache"].setInput( cloth["out"] )

		self.assertTrue( "atoms:agents" in node["out"].attributes( "/crowd" ) )
		attributes = node["out"].attributes( "/crowd/agents/atomsRobot" )
		self.assertTrue( "testAttribute" in attributes )
		self.assertEqual( attributes[ "testAttribute" ].value, 5 )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot" )
		self.assertEqual( attributes, IECore.CompoundObject() )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1" )
		self.assertEqual( attributes, IECore.CompoundObject() )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot2" )
		self.assertEqual( attributes, IECore.CompoundObject() )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/RedRobot" )
		self.assertTrue( "testAttribute" in attributes )
		self.assertEqual( attributes[ "testAttribute" ].value, 5 )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/PurpleRobot" )
		self.assertEqual( attributes, IECore.CompoundObject() )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1/0" )
		self.assertTrue( "user:atoms:agentType" in attributes )
		self.assertEqual( attributes[ "user:atoms:agentType" ].value, "atomsRobot" )
		self.assertTrue( "user:atoms:variation" in attributes )
		self.assertEqual( attributes[ "user:atoms:variation" ].value, "Robot1" )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body" )
		self.assertTrue( "ai:polymesh:subdiv_adaptive_space" in attributes )
		self.assertEqual( attributes[ "ai:polymesh:subdiv_adaptive_space" ].value, "raster" )
		self.assertTrue( "jointIndexCount" not in attributes )
		self.assertTrue( "jointIndices" not in attributes )
		self.assertTrue( "jointWeights" not in attributes )
		self.assertEqual( attributes[ "user:atoms:fooBody" ].value, 1.5 )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pole" )
		self.assertTrue( "ai:polymesh:subdiv_adaptive_space" in attributes )
		self.assertEqual( attributes[ "ai:polymesh:subdiv_adaptive_space" ].value, "raster" )
		self.assertTrue( "jointIndexCount" not in attributes )
		self.assertTrue( "jointIndices" not in attributes )
		self.assertTrue( "jointWeights" not in attributes )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pPlane1" )
		self.assertTrue( "ai:polymesh:subdiv_adaptive_space" in attributes )
		self.assertEqual( attributes[ "ai:polymesh:subdiv_adaptive_space" ].value, "raster" )
		self.assertTrue( "jointIndexCount" not in attributes )
		self.assertTrue( "jointIndices" not in attributes )
		self.assertTrue( "jointWeights" not in attributes )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group" )
		self.assertEqual( attributes[ "user:atoms:fooFlagGroup" ].value, imath.V3f( 1.0, 2.0, 3.0) )

		attributes = node["out"].attributes( "/crowd/agents/atomsRobot/Robot2/1" )
		self.assertTrue( "user:atoms:agentType" in attributes )
		self.assertEqual( attributes[ "user:atoms:agentType" ].value, "atomsRobot" )
		self.assertTrue( "user:atoms:variation" in attributes )
		self.assertEqual( attributes[ "user:atoms:variation" ].value, "Robot2" )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/RedRobot/5" )
		self.assertTrue( "user:atoms:agentType" in attributes )
		self.assertEqual( attributes[ "user:atoms:agentType" ].value, "atoms2Robot" )
		self.assertTrue( "user:atoms:variation" in attributes )
		self.assertEqual( attributes[ "user:atoms:variation" ].value, "RedRobot" )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/RedRobot/5/Body/RobotBody" )
		self.assertTrue( "ai:polymesh:subdiv_adaptive_space" in attributes )
		self.assertEqual( attributes[ "ai:polymesh:subdiv_adaptive_space" ].value, "raster" )
		self.assertTrue( "jointIndexCount" not in attributes )
		self.assertTrue( "jointIndices" not in attributes )
		self.assertTrue( "jointWeights" not in attributes )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/PurpleRobot/12" )
		self.assertTrue( "user:atoms:agentType" in attributes )
		self.assertEqual( attributes[ "user:atoms:agentType" ].value, "atoms2Robot" )
		self.assertTrue( "user:atoms:variation" in attributes )
		self.assertEqual( attributes[ "user:atoms:variation" ].value, "PurpleRobot" )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/PurpleRobot/12/Body" )
		self.assertEqual( attributes, IECore.CompoundObject() )

		attributes = node["out"].attributes( "/crowd/agents/atoms2Robot/RedRobot/12/Body/RobotBody" )
		self.assertTrue( "ai:polymesh:subdiv_adaptive_space" in attributes )
		self.assertEqual( attributes[ "ai:polymesh:subdiv_adaptive_space" ].value, "raster" )
		self.assertTrue( "jointIndexCount" not in attributes )
		self.assertTrue( "jointIndices" not in attributes )
		self.assertTrue( "jointWeights" not in attributes )

	def testCompute( self ) :
		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )

		variations = AtomsGaffer.AtomsVariationReader()
		variations["atomsVariationFile"].setValue("${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/atomsRobot.json")

		node = AtomsGaffer.AtomsCrowdGenerator()
		node["parent"].setValue( "/crowd" )
		node["in"].setInput( crowd_input["out"] )
		node["variations"].setInput( variations["out"] )

		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )
		node["clothCache"].setInput( cloth["out"] )

		obj = node["out"].object( "/crowd/agents" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot2" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/RedRobot" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/PurpleRobot" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head/robot1_head" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 920 )
		self.assertEqual( len(obj["N"].data), 3480 )
		self.assertEqual( len(obj["uv"].data), 1533 )
		self.assertTrue( "blendShape_0_P" not in obj )
		self.assertTrue( "blendShape_1_P" not in obj )
		self.assertTrue( "blendShape_0_N" not in obj )
		self.assertTrue( "blendShape_1_N" not in obj )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 635 )
		self.assertEqual( len(obj["N"].data), 2356 )
		self.assertEqual( len(obj["uv"].data), 1222 )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms/robot1_arms" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 960 )
		self.assertEqual( len(obj["N"].data), 3424 )
		self.assertEqual( len(obj["uv"].data), 1832 )
		self.assertEqual( len(obj["primFaceAttr"].data), 828 )
		self.assertEqual( len(obj["primObjAttr"].data), 1 )
		self.assertEqual( len(obj["primVertsAttr"].data), 960 )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 904 )
		self.assertEqual( len(obj["N"].data), 3312 )
		self.assertEqual( len(obj["uv"].data), 1516 )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pole" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 1022 )
		self.assertEqual( len(obj["N"].data), 4120 )
		self.assertEqual( len(obj["uv"].data), 1113 )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pPlane1" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 121 )
		self.assertEqual( len(obj["N"].data), 400 )
		self.assertEqual( len(obj["uv"].data), 121 )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot2/1" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atomsRobot/Robot2/1/robot2_body" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 16944 )
		self.assertEqual( len(obj["N"].data), 16944 )
		self.assertEqual( len(obj["uv"].data), 16944 )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/RedRobot/5" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/RedRobot/5/Body" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/RedRobot/5/Body/RobotBody" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 28149 )
		self.assertEqual( len(obj["N"].data), 111930 )
		self.assertEqual( len(obj["uv"].data), 31402 )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/PurpleRobot/12" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/PurpleRobot/12/Body" )
		self.assertEqual( obj, IECore.NullObject() )

		obj = node["out"].object( "/crowd/agents/atoms2Robot/PurpleRobot/12/Body/RobotBody" )
		self.assertEqual( obj.typeName(), IECoreScene.MeshPrimitive.staticTypeName() )
		self.assertEqual( len(obj["P"].data), 28149 )
		self.assertEqual( len(obj["N"].data), 111930 )
		self.assertEqual( len(obj["uv"].data), 31402 )

	def testTransform( self ) :
		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )

		variations = AtomsGaffer.AtomsVariationReader()
		variations["atomsVariationFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/atomsRobot.json" )

		node = AtomsGaffer.AtomsCrowdGenerator()
		node["parent"].setValue( "/crowd" )
		node["in"].setInput( crowd_input["out"] )
		node["variations"].setInput( variations["out"] )

		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )
		node["clothCache"].setInput( cloth["out"] )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1" )
		self.assertEqual( obj, imath.M44f() )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head" )
		self.assertEqual( obj, imath.M44f( 1, 0, 0, 0, 0, 0.995992899, 0.0894325897, 0, 0, -0.0894325897, 0.995992899, 0, 0, 1.03006685, -4.52774239, 1 ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head/robot1_head" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, 51.0, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, 13.1848993, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms/robot1_arms" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, 15.8421688, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body" )
		self.assertEqual( obj, imath.M44f() )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, 16.0, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, -10.0, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs" )
		self.assertEqual( obj, imath.M44f().translate( imath.V3f( 0.0, -33.0, 0.0 ) ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group" )
		self.assertEqual( obj.translation(), imath.V3f( 75.4688797, 28.3711166, 11.6567278 ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pPlane1" )
		self.assertEqual( obj, imath.M44f( 64.4932404, 0, 0, 0, 0, 64.4932404, 0, 0, 0, 0, 41.1785202, 0, -32.3346634, 0, 94.8534851, 1 ) )

		obj = node["out"].transform( "/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pole" )
		self.assertEqual( obj, imath.M44f() )

	def testSets( self ):
		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )

		variations = AtomsGaffer.AtomsVariationReader()
		variations["atomsVariationFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/atomsRobot.json" )

		node = AtomsGaffer.AtomsCrowdGenerator()
		node["parent"].setValue( "/crowd" )
		node["in"].setInput( crowd_input["out"] )
		node["variations"].setInput( variations["out"] )

		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )
		node["clothCache"].setInput( cloth["out"] )

		set_names = node["out"]["setNames"].getValue()
		obj = node["out"]
		self.assertTrue( "atoms2Robot:PurpleRobot" in set_names )
		self.assertEqual(
			set( obj.set( "atoms2Robot:PurpleRobot" ).value.paths() ),
			{
				'/crowd/agents/atoms2Robot/PurpleRobot/12/Body/RobotBody',
				'/crowd/agents/atoms2Robot/PurpleRobot/15/Body/RobotBody',
				'/crowd/agents/atoms2Robot/PurpleRobot/19/Body/RobotBody'
			}
		)

		self.assertTrue( "atoms2Robot:RedRobot" in set_names )
		self.assertEqual(
			set( obj.set( "atoms2Robot:RedRobot" ).value.paths() ),
			{
				'/crowd/agents/atoms2Robot/RedRobot/16/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/23/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/5/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/8/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/22/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/6/Body/RobotBody',
				'/crowd/agents/atoms2Robot/RedRobot/7/Body/RobotBody'

			}
		)
		self.assertTrue( "atoms2Robot:RedRobot:A" in set_names )

		self.assertTrue( "atoms2Robot:YellowRobot" in set_names )
		self.assertEqual(
			set( obj.set( "atoms2Robot:YellowRobot" ).value.paths() ),
			{
				'/crowd/agents/atoms2Robot/YellowRobot/9/Body/RobotBody',
				'/crowd/agents/atoms2Robot/YellowRobot/4/Body/RobotBody',
				'/crowd/agents/atoms2Robot/YellowRobot/3/Body/RobotBody'

			}
		)

		self.assertTrue( "atomsRobot:Robot1" in set_names )

		self.assertEqual(
			set( obj.set( "atomsRobot:Robot1" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/legs/robot1_legs'
			}
		)

		self.assertTrue( "atomsRobot:Robot1:A" in set_names )
		self.assertTrue( "atomsRobot:Robot1:B" in set_names )

		self.assertTrue( "atomsRobot:Robot2" in set_names )
		self.assertEqual(
			set( obj.set( "atomsRobot:Robot2" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot2/11/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/2/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/11/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/21/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/1/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/14/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/1/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/13/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/14/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/20/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/11/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/11/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/2/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/21/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/20/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/1/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/1/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/20/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/13/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/13/robot2_arms',
				'/crowd/agents/atomsRobot/Robot2/20/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/2/robot2_body',
				'/crowd/agents/atomsRobot/Robot2/21/robot2_legs',
				'/crowd/agents/atomsRobot/Robot2/14/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/2/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/21/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/13/robot2_head',
				'/crowd/agents/atomsRobot/Robot2/14/robot2_body'
			}
		)

		self.assertTrue( "atomsRobot:Robot2:A" in set_names )
		self.assertTrue( "atomsRobot:Robot2:B" in set_names )

		self.assertTrue( "armsSet" in set_names )
		self.assertEqual(
			set( obj.set( "armsSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/arms/robot1_arms'

			}
		)

		self.assertTrue( "robotSet" in set_names )
		self.assertEqual(
			set( obj.set( "robotSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/arms/robot1_arms',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/legs/robot1_legs'
			}
		)

		self.assertTrue( "bodySet" in set_names )
		self.assertEqual(
			set( obj.set( "bodySet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/body/robot1_body',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/body/robot1_body'
			}
		)

		self.assertTrue( "initialShadingGroup" in set_names )
		self.assertEqual(
			set( obj.set( "initialShadingGroup" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/flag_group/pPlane1',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pPlane1'
			}
		)

		self.assertTrue( "phong3SG" in set_names )
		self.assertEqual(
			set( obj.set( "phong3SG" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/flag_group/pole',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/flag_group/pole'
			}
		)

		self.assertTrue( "headSet" in set_names )
		self.assertEqual(
			set( obj.set( "headSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/head/robot1_head',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/head/robot1_head'

			}
		)

		self.assertTrue( "legsSet" in set_names )
		self.assertEqual(
			set( obj.set( "legsSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/legs/robot1_legs'
			}
		)

		self.assertTrue( "fooSet" in set_names )
		self.assertEqual(
			set( obj.set( "fooSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/legs/robot1_legs'
			}
		)

		self.assertTrue( "newFooSet" in set_names )
		self.assertEqual(
			set( obj.set( "newFooSet" ).value.paths() ),
			{
				'/crowd/agents/atomsRobot/Robot1/24/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/10/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/17/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/0/RobotSkin1/legs/robot1_legs',
				'/crowd/agents/atomsRobot/Robot1/18/RobotSkin1/legs/robot1_legs'
			}
		)


if __name__ == "__main__":
	unittest.main()
