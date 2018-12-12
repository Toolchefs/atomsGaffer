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

import GafferTest
import GafferSceneTest

import AtomsGaffer

from AtomsTestData import buildCrowdTest

class AtomsMetadataTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsMetadata()
		self.assertEqual( a.getName(), "AtomsMetadata" )

	def testCompute( self ) :
		crowd_input = GafferSceneTest.CompoundObjectSource()
		crowd_input["in"].setValue( buildCrowdTest() )

		node = AtomsGaffer.AtomsMetadata()
		node["in"].setInput( crowd_input["out"] )

		node["metadata"].addMember( "tint", IECore.V3fData( imath.V3f( 1.0, 0.0, 0.0 ) ) )
		node["metadata"].addMember( "variation", IECore.StringData( "Robot1" ) )

		output = node["out"].object( "/crowd" )
		self.assertEqual( output.typeId(), IECoreScene.PointsPrimitive.staticTypeId() )

		self.assertTrue( "P" in output )
		self.assertTrue( "atoms:agentId" in output )
		self.assertTrue( "atoms:agentType" in output )
		self.assertTrue( "atoms:variation" in output )
		self.assertTrue( "atoms:lod" in output )
		self.assertTrue( "atoms:velocity" in output )
		self.assertTrue( "atoms:direction" in output )
		self.assertTrue( "atoms:scale" in output )
		self.assertTrue( "atoms:orientation" in output )
		self.assertTrue( "atoms:tint" in output )

		tint_data = output["atoms:tint"].data
		self.assertEqual( len(tint_data), 4 )
		self.assertEqual( tint_data[0], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[1], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[2], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[3], imath.V3f( 1.0, 0.0, 0.0 ) )

		variation_data = output["atoms:variation"].data
		self.assertEqual( len(variation_data), 4 )
		self.assertEqual( variation_data[0], "Robot1" )
		self.assertEqual( variation_data[1], "Robot1" )
		self.assertEqual( variation_data[2], "Robot1" )
		self.assertEqual( variation_data[3], "Robot1" )

	def testFilterCompute( self ) :
		crowd_input = GafferSceneTest.CompoundObjectSource()
		crowd_input["in"].setValue( buildCrowdTest() )

		node = AtomsGaffer.AtomsMetadata()
		node["in"].setInput( crowd_input["out"] )
		node["agentIds"].setValue( "1-2")
		node["metadata"].addMember( "tint", IECore.V3fData( imath.V3f( 1.0, 0.0, 0.0 ) ) )
		node["metadata"].addMember( "variation", IECore.StringData( "Robot3" ) )
		node["metadata"].addMember( "testData", IECore.IntData( 1 ) )

		output = node["out"].object( "/crowd" )
		self.assertEqual( output.typeId(), IECoreScene.PointsPrimitive.staticTypeId() )

		tint_data = output["atoms:tint"].data
		self.assertEqual( len(tint_data), 4 )
		self.assertNotEqual( tint_data[0], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[1], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[2], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertNotEqual( tint_data[3], imath.V3f( 1.0, 0.0, 0.0 ) )

		variation_data = output["atoms:variation"].data
		self.assertEqual( len(variation_data), 4 )
		self.assertNotEqual( variation_data[0], "Robot3" )
		self.assertEqual( variation_data[1], "Robot3" )
		self.assertEqual( variation_data[2], "Robot3" )
		self.assertNotEqual( variation_data[3], "Robot3" )

		test_data = output["atoms:testData"].data
		self.assertEqual( len(test_data), 4 )
		self.assertEqual( test_data[0], 2 )
		self.assertEqual( test_data[1], 1 )
		self.assertEqual( test_data[2], 1 )
		self.assertEqual( test_data[3], 2 )

		node["agentIds"].setValue( "!1-2" )
		output = node["out"].object( "/crowd" )
		self.assertEqual( output.typeId(), IECoreScene.PointsPrimitive.staticTypeId() )

		tint_data = output["atoms:tint"].data
		self.assertEqual( len(tint_data), 4 )
		self.assertEqual( tint_data[0], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertNotEqual( tint_data[1], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertNotEqual( tint_data[2], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[3], imath.V3f( 1.0, 0.0, 0.0 ) )

		variation_data = output["atoms:variation"].data
		self.assertEqual( len(variation_data), 4 )
		self.assertEqual( variation_data[0], "Robot3" )
		self.assertNotEqual( variation_data[1], "Robot3" )
		self.assertNotEqual( variation_data[2], "Robot3" )
		self.assertEqual( variation_data[3], "Robot3" )

		node["invert"].setValue( True )
		output = node["out"].object( "/crowd" )

		tint_data = output["atoms:tint"].data
		self.assertEqual( len(tint_data), 4 )
		self.assertNotEqual( tint_data[0], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[1], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertEqual( tint_data[2], imath.V3f( 1.0, 0.0, 0.0 ) )
		self.assertNotEqual( tint_data[3], imath.V3f( 1.0, 0.0, 0.0 ) )

		variation_data = output["atoms:variation"].data
		self.assertEqual( len(variation_data), 4 )
		self.assertNotEqual( variation_data[0], "Robot3" )
		self.assertEqual( variation_data[1], "Robot3" )
		self.assertEqual( variation_data[2], "Robot3" )
		self.assertNotEqual( variation_data[3], "Robot3" )


if __name__ == "__main__":
	unittest.main()
