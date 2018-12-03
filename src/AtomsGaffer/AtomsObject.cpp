#include "AtomsGaffer/AtomsObject.h"

using namespace IECore;
using namespace AtomsGaffer;

IE_CORE_DEFINEOBJECTTYPEDESCRIPTION(AtomsObject);

const unsigned int AtomsObject::m_ioVersion = 1;

AtomsObject::AtomsObject()
{
}

AtomsObject::AtomsObject(IECore::CompoundDataPtr data): BlindDataHolder(data)
{
}

AtomsObject::~AtomsObject()
{
}

void AtomsObject::copyFrom( const Object *other, CopyContext *context )
{
    BlindDataHolder::copyFrom( other, context );
}

void AtomsObject::save( SaveContext *context ) const
{
    BlindDataHolder::save( context );
}

void AtomsObject::load( LoadContextPtr context )
{
    BlindDataHolder::load( context );
}

bool AtomsObject::isEqualTo( const Object *other ) const
{
    if( !BlindDataHolder::isEqualTo( other ) )
    {
        return false;
    }
    return true;
}

void AtomsObject::memoryUsage( Object::MemoryAccumulator &a ) const
{
    BlindDataHolder::memoryUsage( a );
}

void AtomsObject::hash( MurmurHash &h ) const
{
    BlindDataHolder::hash( h );
}
