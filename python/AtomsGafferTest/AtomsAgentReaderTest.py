import unittest
import imath

import IECore
import IECoreScene

import GafferTest
import GafferSceneTest

import AtomsGaffer

class AtomsAgentReaderTest( GafferSceneTest.SceneTestCase ) :

	def testConstruct( self ) :

		a = AtomsGaffer.AtomsAgentReader()
		self.assertEqual( a.getName(), "AtomsAgentReader" )

	def testCompute( self ) :

		raise RuntimeError, "Write tests for AtomsAgentReader"

if __name__ == "__main__":
	unittest.main()
