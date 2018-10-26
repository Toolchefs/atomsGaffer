import unittest
import imath

import IECore
import IECoreScene

import GafferTest
import GafferSceneTest

import AtomsGaffer

class AtomsCrowdGeneratorTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsCrowdGenerator()
		self.assertEqual( a.getName(), "AtomsCrowdGenerator" )

	def testCompute( self ) :

		raise RuntimeError, "Write tests for AtomsCrowdGenerator"

if __name__ == "__main__":
	unittest.main()
