import GafferUI

import AtomsGaffer
import AtomsGafferUI

nodeMenu = GafferUI.NodeMenu.acquire( application )
nodeMenu.append( "/AtomsGaffer/AtomsCrowdReader", AtomsGaffer.AtomsCrowdReader, searchText = "AtomsCrowdReader" )
nodeMenu.append( "/AtomsGaffer/AtomsAgentReader", AtomsGaffer.AtomsAgentReader, searchText = "AtomsAgentReader" )
