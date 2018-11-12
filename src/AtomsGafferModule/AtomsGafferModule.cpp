#include "boost/python.hpp"

#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsAgentReader.h"
#include "AtomsGaffer/AtomsCrowdGenerator.h"
#include "AtomsGaffer/AtomsAttributes.h"

#include "GafferBindings/DependencyNodeBinding.h"

#include "Atoms/Initialize.h"

using namespace boost::python;
using namespace GafferScene;

BOOST_PYTHON_MODULE( _AtomsGaffer )
{
	// Initialize atoms lib
	Atoms::initAtoms();

	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdReader>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsAgentReader>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdGenerator>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsAttributes>();
}
