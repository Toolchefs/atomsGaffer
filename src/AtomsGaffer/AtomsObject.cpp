//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Toolchefs Ltd. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

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
