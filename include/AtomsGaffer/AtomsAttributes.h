#ifndef ATOMSGAFFER_ATOMSATTRIBUTES_H
#define ATOMSGAFFER_ATOMSATTRIBUTES_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/Attributes.h"

#include "Gaffer/StringPlug.h"

namespace AtomsGaffer
{

    class AtomsAttributes : public GafferScene::Attributes
    {

    public:

        AtomsAttributes( const std::string &name = defaultName<AtomsAttributes>() );
        ~AtomsAttributes() = default;

        IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsAttributes, TypeId::AtomsAttributesTypeId, GafferScene::Attributes );

    };

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSATTRIBUTES_H