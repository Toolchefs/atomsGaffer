#include "boost/python.hpp"

#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsAgentReader.h"

#include "GafferBindings/DependencyNodeBinding.h"

using namespace boost::python;
using namespace GafferScene;

BOOST_PYTHON_MODULE( _AtomsGaffer )
{
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdReader>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsAgentReader>();
}
