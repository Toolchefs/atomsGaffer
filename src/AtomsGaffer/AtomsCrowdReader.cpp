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

#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECore/NullObject.h"
#include "IECore/BlindDataHolder.h"

#include "AtomsUtils/PathSolver.h"
#include "AtomsUtils/Utils.h"

#include "Atoms/AtomsCache.h"
#include "Atoms/GlobalNames.h"

#include "AtomsCore/Metadata/MetadataTypeIds.h"
#include "AtomsCore/Metadata/IntMetadata.h"
#include "AtomsCore/Metadata/BoolMetadata.h"
#include "AtomsCore/Metadata/DoubleMetadata.h"
#include "AtomsCore/Metadata/StringMetadata.h"
#include "AtomsCore/Metadata/QuaternionMetadata.h"
#include "AtomsCore/Metadata/Box3Metadata.h"
#include "AtomsCore/Metadata/Vector3Metadata.h"
#include "AtomsCore/Metadata/Vector2Metadata.h"
#include "AtomsCore/Metadata/MatrixMetadata.h"
#include "AtomsCore/Metadata/IntArrayMetadata.h"
#include "AtomsCore/Metadata/BoolArrayMetadata.h"
#include "AtomsCore/Metadata/DoubleArrayMetadata.h"
#include "AtomsCore/Metadata/StringArrayMetadata.h"
#include "AtomsCore/Metadata/QuaternionArrayMetadata.h"
#include "AtomsCore/Metadata/Vector3ArrayMetadata.h"
#include "AtomsCore/Metadata/Vector2ArrayMetadata.h"
#include "AtomsCore/Metadata/MatrixArrayMetadata.h"
#include "AtomsCore/Metadata/PoseMetadata.h"
#include "AtomsCore/Poser.h"



IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsCrowdReader );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

class AtomsCrowdReader::EngineData : public Data
{

public :

    EngineData( const std::string& filePath, float frame, const std::string& agentIdsStr ):
            m_filePath( filePath ),
            m_frame( frame ),
            m_memorySize( 0 )
    {
        m_frame = frame;

        if ( filePath.empty() )
            return;

        std::string cachePath, cacheName;
        getAtomsCacheName( filePath, cachePath, cacheName, "atoms" );

        if( !m_cache.openCache( cachePath, cacheName ) )
        {
            IECore::msg( IECore::Msg::Warning, "AtomsCrowdReader", "Unable to load the atoms cache " + cachePath + "/" + cacheName + ".atoms" );
            return;
        }

        // Clamp the frame
        m_frame = m_frame < m_cache.startFrame() ? m_cache.startFrame() : m_frame;
        m_frame = m_frame > m_cache.endFrame() ? m_cache.endFrame() : m_frame;

        int cacheFrame = static_cast<int>( m_frame );
        double frameReminder = m_frame - cacheFrame;

        // Load the frame haeder that contains the agent ids
        m_cache.loadFrameHeader( cacheFrame );

        // filter agents
        std::vector<int> agentsIds;
        // Filter the agnet id based on the input expression
        parseVisibleAgents( agentsIds, AtomsUtils::eraseFromString( agentIdsStr, ' ' ) );
        if ( !agentsIds.empty() ) {
            m_cache.setAgentsToLoad( agentsIds );
            m_agentIds = agentsIds;
        }
        else
        {
            m_agentIds = m_cache.agentIds( cacheFrame );
        }

        // Load the pose and metadata
        m_cache.loadFrame( cacheFrame );
        if ( frameReminder > 0.0 )
            m_cache.loadNextFrame( cacheFrame + 1 );

        // Load the agent types in memory
        for( size_t i = 0; i < m_agentIds.size(); ++i )
        {
            int agentId = m_agentIds[i];
            // load in memory the agent type since you need the skeleton to extract the world matrices from the pose
            const std::string &agentTypeName = m_cache.agentType( m_frame, agentId );
            m_cache.loadAgentType( agentTypeName, false );
        }

        if ( m_cache.prevFrameData().frame )
            m_memorySize += m_cache.prevFrameData().frame->memSize();
        if ( m_cache.prevFrameData().header )
            m_memorySize += m_cache.prevFrameData().header->memSize();
        if ( m_cache.prevFrameData().metadata )
            m_memorySize += m_cache.prevFrameData().metadata->memSize();
        if ( m_cache.prevFrameData().pose )
            m_memorySize += m_cache.prevFrameData().pose->memSize();

        if ( m_cache.frameData().frame )
            m_memorySize +=  m_cache.frameData().frame->memSize();
        if ( m_cache.frameData().header )
            m_memorySize += m_cache.frameData().header->memSize();
        if ( m_cache.frameData().metadata )
            m_memorySize +=  m_cache.frameData().metadata->memSize();
        if ( m_cache.frameData().pose )
            m_memorySize +=  m_cache.frameData().pose->memSize();

        if ( m_cache.nextFrameData().frame )
            m_memorySize +=  m_cache.nextFrameData().frame->memSize();
        if ( m_cache.nextFrameData().header )
            m_memorySize += m_cache.nextFrameData().header->memSize();
        if ( m_cache.nextFrameData().metadata )
            m_memorySize +=  m_cache.nextFrameData().metadata->memSize();
        if ( m_cache.nextFrameData().pose )
            m_memorySize += m_cache.nextFrameData().pose->memSize();

        auto& agentTypes = m_cache.agentTypes();
        for ( const auto& agentTypeName: agentTypes.agentTypeNames() )
        {
            auto agentType = agentTypes.agentType(agentTypeName);
            if ( !agentType )
                continue;

            m_memorySize += agentType->memSize();
        }
    }

    virtual ~EngineData()
    {

    }

    void hash( MurmurHash &h ) const override
    {
        h.append( m_filePath );
        h.append( m_frame );
    }

    double frame() const
    {
        return m_frame;
    }

    const std::vector<int>& agentIds() const
    {
        return m_agentIds;
    }

    const Atoms::AtomsCache& cache() const
    {
        return m_cache;
    }

    void getAtomsCacheName( const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension ) const
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

    void parseVisibleAgents( std::vector<int>& agentsFiltered, const std::string& agentIdsStr )
    {
        // The input filter accept '-' to set a range of ids, and ! to exclude an id or a range of ids
        std::vector<std::string> idsEntryStr;
        AtomsUtils::splitString( agentIdsStr, ',', idsEntryStr );
        std::vector<int> idsToRemove;
        std::vector<int> idsToAdd;

        std::vector<int> agentIds = m_cache.agentIds( m_cache.currentFrame() );
        std::sort( agentIds.begin(), agentIds.end() );
        for ( unsigned int eId = 0; eId < idsEntryStr.size(); eId++ )
        {
            std::string currentStr = idsEntryStr[eId];
            currentStr = AtomsUtils::eraseFromString( currentStr, ' ' );
            if ( currentStr.empty() )
            {
                continue;
            }

            std::vector<int>* currentIdArray = &idsToAdd;
            // is a remove id
            if (currentStr[0] == '!')
            {
                currentIdArray = &idsToRemove;
                currentStr = currentStr.substr(1, currentStr.length());
            }

            //remove all the parenthesis
            currentStr = AtomsUtils::eraseFromString( currentStr, '(' );
            currentStr = AtomsUtils::eraseFromString( currentStr, ')' );

            // is a range
            if ( currentStr.find('-') != std::string::npos )
            {
                std::vector<std::string> rangeEntryStr;
                AtomsUtils::splitString( currentStr, '-', rangeEntryStr );

                int startRange = 0;
                int endRange = 0;
                if ( !rangeEntryStr.empty() )
                {
                    startRange = atoi( rangeEntryStr[0].c_str() );

                    if (rangeEntryStr.size() > 1)
                    {
                        endRange = atoi( rangeEntryStr[1].c_str() );
                        if ( startRange < endRange )
                        {
                            for ( int rId = startRange; rId <= endRange; rId++ )
                            {
                                currentIdArray->push_back(rId);
                            }
                        }
                    }
                }
            }
            else
            {
                int tmpId = atoi( currentStr.c_str() );
                currentIdArray->push_back( tmpId );
            }
        }

        agentsFiltered.clear();
        std::sort( idsToAdd.begin(), idsToAdd.end() );
        std::sort( idsToRemove.begin(), idsToRemove.end() );
        if ( !idsToAdd.empty() )
        {
            std::vector<int> validIds;
            std::set_intersection(
                    agentIds.begin(), agentIds.end(),
                    idsToAdd.begin(), idsToAdd.end(),
                    std::back_inserter( validIds )
            );


            std::set_difference(
                    validIds.begin(), validIds.end(),
                    idsToRemove.begin(), idsToRemove.end(),
                    std::back_inserter( agentsFiltered )
            );
        }
        else
        {
            std::set_difference(
                    agentIds.begin(), agentIds.end(),
                    idsToRemove.begin(), idsToRemove.end(),
                    std::back_inserter( agentsFiltered )
            );
        }
    }

protected :

    void copyFrom( const Object *other, CopyContext *context ) override
    {
        Data::copyFrom( other, context );
        msg( Msg::Warning, "EngineData::copyFrom", "Not implemented" );
    }

    void save( SaveContext *context ) const override
    {
        Data::save( context );
        msg( Msg::Warning, "EngineData::save", "Not implemented" );
    }

    void load( LoadContextPtr context ) override
    {
        Data::load( context );
        msg( Msg::Warning, "EngineData::load", "Not implemented" );
    }


    virtual void memoryUsage( Object::MemoryAccumulator &accumulator ) const
    {
        accumulator.accumulate( m_memorySize );
    }


private :

    Atoms::AtomsCache m_cache;

    std::string m_filePath;

    std::vector<int> m_agentIds;

    float m_frame;

    size_t m_memorySize;
};

size_t AtomsCrowdReader::g_firstPlugIndex = 0;

AtomsCrowdReader::AtomsCrowdReader( const std::string &name )
	:	ObjectSource( name, "crowd" )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsSimFile" ) );
    addChild( new StringPlug( "agentIds" ) );
    addChild( new FloatPlug( "timeOffset" ) );
	addChild( new IntPlug( "refreshCount" ) );
    addChild( new ObjectPlug( "__engine", Plug::Out, NullObject::defaultNullObject() ) );
}

StringPlug* AtomsCrowdReader::atomsSimFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsCrowdReader::atomsSimFilePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::StringPlug *AtomsCrowdReader::agentIdsPlug()
{
return getChild<StringPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::StringPlug *AtomsCrowdReader::agentIdsPlug() const
{
    return getChild<StringPlug>( g_firstPlugIndex  + 1 );
}

Gaffer::FloatPlug *AtomsCrowdReader::timeOffsetPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex  + 2 );
}

const Gaffer::FloatPlug *AtomsCrowdReader::timeOffsetPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex  + 2 );
}

IntPlug* AtomsCrowdReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

const IntPlug* AtomsCrowdReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

Gaffer::ObjectPlug *AtomsCrowdReader::enginePlug()
{
    return getChild<ObjectPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::ObjectPlug *AtomsCrowdReader::enginePlug() const
{
    return getChild<ObjectPlug>( g_firstPlugIndex + 4 );
}

void AtomsCrowdReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{

	ObjectSource::affects( input, outputs );

	if( input == atomsSimFilePlug() || input == refreshCountPlug() ||
	    input == agentIdsPlug() || input == timeOffsetPlug() )
    {
	    outputs.push_back( enginePlug() );
    }

	if ( input == enginePlug() )
	{
        outputs.push_back( sourcePlug() );
        outputs.push_back( outPlug()->attributesPlug() );
	}
}

void AtomsCrowdReader::hashSource( const Gaffer::Context *context, MurmurHash &h ) const
{
	atomsSimFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
	timeOffsetPlug()->hash( h );
    agentIdsPlug()->hash( h );
	outPlug()->attributesPlug()->hash( h );
	h.append( context->getFrame() );
}

ConstObjectPtr AtomsCrowdReader::computeSource( const Gaffer::Context *context ) const
{
    // The compute sorce generate a point cloud. Every point contains
    // the agentId, agentType, variation, lod, velocity, direction, orientation and scale ad prim var with "atoms:" prefix
    // This data can be manipulated after this node and before the crowd generator
    ConstEngineDataPtr engineData = boost::static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        PointsPrimitivePtr points = new PointsPrimitive( );
        return points;
    }

    double frame = engineData->frame();
    auto& atomsCache = engineData->cache();
    auto& agentIds = engineData->agentIds();
    size_t numAgents = agentIds.size();


    V3fVectorDataPtr positionData = new V3fVectorData;
    positionData->setInterpretation( IECore::GeometricData::Interpretation::Point );
    auto &positions = positionData->writable();
    positions.resize( numAgents );

    IntVectorDataPtr agentCacheIdsData = new IntVectorData;
    agentCacheIdsData->writable() = agentIds;

	StringVectorDataPtr agentCacheIdsStrData = new StringVectorData;
	auto &agentCacheIdsStr = agentCacheIdsStrData->writable();
	agentCacheIdsStr.resize( numAgents );

    StringVectorDataPtr agentTypesData = new StringVectorData;
    auto &agentTypes = agentTypesData->writable();
    agentTypes.resize( numAgents );

    StringVectorDataPtr agentVariationData = new StringVectorData;
    auto &agentVariations = agentVariationData->writable();
    agentVariations.resize( numAgents );

    StringVectorDataPtr agentLodData = new StringVectorData;
    auto &agentLods = agentLodData->writable();
    agentLods.resize( numAgents );

    V3fVectorDataPtr velocityData = new V3fVectorData;
    auto &velocity = velocityData->writable();
    velocity.resize( numAgents );

    V3fVectorDataPtr directionData = new V3fVectorData;
    auto &direction = directionData->writable();
    direction.resize( numAgents );

    V3fVectorDataPtr scaleData = new V3fVectorData;
    auto &scale = scaleData->writable();
    scale.resize( numAgents );

    Box3fVectorDataPtr boundingBoxData = new Box3fVectorData;
    auto &boundingBox = boundingBoxData->writable();
    boundingBox.resize( numAgents );

    M44fVectorDataPtr rootMatrixData = new M44fVectorData;
    auto &rootMatrix = rootMatrixData->writable();
    rootMatrix.resize( numAgents );

    QuatfVectorDataPtr orientationData = new QuatfVectorData;
    auto &orientation = orientationData->writable();
    orientation.resize( numAgents );


    auto& atomsAgentTypes = atomsCache.agentTypes();
    for( size_t i = 0; i < numAgents; ++i )
    {
        CompoundDataPtr agentCompoundData = new CompoundData;
        auto &agentCompound = agentCompoundData->writable();
        int agentId = agentIds[i];
        // \todo: In order to use the agentId for cryptomattes we need a string attribute,
        // but we don't currently have a way of changing attribute data type in Gaffer.
        // This is just a hack until that functionality exists. Remove when possible.
        agentCacheIdsStr[i] = std::to_string( agentId );

        // load in memory the agent type since the cache need this to interpolate the pose
        const std::string &agentTypeName = atomsCache.agentType( frame, agentId );
        agentTypes[i] = agentTypeName;

        AtomsCore::Pose pose;
        atomsCache.loadAgentPose( frame, agentId, pose );
        AtomsCore::MapMetadata metadata;
        atomsCache.loadAgentMetadata( frame, agentId, metadata );

        auto variationMetadata = metadata.getTypedEntry<AtomsCore::StringMetadata>( ATOMS_AGENT_VARIATION );
        agentVariations[i] = variationMetadata ? variationMetadata->get() : "";

        auto lodMetadata = metadata.getTypedEntry<AtomsCore::StringMetadata>( ATOMS_AGENT_LOD );
        agentLods[i] = lodMetadata ? lodMetadata->get(): "";

        auto directionMetadata = metadata.getTypedEntry<AtomsCore::Vector3Metadata>( ATOMS_AGENT_DIRECTION );
        if ( directionMetadata )
        {
            auto& v = directionMetadata->get();
            auto& vOut = direction[i];
            vOut.x = v.x;
            vOut.y = v.y;
            vOut.z = v.z;
        }


        auto velocityMetadata = metadata.getTypedEntry<AtomsCore::Vector3Metadata>( ATOMS_AGENT_VELOCITY );
        if ( velocityMetadata )
        {
            auto& v = velocityMetadata->get();
            auto& vOut = velocity[i];
            vOut.x = v.x;
            vOut.y = v.y;
            vOut.z = v.z;
        }

        auto scaleMetadata = metadata.getTypedEntry<AtomsCore::Vector3Metadata>( ATOMS_AGENT_SCALE );
        if ( scaleMetadata )
        {
            auto& v = scaleMetadata->get();
            auto& vOut = scale[i];
            vOut.x = v.x;
            vOut.y = v.y;
            vOut.z = v.z;
        }


        if ( pose.numJoints() > 0 )
        {
            auto agentTypePtr = atomsAgentTypes.agentType( agentTypeName );
            if ( agentTypePtr )
            {
                AtomsCore::Poser poser( &agentTypePtr->skeleton() );
                auto matrices = poser.getAllWorldMatrix( pose );
                if ( !matrices.empty() )
                {
                    auto& pelvisMtx = matrices[0];
                    positions[i] = pelvisMtx.translation();
                    rootMatrix[i] = Imath::M44f( pelvisMtx );
                    orientation[i] = Imath::extractQuat( pelvisMtx );
                }
            }
            else
            {
                positions[i] = pose.jointPose( 0 ).translation;
            }
        }
    }

    PointsPrimitivePtr points = new PointsPrimitive( positionData );
    points->variables["atoms:agentType"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentTypesData );
    points->variables["atoms:variation"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentVariationData );
    points->variables["atoms:lod"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentLodData );
    points->variables["atoms:agentId"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentCacheIdsData );
    points->variables["atoms:agentIdStr"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentCacheIdsStrData );
    points->variables["atoms:velocity"] = PrimitiveVariable( PrimitiveVariable::Vertex, velocityData );
    points->variables["atoms:direction"] = PrimitiveVariable( PrimitiveVariable::Vertex, directionData );
    points->variables["atoms:scale"] = PrimitiveVariable( PrimitiveVariable::Vertex, scaleData );
    points->variables["atoms:orientation"] = PrimitiveVariable( PrimitiveVariable::Vertex, orientationData );
    return points;

}

void AtomsCrowdReader::hashAttributes( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const
{
    atomsSimFilePlug()->hash( h );
    refreshCountPlug()->hash( h );
    timeOffsetPlug()->hash( h );
    agentIdsPlug()->hash( h );
    h.append( context->getFrame() );
}

IECore::ConstCompoundObjectPtr AtomsCrowdReader::computeAttributes( const SceneNode::ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const
{
    // The attribute output a single attribute atoms:agents containing the pose and metadata of every agents
    ConstEngineDataPtr engineData = boost::static_pointer_cast<const EngineData>( enginePlug()->getValue() );

    IECore::CompoundObjectPtr result = new IECore::CompoundObject;
    auto& members = result->members();

    if ( !engineData )
    {
        return result;
    }

    auto& atomsCache = engineData->cache();
    double frame = engineData->frame();

    auto& agentIds = engineData->agentIds();
    size_t numAgents = agentIds.size();

    auto& translator = AtomsMetadataTranslator::instance();

    Box3dDataPtr cacheBox = new Box3dData;
    atomsCache.loadBoundingBox( frame, cacheBox->writable() );
    //members["atoms:boundingBox"] = cacheBox;

    IntVectorDataPtr agentIndices = new IntVectorData;
    agentIndices->writable() = agentIds;
    //members["atoms:agentIds"] = agentIndices;

    CompoundDataPtr agentsCompound = new CompoundData;
    auto &agentsCompoundData = agentsCompound->writable();

    auto& atomsAgentTypes = atomsCache.agentTypes();
    for( size_t i = 0; i < numAgents; ++i )
    {
        CompoundDataPtr agentCompoundData = new CompoundData;
        auto &agentCompound = agentCompoundData->writable();
        int agentId = agentIds[i];

        // load in memory the agent type since the cache need this to interpolate the pose
        const std::string &agentTypeName = atomsCache.agentType( frame, agentId );
        auto agentTypePtr = atomsAgentTypes.agentType( agentTypeName );

        AtomsPtr<AtomsCore::PoseMetadata> posePtr( new AtomsCore::PoseMetadata );
        atomsCache.loadAgentPose( frame, agentId, posePtr->get() );
        AtomsPtr<AtomsCore::MapMetadata> metadataPtr( new AtomsCore::MapMetadata );
        atomsCache.loadAgentMetadata( frame, agentId, *metadataPtr.get() );

        Box3dDataPtr agentBBox = new Box3dData;
        auto& agentBBoxData = agentBBox->writable();
        if ( agentTypePtr )
        {
            AtomsCore::Poser poser( &agentTypePtr->skeleton() );
            M44dVectorDataPtr matricesData = new M44dVectorData;
            M44dVectorDataPtr normalMatricesData = new M44dVectorData;

            // get the root world matrix
            auto rootMatrix = poser.getWorldMatrix( posePtr->get(), 0 );
            // now for all the detached joint multiply transform in root local space
            AtomsCore::Matrix rootInverseMatrix = rootMatrix.inverse();
            const std::vector<unsigned short>& detachedJoints = agentTypePtr->skeleton().detachedJoints();
            for ( unsigned int ii = 0; ii < agentTypePtr->skeleton().detachedJoints().size(); ii++ )
            {
                poser.setWorldMatrix( posePtr->get(), poser.getWorldMatrix( posePtr->get(), detachedJoints[ii] ) * rootInverseMatrix, detachedJoints[ii] );
            }

            auto& outMatrices = matricesData->writable();
            auto& outNormalMatrices = normalMatricesData->writable();
            outMatrices = poser.getAllWorldMatrix( posePtr->get() );
            outNormalMatrices.resize( outMatrices.size() );

            //update the position metadata
            auto positionMeta = metadataPtr->getTypedEntry<AtomsCore::Vector3Metadata>( ATOMS_AGENT_POSITION );
            if ( positionMeta )
            {
                positionMeta->set( rootMatrix.translation() );
            }

            const AtomsCore::MapMetadata& metadata = agentTypePtr->metadata();
            AtomsPtr<const AtomsCore::MatrixArrayMetadata> bindPosesInvPtr = metadata.getTypedEntry<const AtomsCore::MatrixArrayMetadata>( "worldBindPoseInverseMatrices" );
            if ( !bindPosesInvPtr )
            {
                throw InvalidArgumentException( "AtomsCrowdReader : No worldBindPoseInverseMatrices metadata found on agent type: " +  agentTypeName );
            }

            const std::vector<AtomsCore::Matrix>& bindPosesInv = bindPosesInvPtr->get();

            // Store the matrices for the skinning
            for ( unsigned int j = 0; j < outMatrices.size(); j++ )
            {
                AtomsCore::Matrix &jMtx = outMatrices[j];
                agentBBoxData.extendBy(jMtx.translation());
                jMtx = bindPosesInv[j] * jMtx;
                outNormalMatrices[j] = jMtx.inverse().transpose();
            }

            agentCompound["poseWorldMatrices"] = matricesData;
            agentCompound["poseNormalWorldMatrices"] = normalMatricesData;

            M44dDataPtr rootMatrixData = new M44dData( rootMatrix );
            agentCompound["rootMatrix"] = rootMatrixData;

            // This is an hash of the agent pose in local space
            UInt64DataPtr hashData = new UInt64Data( posePtr->get().hash() );
            agentCompound["hash"] = hashData;
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdReader: Invalid agent type " + agentTypeName );
        }

        agentCompound["metadata"] = translator.translate( metadataPtr );

        agentCompound["boundingBox"] = agentBBox;

        StringDataPtr aTypeData = new StringData();
        aTypeData->writable() = agentTypeName;
        agentCompound["agentType"] = aTypeData;

        //members[ "atoms:agent:" + std::to_string( agentId ) ] = agentCompoundData;
        agentsCompoundData[ std::to_string( agentId ) ] =  agentCompoundData;
    }

    // Store the frame offset, this is used by the cloth reader to mantain the 2 caches in synch
    FloatDataPtr frameOffsetData = new FloatData;
    frameOffsetData->writable() = timeOffsetPlug()->getValue();
    agentsCompoundData[ "frameOffset" ] = frameOffsetData;

    // Store the cache data inside a blind data container to not inpact on the UI performance
    BlindDataHolderPtr atomsObj = new BlindDataHolder(agentsCompound);
    members[ "atoms:agents" ] = atomsObj;
    return result;
}

void AtomsCrowdReader::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
    if( output == enginePlug() )
    {
        atomsSimFilePlug()->hash( h );
        refreshCountPlug()->hash( h );
        timeOffsetPlug()->hash( h );
        agentIdsPlug()->hash( h );
        h.append(context->getFrame());
    }

    if ( output == sourcePlug() )
    {
        enginePlug()->hash( h );
        h.append( context->getFrame() );
    }
    ObjectSource::hash( output, context, h );
}

void AtomsCrowdReader::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
    if ( output == enginePlug() )
    {
        // check if the frame or file path is changed and update it
        static_cast<ObjectPlug *>( output )->setValue(
                new EngineData( atomsSimFilePlug()->getValue(), context->getFrame() + timeOffsetPlug()->getValue(), agentIdsPlug()->getValue() )
                );
        return;
    }

    ObjectSource::compute( output, context );
}
