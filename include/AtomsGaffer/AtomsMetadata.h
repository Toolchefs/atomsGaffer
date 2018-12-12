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

#ifndef ATOMSGAFFER_ATOMSMETADATA_H
#define ATOMSGAFFER_ATOMSMETADATA_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneProcessor.h"
#include "GafferScene/ScenePath.h"

#include "Gaffer/StringPlug.h"
#include "Gaffer/CompoundDataPlug.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECore/CompoundObject.h"

namespace AtomsGaffer
{

    class AtomsMetadata : public GafferScene::SceneProcessor
    {

    public :

        AtomsMetadata( const std::string &name = defaultName<AtomsMetadata>() );
        ~AtomsMetadata() = default;

        Gaffer::StringPlug *agentIdsPlug();
        const Gaffer::StringPlug *agentIdsPlug() const;

        Gaffer::BoolPlug *invertPlug();
        const Gaffer::BoolPlug *invertPlug() const;

        Gaffer::CompoundDataPlug *metadataPlug();
        const Gaffer::CompoundDataPlug *metadataPlug() const;

        IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsMetadata, TypeId::AtomsMetadataTypeId, GafferScene::SceneProcessor );
        void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

    protected :

        void hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
        IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

    private:

        void parseVisibleAgents( std::vector<int>& agentsFiltered, std::vector<int>& agentIds, const std::string& filter, bool invert = false ) const;

        template <typename T, typename OUT, typename IN>
        void setMetadataOnPoints(
                IECoreScene::PointsPrimitivePtr& primitive,
                const std::string& metadataName,
                const std::vector<int>& agentIdVec,
                const std::vector<int>& agentsFiltered,
                std::map<int, int>& agentIdPointsMapper,
                const T& data,
                const T& defaultValue,
                IECore::ConstCompoundObjectPtr& attributes
        ) const;

    private :

        static size_t g_firstPlugIndex;

    };

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSAMETADATA_H
