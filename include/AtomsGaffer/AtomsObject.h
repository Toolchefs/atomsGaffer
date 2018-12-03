#ifndef ATOMSGAFFER_ATOMSOBJECT_H
#define ATOMSGAFFER_ATOMSOBJECT_H

#include "AtomsGaffer/TypeIds.h"

#include "IECore/BlindDataHolder.h"

namespace AtomsGaffer {

    class AtomsObject : public IECore::BlindDataHolder
    {
        public:

            AtomsObject();
            AtomsObject(IECore::CompoundDataPtr data);
            ~AtomsObject() override;

            IE_CORE_DECLAREEXTENSIONOBJECT( AtomsObject, TypeId::AtomsObjectTypeId, IECore::BlindDataHolder );

        private:

            static const unsigned int m_ioVersion;
    };

    IE_CORE_DECLAREPTR( AtomsObject );

}

#endif