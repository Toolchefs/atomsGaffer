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

#ifndef ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H
#define ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneProcessor.h"

#include "Gaffer/StringPlug.h"
#include "Gaffer/NumericPlug.h"

namespace AtomsGaffer
{

// \todo: We may replace this node with an AtomsSceneInterface
// and load via the GafferScene::SceneReader. We're not doing
// that yet because we haven't decided what direction to go with
// regards to the various modes.
    class AtomsCrowdClothReader : public GafferScene::SceneProcessor
    {

    public:

        AtomsCrowdClothReader( const std::string &name = defaultName<AtomsCrowdClothReader>() );
        ~AtomsCrowdClothReader() = default;

        IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsCrowdClothReader, TypeId::AtomsCrowdClothReaderTypeId, GafferScene::SceneProcessor );

        Gaffer::StringPlug *atomsClothFilePlug();
        const Gaffer::StringPlug *atomsClothFilePlug() const;

        Gaffer::IntPlug *refreshCountPlug();
        const Gaffer::IntPlug *refreshCountPlug() const;

        void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

    protected:

        void hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
        IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

    private:

        void getAtomsCacheName( const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension ) const;

    private:

        static size_t g_firstPlugIndex;

    };

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H
