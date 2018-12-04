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
