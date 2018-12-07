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

		a = AtomsGaffer.AtomsCrowdReader()

		self.assertEqual( a["out"].object( "/" ), IECore.NullObject() )
		self.assertEqual( a["out"].transform( "/" ), imath.M44f() )
		self.assertEqual( a["out"].bound( "/" ), imath.Box3f( imath.V3f( -0.5 ), imath.V3f( 9.5, 0.5, 0.5 ) ) )
		self.assertEqual( a["out"].childNames( "/" ), IECore.InternedStringVectorData( [ "crowd" ] ) )

		# expected result
		points = IECoreScene.PointsPrimitive( IECore.V3fVectorData( [ imath.V3f( x, 0, 0 ) for x in range( 0, 10 ) ], IECore.GeometricData.Interpretation.Point ) )
		points["agentType"] = IECoreScene.PrimitiveVariable(
			IECoreScene.PrimitiveVariable.Interpolation.Vertex,
			IECore.StringVectorData( [ "type 0", "type 1", "type 2" ] ),
			IECore.IntVectorData( [ x % 3 for x in range( 0, 10 ) ] ),
		)

		self.assertEqual( a["out"].object( "/crowd" ), points )
		self.assertEqual( a["out"].transform( "/crowd" ), imath.M44f() )
		self.assertEqual( a["out"].bound( "/crowd" ), imath.Box3f( imath.V3f( -0.5, -0.5, -0.5 ), imath.V3f( 9.5, 0.5, 0.5 ) ) )
		self.assertEqual( a["out"].childNames( "/crowd" ), IECore.InternedStringVectorData() )

	def testAffects( self ) :

		a = AtomsGaffer.AtomsCrowdReader()

		ss = GafferTest.CapturingSlot( a.plugDirtiedSignal() )

		a["name"].setValue( "army" )
		self.assertEqual( len( ss ), 4 )
		self.failUnless( ss[0][0].isSame( a["name"] ) )
		self.failUnless( ss[1][0].isSame( a["out"]["childNames"] ) )
		self.failUnless( ss[2][0].isSame( a["out"]["set"] ) )
		self.failUnless( ss[3][0].isSame( a["out"] ) )

		del ss[:]

		a["atomsSimFile"].setValue( "/path/to/file" )
		found = False
		for sss in ss :
			if sss[0].isSame( a["out"] ) :
				found = True
		self.failUnless( found )

	def testEnabled( self ) :

		a = AtomsGaffer.AtomsCrowdReader()
		a["enabled"].setValue( False )

		self.assertSceneValid( a["out"] )
		self.assertTrue( a["out"].bound( "/" ).isEmpty() )
		self.assertEqual( a["out"].childNames( "/" ), IECore.InternedStringVectorData() )

		a["enabled"].setValue( True )
		self.assertSceneValid( a["out"] )
		self.assertEqual( a["out"].childNames( "/" ), IECore.InternedStringVectorData( [ "crowd" ] ) )

	def testChildNamesHash( self ) :

		s1 = AtomsGaffer.AtomsCrowdReader()
		s1["name"].setValue( "crowd1" )

		s2 = AtomsGaffer.AtomsCrowdReader()
		s2["name"].setValue( "crowd2" )

		self.assertNotEqual( s1["out"].childNamesHash( "/" ), s2["out"].childNamesHash( "/" ) )
		self.assertEqual( s1["out"].childNamesHash( "/crowd1" ), s2["out"].childNamesHash( "/crowd2" ) )

		self.assertNotEqual( s1["out"].childNames( "/" ), s2["out"].childNames( "/" ) )
		self.assertTrue( s1["out"].childNames( "/crowd1", _copy=False ).isSame( s2["out"].childNames( "crowd2", _copy=False ) ) )

	def testTransformAffectsBound( self ) :

		a = AtomsGaffer.AtomsCrowdReader()
		self.assertTrue( a["out"]["bound"] in a.affects( a["transform"]["translate"]["x"] ) )

if __name__ == "__main__":
	unittest.main()
