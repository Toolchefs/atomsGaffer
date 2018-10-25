import os

import AtomsGaffer

def documentationURL( node ) :

	fallback = "$ATOMS_ROOT/maya/docs/html/index.html"
	fileName = "$ATOMS_ROOT/gaffer/docs/html/NodeReference/{nodePath}.html".format(
		nodePath = node.typeName().replace( "::", "/" )
	)
	fileName = os.path.expandvars( fileName )
	return "file://" + fileName if os.path.isfile( fileName ) else fallback
