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

#include "AtomsGaffer/AtomsCrowdClothReader.h"
#include "AtomsGaffer/AtomsObject.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECore/NullObject.h"

#include "AtomsUtils/PathSolver.h"
#include "AtomsUtils/Utils.h"

#include "Atoms/AtomsClothCache.h"
#include "Atoms/GlobalNames.h"


IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsCrowdClothReader );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

size_t AtomsCrowdClothReader::g_firstPlugIndex = 0;

AtomsCrowdClothReader::AtomsCrowdClothReader( const std::string &name )
        :	SceneProcessor( name )
{
    storeIndexOfNextChild( g_firstPlugIndex );

    addChild( new StringPlug( "atomsClothFile" ) );
    addChild( new IntPlug( "refreshCount" ) );

    outPlug()->attributesPlug()->setInput( inPlug()->attributesPlug() );
    outPlug()->transformPlug()->setInput( inPlug()->transformPlug() );
    outPlug()->boundPlug()->setInput( inPlug()->boundPlug() );
    outPlug()->childNamesPlug()->setInput( inPlug()->childNamesPlug() );
    outPlug()->setNamesPlug()->setInput( inPlug()->setNamesPlug() );
    outPlug()->setPlug()->setInput( inPlug()->setPlug() );
    outPlug()->globalsPlug()->setInput( inPlug()->globalsPlug() );
}

Gaffer::StringPlug *AtomsCrowdClothReader::atomsClothFilePlug()
{
    return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *AtomsCrowdClothReader::atomsClothFilePlug() const
{
    return getChild<StringPlug>( g_firstPlugIndex );
}

IntPlug* AtomsCrowdClothReader::refreshCountPlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const IntPlug* AtomsCrowdClothReader::refreshCountPlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

void AtomsCrowdClothReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
    if( input == inPlug()->objectPlug() || input == inPlug()->attributesPlug() ||
        input == atomsClothFilePlug() || input == refreshCountPlug() )
    {
        outputs.push_back( outPlug()->objectPlug() );
    }
}

void AtomsCrowdClothReader::hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const
{
    inPlug()->objectPlug()->hash( h );
    inPlug()->attributesPlug()->hash( h );
    atomsClothFilePlug()->hash( h );
    refreshCountPlug()->hash( h );
    h.append( context->getFrame() );
}

ConstObjectPtr AtomsCrowdClothReader::computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const
{
    const std::vector<int>* agentIdVec = nullptr;
    float frameOffset = 0.0;

    auto crowd = runTimeCast<const IECoreScene::PointsPrimitive>( inPlug()->objectPlug()->getValue() );
    if ( crowd )
    {
        const auto agentId = crowd->variables.find( "atoms:agentId" );
        if( agentId != crowd->variables.end() )
        {
            auto agentIdData = runTimeCast<const IntVectorData>(agentId->second.data);
            if ( agentIdData )
            {
                agentIdVec = &agentIdData->readable();
            }
        }
    }

    auto crowdAttributes = runTimeCast<const IECore::CompoundObject>( inPlug()->attributesPlug()->getValue() );
    if ( crowdAttributes )
    {
        auto& crowdAttributesData = crowdAttributes->members();
        auto attrIt = crowdAttributesData.find( "frameOffset" );
        if ( attrIt != crowdAttributesData.cend() )
        {
            auto frameOffsetData = runTimeCast<const FloatData>( attrIt->second );
            if ( frameOffsetData )
                frameOffset = frameOffsetData->readable();
        }
    }

    Atoms::AtomsClothCache cache;
    std::string cachePath, cacheName;
    std::string filePath = atomsClothFilePlug()->getValue();
    double frame = context->getFrame() + frameOffset;

    getAtomsCacheName( filePath, cachePath, cacheName, "clothcache" );

    if( !cache.openCache( cachePath, cacheName ) )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdReader", "Unable to load the atoms cache " + cachePath + "/" + cacheName + ".atoms" );
        return NullObject::defaultNullObject();
    }

    frame = frame < cache.startFrame() ? cache.startFrame() : frame;
    frame = frame > cache.endFrame() ? cache.endFrame() : frame;

    int cacheFrame = static_cast<int>( frame );
    double frameReminder = frame - cacheFrame;

    if ( agentIdVec )
    {
        cache.setAgentsToLoad( *agentIdVec );
    }

    cache.loadFrame( cacheFrame );
    if ( frameReminder > 0.0 )
        cache.loadNextFrame( cacheFrame + 1 );

    auto agentIds = cache.agentIds( cacheFrame );

    CompoundDataPtr clothCompound = new CompoundData;
    auto& clothData = clothCompound->writable();
    for (auto agentId: agentIds )
    {
        CompoundDataPtr agentCompound = new CompoundData;
        auto& agentData = agentCompound->writable();

        std::vector<std::string> meshNames;

        std::string agentIdStr = std::to_string(agentId);

        auto frameData = cache.frameData();
        if ( frameData )
        {
            AtomsPtr<AtomsCore::MapMetadata> agentClothData = frameData->getTypedEntry<AtomsCore::MapMetadata>( agentIdStr );
            if ( !agentClothData )
            {
                agentClothData = frameData->getTypedEntry<AtomsCore::MapMetadata>("*");
            }
            if ( agentClothData )
            {
                meshNames = agentClothData->getKeys();
            }
        }

        for( const auto& meshName: meshNames )
        {
            CompoundDataPtr meshCompound = new CompoundData;
            auto& meshData = meshCompound->writable();

            V3dVectorDataPtr p = new V3dVectorData;
            V3dVectorDataPtr n = new V3dVectorData;
            Box3dDataPtr bbox = new Box3dData;
            StringDataPtr stackOrder = new StringData;

            cache.loadAgentClothMesh( frame, agentId, meshName, p->writable(), n->writable() );
            cache.loadAgentClothMeshBoundingBox( frame, agentId, meshName, bbox->writable() );
            stackOrder->writable() = cache.getAgentClothMeshStackOrder( frame, agentId, meshName );

            meshData[ "P" ] = p;
            meshData[ "N" ] = n;
            meshData[ "boundingBox" ] = bbox;
            meshData[ "stackOrder" ] = stackOrder;

            agentData[ meshName ] = meshCompound;
        }

        Box3dDataPtr bbox = new Box3dData;
        cache.loadAgentClothBoundingBox( frame, agentId, bbox->writable() );
        agentData[ "boundingBox" ] = bbox;

        clothData[ agentIdStr ] = agentCompound;
    }

    AtomsObjectPtr result = new AtomsObject( clothCompound );
    return result;
}

void AtomsCrowdClothReader::getAtomsCacheName( const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension ) const
{
    size_t found = filePath.find_last_of( "/\\" );
    std::string folderPath = filePath.substr( 0, found );

    cachePath = filePath;
    std::string fullPath = cachePath;
    fullPath = AtomsUtils::solvePath( fullPath );
    std::replace( fullPath.begin(), fullPath.end(), '\\', '/' );

    const size_t lastSlashIdx = fullPath.rfind( '/' );
    if ( std::string::npos != lastSlashIdx )
    {

        cacheName = fullPath.substr( lastSlashIdx + 1, fullPath.length() );
        const size_t lastDotIdx = cacheName.rfind( '.' );
        if ( !( std::string::npos == lastDotIdx || cacheName.substr( lastDotIdx + 1, cacheName.length() ) != extension ) )
        {
            const size_t firstDotIdx = cacheName.find( '.' );
            cacheName = cacheName.substr( 0, firstDotIdx );
            cachePath = fullPath.substr( 0, lastSlashIdx );
        }
        else
        {
            cacheName = "";
        }
    }
}
