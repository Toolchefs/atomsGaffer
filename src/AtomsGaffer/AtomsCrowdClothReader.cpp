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
    addChild( new FloatPlug( "timeOffset" ) );
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

Gaffer::FloatPlug *AtomsCrowdClothReader::timeOffsetPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex  + 1 );
}

const Gaffer::FloatPlug *AtomsCrowdClothReader::timeOffsetPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex  + 1 );
}

IntPlug* AtomsCrowdClothReader::refreshCountPlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 2 );
}

const IntPlug* AtomsCrowdClothReader::refreshCountPlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 2 );
}

void AtomsCrowdClothReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
    if( input == inPlug()->objectPlug() || input == inPlug()->attributesPlug() ||
        input == atomsClothFilePlug() || input == refreshCountPlug() || input == timeOffsetPlug() )
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
    timeOffsetPlug()->hash( h );
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


    Atoms::AtomsClothCache m_cache;
    std::string cachePath, cacheName;
    std::string filePath = atomsClothFilePlug()->getValue();
    double m_frame = context->getFrame() + timeOffsetPlug()->getValue() + frameOffset;

    getAtomsCacheName( filePath, cachePath, cacheName, "clothcache" );

    if( !m_cache.openCache( cachePath, cacheName ) )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdReader", "Unable to load the atoms cache " + cachePath + "/" + cacheName + ".atoms" );
        return NullObject::defaultNullObject();
    }

    m_frame = m_frame < m_cache.startFrame() ? m_cache.startFrame() : m_frame;
    m_frame = m_frame > m_cache.endFrame() ? m_cache.endFrame() : m_frame;

    int cacheFrame = static_cast<int>( m_frame );
    double frameReminder = m_frame - cacheFrame;

    if ( agentIdVec )
    {
        m_cache.setAgentsToLoad( *agentIdVec );
    }

    m_cache.loadFrame( cacheFrame );
    if ( frameReminder > 0.0 )
        m_cache.loadNextFrame( cacheFrame + 1 );

    auto m_agentIds = m_cache.agentIds( cacheFrame );

    //extract the ids, variation, lod, position, direction and velocity and bbox and store them on points

    CompoundDataPtr clothCompound = new CompoundData;
    auto& clothData = clothCompound->writable();
    for (auto agentId: m_agentIds )
    {
        CompoundDataPtr agentCompound = new CompoundData;
        auto& agentData = agentCompound->writable();
        /*
        std::vector<std::string> meshNames = m_cache.agentClothMeshNames( m_frame, agentId );
        for( const auto& meshName: meshNames )
        {
            CompoundDataPtr meshCompound = new CompoundData;
            auto& meshData = meshCompound->writable();


            V3dVectorDataPtr p = new V3dVectorData;
            V3dVectorDataPtr n = new V3dVectorData;
            Box3dDataPtr bbox = new Box3dData;

            m_cache.loadAgentClothMesh( m_frame, agentId, meshName, p->writable(), n->writable() );
            m_cache.loadAgentClothMeshBoundingBox( m_frame, agentId, meshName, bbox->writable() );

            meshData[ "P" ] = p;
            meshData[ "N" ] = n;
            meshData[ "boundingBox" ] = bbox;

            agentData[ meshName ] = meshCompound;

        }
        */
        clothData[ std::to_string(agentId) ] = agentCompound;
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
