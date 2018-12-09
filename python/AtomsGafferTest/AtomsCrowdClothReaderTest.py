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

class AtomsCrowdClothReaderTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsCrowdClothReader()
		self.assertEqual( a.getName(), "AtomsCrowdClothReader" )

	def testCompute( self ) :

		crowd_input = AtomsGaffer.AtomsCrowdReader()
		crowd_input["atomsSimFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cache/test_sim.atoms" )


		cloth = AtomsGaffer.AtomsCrowdClothReader()
		cloth["atomsClothFile"].setValue( "${ATOMS_GAFFER_ROOT}/examples/assets/atomsRobot/cloth_cache/cloth_sim.clothcache" )
		cloth["in"].setInput( crowd_input["out"] )

		obj = cloth["out"].object( "/crowd" )
		self.assertEqual( obj.typeName(), IECore.BlindDataHolder.staticTypeName() )
		data = obj.blindData()
		self.assertTrue( "0" in data )
		self.assertTrue( "10" in data )
		self.assertTrue( "17" in data )

		agent_data = data["0"]
		self.assertTrue( "flag" in agent_data )

		flag_data = agent_data["flag"]
		self.assertTrue( "P" in flag_data )
		self.assertEqual( len(flag_data["P"]), 121 )
		self.assertTrue( "N" in flag_data )
		self.assertEqual( len(flag_data["N"]), 121 )
		self.assertTrue( "stackOrder" in flag_data )
		self.assertEqual( flag_data["stackOrder"].value, "last" )
		self.assertTrue( "boundingBox" in flag_data )
		self.assertAlmostEqual( flag_data["boundingBox"].value.min(), imath.V3d( -302.4302978515625, -0.043552137911319733, -367.09475708007812 ) )
		self.assertAlmostEqual( flag_data["boundingBox"].value.max(), imath.V3d( -270.42013549804688, 32.08740234375, -297.2152099609375 ) )


if __name__ == "__main__":
	unittest.main()
