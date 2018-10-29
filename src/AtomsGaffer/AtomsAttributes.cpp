#include "AtomsGaffer/AtomsAttributes.h"

IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsAttributes );

using namespace IECore;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

AtomsAttributes::AtomsAttributes( const std::string &name )
	:	Attributes( name )
{
	Gaffer::CompoundDataPlug *attributes = attributesPlug();

	// Agent specific parameters

	attributes->addOptionalMember( "atoms:agent:type", new StringData(), "agentType", Gaffer::Plug::Default, false );

	// Variation parameters

	attributes->addOptionalMember( "atoms:variation:tint", new Color3fData(), "tint", Gaffer::Plug::Default, false );

}
