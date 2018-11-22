#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECore/NullObject.h"

#include "AtomsUtils/PathSolver.h"
#include "AtomsUtils/Utils.h"
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

class AtomsCrowdReader::EngineData : public Data
{

public :

    EngineData(const std::string& filePath, float frame, const std::string& agentIdsStr):
            m_filePath(filePath),
            m_frame(frame)
    {
        m_frame = frame;
        std::string cachePath, cacheName;
        getAtomsCacheName( filePath, cachePath, cacheName, "atoms" );

        if( !m_cache.openCache( cachePath, cacheName ) )
        {
            return;
        }

        m_frame = m_frame < m_cache.startFrame() ? m_cache.startFrame() : m_frame;
        m_frame = m_frame > m_cache.endFrame() ? m_cache.endFrame() : m_frame;

        int cacheFrame = static_cast<int>( m_frame );
        double frameReminder = m_frame - cacheFrame;

        m_cache.loadFrameHeader( cacheFrame );

        // filter agents
        std::vector<int> agentsIds;
        parseVisibleAgents(agentsIds, agentIdsStr);
        if (agentsIds.size() > 0) {
            m_cache.setAgentsToLoad(agentsIds);
            m_agentIds = agentsIds;
        }
        else
        {
            m_agentIds = m_cache.agentIds(cacheFrame);
        }

        m_cache.loadFrame( cacheFrame );
        if (frameReminder > 0.0)
            m_cache.loadNextFrame( cacheFrame + 1 );

        for( size_t i = 0; i < m_agentIds.size(); ++i )
        {
            size_t agentId = m_agentIds[i];
            // load in memory the agent type since the cache need this to interpolate the pose
            const std::string &agentTypeName = m_cache.agentType(m_frame, agentId);
            m_cache.loadAgentType(agentTypeName, false);
        }
    }

    void hash( MurmurHash &h ) const override
    {
        h.append(m_filePath);
        h.append(m_frame);
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

    void getAtomsCacheName(const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension) const
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

    void parseVisibleAgents(std::vector<int>& agentsFiltered, const std::string& agentIdsStr)
    {
        std::vector<std::string> idsEntryStr;
        AtomsUtils::splitString(agentIdsStr, ',', idsEntryStr);
        std::vector<int> idsToRemove;
        std::vector<int> idsToAdd;

        std::vector<int> agentIds = m_cache.agentIds(m_cache.currentFrame());
        std::sort(agentIds.begin(), agentIds.end());
        for (unsigned int eId = 0; eId < idsEntryStr.size(); eId++)
        {
            std::string currentStr = idsEntryStr[eId];
            currentStr = AtomsUtils::eraseFromString(currentStr, ' ');
            if (currentStr.length() == 0)
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
            currentStr = AtomsUtils::eraseFromString(currentStr, '(');
            currentStr = AtomsUtils::eraseFromString(currentStr, ')');

            // is a range
            if (currentStr.find('-') != std::string::npos)
            {
                std::vector<std::string> rangeEntryStr;
                AtomsUtils::splitString(currentStr, '-', rangeEntryStr);

                int startRange = 0;
                int endRange = 0;
                if (rangeEntryStr.size() > 0)
                {
                    startRange = atoi(rangeEntryStr[0].c_str());

                    if (rangeEntryStr.size() > 1)
                    {
                        endRange = atoi(rangeEntryStr[1].c_str());
                        if (startRange < endRange)
                        {
                            for (unsigned int rId = startRange; rId <= endRange; rId++)
                            {
                                currentIdArray->push_back(rId);
                            }
                        }
                    }
                }
            }
            else
            {
                int tmpId = atoi(currentStr.c_str());
                currentIdArray->push_back(tmpId);
            }
        }

        agentsFiltered.clear();
        std::sort(idsToAdd.begin(), idsToAdd.end());
        std::sort(idsToRemove.begin(), idsToRemove.end());
        if (idsToAdd.size() > 0)
        {
            std::vector<int> validIds;
            std::set_intersection(
                    agentIds.begin(), agentIds.end(),
                    idsToAdd.begin(), idsToAdd.end(),
                    std::back_inserter(validIds)
            );


            std::set_difference(
                    validIds.begin(), validIds.end(),
                    idsToRemove.begin(), idsToRemove.end(),
                    std::back_inserter(agentsFiltered)
            );
        }
        else
        {
            std::set_difference(
                    agentIds.begin(), agentIds.end(),
                    idsToRemove.begin(), idsToRemove.end(),
                    std::back_inserter(agentsFiltered)
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

private :

    Atoms::AtomsCache m_cache;

    std::string m_filePath;

    std::vector<int> m_agentIds;

    float m_frame;



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

	if( input == atomsSimFilePlug() || input == refreshCountPlug() || input == agentIdsPlug() || input == timeOffsetPlug())
    {
	    outputs.push_back( enginePlug() );
    }

	if (input == enginePlug())
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
    ConstEngineDataPtr engineData = boost::static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        PointsPrimitivePtr points = new PointsPrimitive( );
        return points;
    }

    double frame = engineData->frame();
    auto& atomsCache = engineData->cache();
    //extract the ids, variation, lod, position, direction and velocity and bbox and store them on points

    auto& agentIds = engineData->agentIds();
    size_t numAgents = agentIds.size();


    V3fVectorDataPtr positionData = new V3fVectorData;
    positionData->setInterpretation( IECore::GeometricData::Interpretation::Point );
    auto &positions = positionData->writable();
    positions.resize( numAgents );

    IntVectorDataPtr agentCacheIdsData = new IntVectorData;
    agentCacheIdsData->writable() = agentIds;

    StringVectorDataPtr agentTypesData = new StringVectorData;
    auto &agentTypes = agentTypesData->writable();
    agentTypes.resize( numAgents );

    StringVectorDataPtr agentVariationData = new StringVectorData;
    auto &agentVariations = agentVariationData->writable();
    agentVariations.resize( numAgents );

    StringVectorDataPtr agentLodData = new StringVectorData;
    auto &agentLods = agentLodData->writable();
    agentLods.resize( numAgents);

    V3fVectorDataPtr velocityData = new V3fVectorData;
    auto &velocity = velocityData->writable();
    velocity.resize( numAgents);

    V3fVectorDataPtr directionData = new V3fVectorData;
    auto &direction = directionData->writable();
    direction.resize( numAgents);

    Box3fVectorDataPtr boundingBoxData = new Box3fVectorData;
    auto &boundingBox = boundingBoxData->writable();
    boundingBox.resize( numAgents);

    M44fVectorDataPtr rootMatrixData = new M44fVectorData;
    auto &rootMatrix = rootMatrixData->writable();
    rootMatrix.resize( numAgents);


    auto& atomsAgentTypes = atomsCache.agentTypes();
    for( size_t i = 0; i < numAgents; ++i )
    {
        CompoundDataPtr agentCompoundData = new CompoundData;
        auto &agentCompound = agentCompoundData->writable();
        size_t agentId = agentIds[i];

        // load in memory the agent type since the cache need this to interpolate the pose
        const std::string &agentTypeName = atomsCache.agentType( frame, agentId );
        agentTypes[i] = agentTypeName;

        AtomsCore::Pose pose;
        atomsCache.loadAgentPose( frame, agentId, pose );
        AtomsCore::MapMetadata metadata;
        atomsCache.loadAgentMetadata( frame, agentId, metadata );

        auto variationMetadata = metadata.getTypedEntry<AtomsCore::StringMetadata>(ATOMS_AGENT_VARIATION);
        agentVariations[i] = variationMetadata ? variationMetadata->get() : "";

        auto lodMetadata = metadata.getTypedEntry<AtomsCore::StringMetadata>(ATOMS_AGENT_LOD);
        agentLods[i] = lodMetadata ? lodMetadata->get(): "";

        auto directionMetadata = metadata.getTypedEntry<AtomsCore::Vector3Metadata>(ATOMS_AGENT_DIRECTION);
        if (directionMetadata)
        {
            auto& v = directionMetadata->get();
            auto& vOut = direction[i];
            vOut.x = v.x;
            vOut.y = v.y;
            vOut.z = v.z;
        }


        auto velocityMetadata = metadata.getTypedEntry<AtomsCore::Vector3Metadata>(ATOMS_AGENT_VELOCITY);
        if (velocityMetadata)
        {
            auto& v = velocityMetadata->get();
            auto& vOut = velocity[i];
            vOut.x = v.x;
            vOut.y = v.y;
            vOut.z = v.z;
        }


        AtomsCore::Box3 bbox;
        atomsCache.loadAgentBoundingBox( frame, agentId, bbox );
        auto& bbOut = boundingBox[i];
        bbOut.min.x = bbox.min.x;
        bbOut.min.y = bbox.min.y;
        bbOut.min.z = bbox.min.z;

        bbOut.max.x = bbox.max.x;
        bbOut.max.y = bbox.max.y;
        bbOut.max.z = bbox.max.z;

        if ( pose.numJoints() > 0 ) {
            auto agentTypePtr = atomsAgentTypes.agentType( agentTypeName );
            if (agentTypePtr)
            {
                AtomsCore::Poser poser(&agentTypePtr->skeleton());
                auto matrices = poser.getAllWorldMatrix(pose);
                if (matrices.size() > 0) {
                    positions[i] = matrices[0].translation();
                    rootMatrix[i] = Imath::M44f(matrices[0]);
                }
            } else {
                positions[i] = pose.jointPose(0).translation;
            }
        }
    }

    PointsPrimitivePtr points = new PointsPrimitive( positionData );
    points->variables["agentType"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentTypesData);
    points->variables["variation"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentVariationData);
    points->variables["lod"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentLodData);
    points->variables["agentId"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentCacheIdsData);
    points->variables["velocity"] = PrimitiveVariable( PrimitiveVariable::Vertex, velocityData);
    points->variables["direction"] = PrimitiveVariable( PrimitiveVariable::Vertex, directionData);
    // For now don't save this since the ui inspector rises an exception
    //points->variables["boundingBox"] = PrimitiveVariable( PrimitiveVariable::Vertex, boundingBoxData);
    //points->variables["rootMatrix"] = PrimitiveVariable( PrimitiveVariable::Vertex, rootMatrixData);
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
    ConstEngineDataPtr engineData = boost::static_pointer_cast<const EngineData>( enginePlug()->getValue() );

    IECore::CompoundObjectPtr result = new IECore::CompoundObject;
    auto& members = result->members();

    if (!engineData)
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
    members["boundingBox"] = cacheBox;

    IntVectorDataPtr agentIndices = new IntVectorData;
    agentIndices->writable() = agentIds;
    members["agentIds"] = agentIndices;
    auto& atomsAgentTypes = atomsCache.agentTypes();
    for( size_t i = 0; i < numAgents; ++i )
    {
        CompoundDataPtr agentCompoundData = new CompoundData;
        auto &agentCompound = agentCompoundData->writable();
        size_t agentId = agentIds[i];

        // load in memory the agent type since the cache need this to interpolate the pose
        const std::string &agentTypeName = atomsCache.agentType( frame, agentId );
        auto agentTypePtr = atomsAgentTypes.agentType( agentTypeName );

        AtomsPtr<AtomsCore::PoseMetadata> posePtr( new AtomsCore::PoseMetadata );
        atomsCache.loadAgentPose( frame, agentId, posePtr->get() );
        AtomsPtr<AtomsCore::MapMetadata> metadataPtr( new AtomsCore::MapMetadata );
        atomsCache.loadAgentMetadata( frame, agentId, *metadataPtr.get() );


        if (agentTypePtr)
        {
            AtomsCore::Poser poser(&agentTypePtr->skeleton());
            M44dVectorDataPtr matricesData = new M44dVectorData;
            M44dVectorDataPtr normalMatricesData = new M44dVectorData;

            //update the position metadata
            auto positionMeta = metadataPtr->getTypedEntry<AtomsCore::Vector3Metadata>(ATOMS_AGENT_POSITION);
            if (positionMeta && matricesData->readable().size() > 0)
            {
                positionMeta->set(matricesData->readable()[0].translation());
            }

            //get the root world matrix
            auto rootMatrix = poser.getWorldMatrix(posePtr->get(), 0);
            //now for all the detached joint multiply transform in root local space
            AtomsCore::Matrix rootInverseMatrix = rootMatrix.inverse();
            const std::vector<unsigned short>& detachedJoints = agentTypePtr->skeleton().detachedJoints();
            for (unsigned int ii = 0; ii < agentTypePtr->skeleton().detachedJoints().size(); ii++)
            {
                poser.setWorldMatrix(posePtr->get(), poser.getWorldMatrix(posePtr->get(), detachedJoints[ii]) * rootInverseMatrix, detachedJoints[ii]);
            }

            auto& outMatrices = matricesData->writable();
            auto& outNormalMatrices = normalMatricesData->writable();
            outMatrices = poser.getAllWorldMatrix(posePtr->get());
            outNormalMatrices.resize(outMatrices.size());

            const AtomsCore::MapMetadata& metadata = agentTypePtr->metadata();
            AtomsPtr<const AtomsCore::MatrixArrayMetadata> bindPosesInvPtr = metadata.getTypedEntry<const AtomsCore::MatrixArrayMetadata>("worldBindPoseInverseMatrices");
            const std::vector<AtomsCore::Matrix>& bindPosesInv = bindPosesInvPtr->get();

            for (unsigned int j = 0; j < outMatrices.size(); j++) {
                AtomsCore::Matrix &jMtx = outMatrices[j];
                jMtx = bindPosesInv[j] * jMtx;
                outNormalMatrices[j] = jMtx.inverse().transpose();
            }

            agentCompound["poseWorldMatrices"] = matricesData;
            agentCompound["poseNormalWorldMatrices"] = normalMatricesData;

            M44dDataPtr rootMatrixData = new M44dData(rootMatrix);
            agentCompound["rootMatrix"] = rootMatrixData;

            UInt64DataPtr hashData = new UInt64Data(posePtr->get().hash());
            agentCompound["hash"] = hashData;
        }
        else
        {
            throw InvalidArgumentException("AtomsCrowdReader: Invalid agent type " + agentTypeName);
        }
        //agentCompound["pose"] = translator.translate( posePtr );

        agentCompound["metadata"] = translator.translate(metadataPtr);


        Box3dDataPtr agentBBox = new Box3dData;
        atomsCache.loadAgentBoundingBox( frame, agentId, agentBBox->writable() );
        agentCompound["boundingBox"] = agentBBox;

        StringDataPtr aTypeData = new StringData();
        aTypeData->writable() = agentTypeName;
        agentCompound["agentType"] = aTypeData;

        members[ std::to_string( agentId ) ] = agentCompoundData;
    }

    /*
    // store also the agent types inside the compound
    auto& agentTypes = atomsCache.agentTypes();
    auto agentTypeNames = agentTypes.agentTypeNames();
    CompoundDataPtr agentTypesCompoundData = new CompoundData;
    auto &agentTypesCompound = agentTypesCompoundData->writable();
    for ( size_t i = 0; i < agentTypeNames.size(); ++i )
    {
        const std::string& name = agentTypeNames[i];

        auto agentType = agentTypes.agentType( name );
        if (!agentType)
            continue;

        CompoundDataPtr agentTypeCompoundData = new CompoundData;
        auto &agentTypeCompound = agentTypeCompoundData->writable();

        //store the bind world matrices
        auto& skeleton = agentType->skeleton();
        AtomsCore::Poser poser(&skeleton);
        M44dVectorDataPtr worldBindMatrices = new M44dVectorData;
        worldBindMatrices->writable() = poser.getAllWorldBindMatrix();
        agentTypeCompound["worldBindMatrices"] = worldBindMatrices;

        M44dVectorDataPtr bindMatrices = new M44dVectorData;
        auto& bindMtxVec = worldBindMatrices->writable();
        bindMtxVec.resize(skeleton.numJoints());
        for( size_t jId=0; jId < skeleton.numJoints(); ++jId )
        {
            bindMtxVec[jId] = skeleton.joint( jId ).matrix();
        }
        agentTypeCompound["bindMatrices"] = bindMatrices;


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
        //agentTypeCompound["metadata"] = agentTypeMetadataCompound;

        agentTypesCompound[name] = agentTypeCompoundData;
    }

    members["agentTypes"] = agentTypesCompoundData;
    */
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

    if ( output == sourcePlug())
    {
        enginePlug()->hash(h);
        h.append(context->getFrame());
    }
    ObjectSource::hash( output, context, h );
}

void AtomsCrowdReader::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
    if (output == enginePlug()) {

        static_cast<ObjectPlug *>( output )->setValue(new EngineData(atomsSimFilePlug()->getValue(), context->getFrame() + timeOffsetPlug()->getValue(), agentIdsPlug()->getValue()));
        return;
    }

    ObjectSource::compute(output, context);
}