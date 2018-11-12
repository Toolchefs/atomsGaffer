#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECoreScene/PointsPrimitive.h"

#include "AtomsUtils/PathSolver.h"
#include "Atoms/AtomsCache.h"
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
#include "Atoms/GlobalNames.h"

IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsCrowdReader );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

size_t AtomsCrowdReader::g_firstPlugIndex = 0;

AtomsCrowdReader::AtomsCrowdReader( const std::string &name )
	:	ObjectSource( name, "crowd" )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsSimFile" ) );
	addChild( new IntPlug( "refreshCount" ) );
}

StringPlug* AtomsCrowdReader::atomsSimFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsCrowdReader::atomsSimFilePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

IntPlug* AtomsCrowdReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const IntPlug* AtomsCrowdReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

void AtomsCrowdReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	ObjectSource::affects( input, outputs );

	if( input == atomsSimFilePlug() || input == refreshCountPlug() )
	{
		outputs.push_back( sourcePlug() );
        outputs.push_back( sourcePlug() );
        outputs.push_back(outPlug()->attributesPlug());
	}
}

void AtomsCrowdReader::hashSource( const Gaffer::Context *context, MurmurHash &h ) const
{
	atomsSimFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
	outPlug()->attributesPlug()->hash( h );
	h.append( context->getFrame() );
}

ConstObjectPtr AtomsCrowdReader::computeSource( const Gaffer::Context *context ) const
{
    auto cacheDataPtr = outPlug()->attributesPlug()->getValue();
    if (!cacheDataPtr)
    {
        PointsPrimitivePtr points = new PointsPrimitive( );
        return points;
    }


    auto crowd = runTimeCast<const CompoundObject>( cacheDataPtr );
    if( !crowd )
    {
        throw InvalidArgumentException( "AtomsCrowdReader : Empty attributes." );
    }

    auto agentIdsObj = crowd->members().find( "agentIds" );
    if ( agentIdsObj == crowd->members().cend())
    {
        throw InvalidArgumentException( "AtomsCrowdReader : No agents found." );
    }
    auto agentIdsPtr = runTimeCast<const IntVectorData>(agentIdsObj->second.get());
    if (!agentIdsPtr)
    {
        throw InvalidArgumentException( "AtomsCrowdReader : No agents found." );
    }

    auto agentIds = agentIdsPtr->readable();

    V3fVectorDataPtr positionData = new V3fVectorData;
    positionData->setInterpretation( IECore::GeometricData::Interpretation::Point );
    auto &positions = positionData->writable();
    positions.resize( agentIds.size() );

    IntVectorDataPtr agentCacheIdsData = new IntVectorData;
    agentCacheIdsData->writable() = agentIdsPtr->readable();

    StringVectorDataPtr agentTypesData = new StringVectorData;
    auto &agentTypes = agentTypesData->writable();
    agentTypes.resize( agentIds.size() );

    StringVectorDataPtr agentVariationData = new StringVectorData;
    auto &agentVariations = agentVariationData->writable();
    agentVariations.resize( agentIds.size() );

    StringVectorDataPtr agentLodData = new StringVectorData;
    auto &agentLods = agentLodData->writable();
    agentLods.resize( agentIds.size() );


    for( size_t i = 0; i < agentIds.size(); ++i )
    {
        auto agentObj = crowd->members().find( std::to_string( agentIds[i] ) );
        if(agentObj == crowd->members().cend())
            continue;

        auto agentDataPtr = runTimeCast<const CompoundData>(agentObj->second.get());
        if (!agentDataPtr)
        {
            continue;
        }

        const CompoundData* posePtr = agentDataPtr->member<const CompoundData>("metadata");
        if (!posePtr)
        {
            continue;
        }

        auto posDataPtr = posePtr->member<const V3dDataBase>("position");
        if (posDataPtr)
            positions[i] = posDataPtr->readable();

        auto variationDataPtr = posePtr->member<const StringData>(ATOMS_AGENT_VARIATION);
        if (variationDataPtr)
            agentVariations[i] = variationDataPtr->readable();

        auto lodDataPtr = posePtr->member<const StringData>(ATOMS_AGENT_LOD);
        if (lodDataPtr)
            agentLods[i] = lodDataPtr->readable();

        auto aTypePtr = agentDataPtr->member<const StringData>("agentType");
        if (!aTypePtr)
        {
            agentTypes[i] = aTypePtr->readable();
        }
    }

    PointsPrimitivePtr points = new PointsPrimitive( positionData );
    points->variables["agentType"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentTypesData);
    points->variables["variation"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentVariationData);
    points->variables["lod"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentLodData);
    points->variables["agentId"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentCacheIdsData);
    return points;
}

void AtomsCrowdReader::getAtomsCacheName(const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension) const
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
        if (!(std::string::npos == lastDotIdx || cacheName.substr( lastDotIdx + 1, cacheName.length() ) != extension))
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

void AtomsCrowdReader::hashAttributes( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const
{
    atomsSimFilePlug()->hash( h );
    h.append( context->getFrame() );
}

IECore::ConstCompoundObjectPtr AtomsCrowdReader::computeAttributes( const SceneNode::ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const
{
    std::string atomsSimFile = atomsSimFilePlug()->getValue();

    // split sim file in path/name
    std::string cachePath, cacheName;
    getAtomsCacheName( atomsSimFile, cachePath, cacheName, "atoms" );

    double frame = context->getFrame();
    IECore::CompoundObjectPtr result = new IECore::CompoundObject;

    Atoms::AtomsCache atomsCache;
    if( !atomsCache.openCache( cachePath, cacheName ) )
    {
        return result;
    }

    frame = frame < atomsCache.startFrame() ? atomsCache.startFrame() : frame;
    frame = frame > atomsCache.endFrame() ? atomsCache.endFrame() : frame;

    int cacheFrame = static_cast<int>( frame );
    double frameReminder = frame - cacheFrame;

    atomsCache.loadFrameHeader( cacheFrame );

    atomsCache.loadFrame( cacheFrame );
    if (frameReminder > 0.0)
        atomsCache.loadNextFrame( cacheFrame + 1 );

    auto agentIds = atomsCache.agentIds( cacheFrame );
    size_t numAgents = agentIds.size();

    auto& translator = AtomsMetadataTranslator::instance();

    Box3dDataPtr cacheBox = new Box3dData;
    atomsCache.loadBoundingBox( frame, cacheBox->writable() );
    result->members()["boundingBox"] = cacheBox;

    IntVectorDataPtr agentIndices = new IntVectorData;
    agentIndices->writable() = agentIds;
    result->members()["agentIds"] = agentIndices;

    for( size_t i = 0; i < numAgents; ++i )
    {
        CompoundDataPtr agentCompound = new CompoundData;
        size_t agentId = agentIds[i];

        // load in memory the agent type since the cache need this to interpolate the pose
        const std::string &agentTypeName = atomsCache.agentType( frame, agentId );
        auto agentTypePtr = atomsCache.loadAgentType( agentTypeName, false );

        AtomsPtr<AtomsCore::PoseMetadata> posePtr( new AtomsCore::PoseMetadata );
        atomsCache.loadAgentPose( frame, agentId, posePtr->get() );
        AtomsPtr<AtomsCore::MapMetadata> metadataPtr( new AtomsCore::MapMetadata );
        atomsCache.loadAgentMetadata( frame, agentId, *metadataPtr.get() );


        if (agentTypePtr)
        {
            AtomsCore::Poser poser(&agentTypePtr->skeleton());
            M44dVectorDataPtr matricesData = new M44dVectorData;
            matricesData->writable()= poser.getAllWorldMatrix(posePtr->get());

            //update the position metadata
            auto positionMeta = metadataPtr->getTypedEntry<AtomsCore::Vector3Metadata>(ATOMS_AGENT_POSITION);
            if (positionMeta && matricesData->readable().size() > 0)
            {
                positionMeta->set(matricesData->readable()[0].translation());
            }
            agentCompound->writable()["poseWorldMatrices"] = matricesData;
        }
        agentCompound->writable()["pose"] = translator.translate( posePtr );


        agentCompound->writable()["metadata"] = translator.translate(metadataPtr);


        Box3dDataPtr agentBBox = new Box3dData;
        atomsCache.loadAgentBoundingBox( frame, agentId, agentBBox->writable() );
        agentCompound->writable()["boundingBox"] = agentBBox;

        StringDataPtr aTypeData = new StringData();
        agentCompound->writable()["agentType"] = aTypeData;

        result->members()[ std::to_string( agentId ) ] = agentCompound;
    }

    // store also the agent types inside the compound
    auto& agentTypes = atomsCache.agentTypes();
    auto agentTypeNames = agentTypes.agentTypeNames();
    CompoundDataPtr agentTypesCompound = new CompoundData;
    for ( size_t i = 0; i < agentTypeNames.size(); ++i )
    {
        const std::string& name = agentTypeNames[i];

        auto agentType = agentTypes.agentType( name );
        if (!agentType)
            continue;

        CompoundDataPtr agentTypeCompound = new CompoundData;

        //store the bind world matrices
        auto& skeleton = agentType->skeleton();
        AtomsCore::Poser poser(&skeleton);
        M44dVectorDataPtr worldBindMatrices = new M44dVectorData;
        worldBindMatrices->writable() = poser.getAllWorldBindMatrix();
        agentTypeCompound->writable()["worldBindMatrices"] = worldBindMatrices;

        M44dVectorDataPtr bindMatrices = new M44dVectorData;
        auto& bindMtxVec = worldBindMatrices->writable();
        bindMtxVec.resize(skeleton.numJoints());
        for( size_t jId=0; jId < skeleton.numJoints(); ++jId )
        {
            bindMtxVec[jId] = skeleton.joint( jId ).matrix();
        }
        agentTypeCompound->writable()["bindMatrices"] = bindMatrices;


        CompoundDataPtr agentTypeMetadataCompound = new CompoundData;
        auto& agentTypeMetadata = agentType->metadata();
        for( auto it = agentTypeMetadata.cbegin(); it != agentTypeMetadata.cend(); ++it )
        {
            if (!it->second)
                continue;

            IECore::DataPtr object = translator.translate( it->second );
            if (object)
                agentTypeMetadataCompound->writable()[it->first] = object;
        }
        agentTypeCompound->writable()["metadata"] = agentTypeMetadataCompound;

        agentTypesCompound->writable()[name] = agentTypeCompound;
    }

    result->members()["agentTypes"] = agentTypesCompound;

    return result;
}