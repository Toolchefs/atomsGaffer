import unittest
import imath

import IECore
import IECoreScene

import GafferTest
import GafferSceneTest

import AtomsGaffer

class AtomsAttributesTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsAttributes()
		self.assertEqual( a.getName(), "AtomsAttributes" )

	def testCompute( self ) :

		raise RuntimeError, "Write tests for AtomsAttributes"

if __name__ == "__main__":
	unittest.main()
