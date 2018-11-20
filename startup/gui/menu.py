import GafferUI

import AtomsGaffer
import AtomsGafferUI

nodeMenu = GafferUI.NodeMenu.acquire( application )
nodeMenu.append( "/AtomsGaffer/AtomsCrowdReader", AtomsGaffer.AtomsCrowdReader, searchText = "AtomsCrowdReader" )
nodeMenu.append( "/AtomsGaffer/AtomsVariationReader", AtomsGaffer.AtomsVariationReader, searchText = "AtomsVariationReader" )
nodeMenu.append( "/AtomsGaffer/AtomsCrowdGenerator", AtomsGaffer.AtomsCrowdGenerator, searchText = "AtomsCrowdGenerator" )
nodeMenu.append( "/AtomsGaffer/AtomsAttributes", AtomsGaffer.AtomsAttributes, searchText = "AtomsAttributes" )
