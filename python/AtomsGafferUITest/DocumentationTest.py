import Gaffer
import GafferUI
import GafferScene
import GafferSceneUI

import AtomsGaffer
import AtomsGafferUI

import GafferUITest

class DocumentationTest( GafferUITest.TestCase ) :

	def test( self ) :

		self.maxDiff = None
		self.assertNodesAreDocumented(
			AtomsGaffer,
			additionalTerminalPlugTypes = ( GafferScene.ScenePlug, )
		)

if __name__ == "__main__":
	unittest.main()
