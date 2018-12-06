#include "AtomsGaffer/AtomsVariationReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECoreScene/MeshPrimitive.h"
#include "IECore/NullObject.h"
#include "IECoreScene/PointsPrimitive.h"

#include "Atoms/Variations.h"
#include "Atoms/Loaders/MeshLoader.h"
#include "Atoms/Variation/VariationLoader.h"

#include "Atoms/GlobalNames.h"
#include "AtomsUtils/PathSolver.h"
#include "AtomsUtils/Utils.h"

#include "AtomsCore/Metadata/MeshMetadata.h"
#include "AtomsCore/Metadata/Box3Metadata.h"
#include "AtomsCore/Metadata/BoolMetadata.h"
#include "AtomsCore/Metadata/ArrayMetadata.h"
#include "AtomsCore/Metadata/IntArrayMetadata.h"
#include "AtomsCore/Metadata/DoubleArrayMetadata.h"
#include "AtomsCore/Metadata/Vector3ArrayMetadata.h"


IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsVariationReader );

using namespace std;
using namespace std::placeholders;
using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

IECoreScene::PrimitiveVariable convertNormals( AtomsUtils::Mesh& mesh )
{
    auto& inNormals = mesh.normals();
    auto& inIndices = mesh.indices();

    V3fVectorDataPtr normalsData = new V3fVectorData;
    normalsData->setInterpretation( GeometricData::Normal );
    auto &normals = normalsData->writable();
    normals.reserve( inIndices.size() );
    normals.insert( normals.begin(), inNormals.begin(), inNormals.end() );
    return PrimitiveVariable( PrimitiveVariable::FaceVarying, normalsData );
}

IECoreScene::PrimitiveVariable convertUvs( AtomsUtils::Mesh& mesh, AtomsUtils::Mesh::UVData& uvSet )
{
    auto& inIndices = uvSet.uvIndices;

    V2fVectorDataPtr uvData = new V2fVectorData;
    uvData->setInterpretation( GeometricData::UV );
    auto& outUV = uvData->writable();
    outUV.reserve( inIndices.size() );
    for ( auto id: inIndices )
        outUV.push_back( uvSet.uvs[id] );

    return PrimitiveVariable( PrimitiveVariable::FaceVarying, uvData );
}

MeshPrimitivePtr convertAtomsMesh( AtomsPtr<AtomsCore::MapMetadata>& geoMap, bool genPref, bool genNref )
{
    if ( !geoMap )
    {
        return MeshPrimitivePtr();
    }

    auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
    if ( !meshMeta )
    {
        meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "cloth" );
        if (!meshMeta) {
            return MeshPrimitivePtr();
        }
    }

    auto& mesh = meshMeta->get();

    auto& vertexCount = mesh.vertexCount();
    auto& vertexIndices = mesh.indices();
    auto& points = mesh.points();

    IntVectorDataPtr verticesPerFace = new IntVectorData;
    auto &outCount = verticesPerFace->writable();
    outCount.insert( outCount.begin(), vertexCount.begin(), vertexCount.end() );

    IntVectorDataPtr vertexIds = new IntVectorData;
    auto &outIndices = vertexIds->writable();
    outIndices.insert( outIndices.begin(), vertexIndices.begin(), vertexIndices.end() );

    V3fVectorDataPtr p = new V3fVectorData;
    auto &outP = p->writable();
    outP = points;

    MeshPrimitivePtr meshPtr = new MeshPrimitive( verticesPerFace, vertexIds, "linear", p );

    meshPtr->variables["N"] = convertNormals( mesh );
    if ( genNref )
        meshPtr->variables["Nref"] = meshPtr->variables["N"];
    if ( genPref )
        meshPtr->variables["Pref"] = meshPtr->variables["P"];

    // Convert uv sets data
    auto& uvSets = mesh.uvSets();
    if ( !uvSets.empty() )
    {
        for ( size_t i = 0; i < uvSets.size(); ++i )
        {
            auto &uvSet = uvSets[i];
            auto uvPrim = convertUvs( mesh, uvSet );
            meshPtr->variables[uvSet.name] = uvPrim;
            if (i == 0) {
                meshPtr->variables["uv"] = uvPrim;
            }
        }
    }
    else
    {
        V2fVectorDataPtr uvData = new V2fVectorData;
        uvData->setInterpretation( GeometricData::UV );
        uvData->writable() = mesh.uvs();

        meshPtr->variables["uv"] = PrimitiveVariable( PrimitiveVariable::FaceVarying, uvData );
    }

    // Convert blend shape data
    auto blendShapes = geoMap->getTypedEntry<const AtomsCore::ArrayMetadata>("blendShapes");
    if ( blendShapes && blendShapes->size() > 0 )
    {
        for ( unsigned int blendId = 0; blendId < blendShapes->size(); blendId++ )
        {
            auto blendMap = blendShapes->getTypedElement<AtomsCore::MapMetadata>( blendId );
            if (!blendMap)
                continue;

            auto blendPoints = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "P" );
            auto& blendP = blendPoints->get();

            auto blendNormals = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "N" );
            auto& blendN = blendNormals->get();

            V3fVectorDataPtr normalsData = new V3fVectorData;
            normalsData->setInterpretation( GeometricData::Normal );
            auto &normals = normalsData->writable();
            normals.reserve( blendN.size() );
            if ( blendN.size() == vertexIndices.size() )
            {
                normals.insert( normals.begin(), blendN.begin(), blendN.end() );
            }
            else
            {
                // convert normals from vertex to face varying
                normals.resize( vertexIndices.size() );
                for ( size_t vId = 0; vId < std::min(vertexIndices.size(), blendN.size()); ++vId )
                {
                    normals[vId] = blendN[vertexIndices[vId]];
                }
            }

            meshPtr->variables["blendShape_" + std::to_string( blendId ) + "_N"] =
                    PrimitiveVariable( PrimitiveVariable::FaceVarying, normalsData );

            V3fVectorDataPtr blendPointsData = new V3fVectorData;
            auto &blendPts = blendPointsData->writable();
            blendPts.reserve( blendP.size() );
            blendPts.insert( blendPts.begin(), blendP.begin(), blendP.end() );
            meshPtr->variables["blendShape_" + std::to_string( blendId ) + "_P"] =
                    PrimitiveVariable( PrimitiveVariable::Vertex, blendPointsData );
        }

        IntDataPtr blendShapeCount = new IntData( blendShapes->size() );
        meshPtr->variables["blendShapeCount"] =  PrimitiveVariable( PrimitiveVariable::Constant, blendShapeCount );
    }

    // Convert the atoms metadata
    auto atomsMap = geoMap->getTypedEntry<const AtomsCore::MapMetadata>( "atoms" );
    if ( atomsMap && atomsMap->size() > 0 )
    {
        auto &translator = AtomsMetadataTranslator::instance();
        for ( auto it = atomsMap->cbegin(); it != atomsMap->cend(); ++it )
        {
            meshPtr->variables[it->first] =  PrimitiveVariable( PrimitiveVariable::Constant, translator.translate(it->second) );
        }
    }

    return meshPtr;
}


// Custom Data derived class used to encapsulate the data and
// logic needed to generate instances. We are deliberately omitting
// a custom TypeId etc because this is just a private class.
class AtomsVariationReader::EngineData : public Data
{

public :

    EngineData( const std::string& filePath ):
        m_filePath( filePath )
    {
        m_hierarchy = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
        if ( filePath.empty() )
            return;

        std::string fullFilePath = AtomsUtils::solvePath( filePath );

        if ( !AtomsUtils::fileExists( fullFilePath.c_str() ) )
        {
            throw InvalidArgumentException( "AtomsVariationsReader: " + fullFilePath + " doesn't exist" );
        }

        m_variations = Atoms::loadVariationFromFile( fullFilePath );

        auto agentTypeNames = m_variations.getAgentTypeNames();
        if ( agentTypeNames.empty() )
        {
            throw InvalidArgumentException( "AtomsVariationsReader: " + filePath + " is empty" );
        }

        // Atoms can export mesh using full path as name
        // In Gaffer the full path is built so this build a tree containing the full hierarchy of all the meshes
        for ( size_t aTypeId = 0; aTypeId != agentTypeNames.size(); ++aTypeId )
        {
            const auto& agentTypeName = agentTypeNames[aTypeId];
            auto agentTypePtr = m_variations.getAgentTypeVariationPtr( agentTypeName);
            if ( !agentTypePtr )
                continue;

            loadAgentTypeMesh( agentTypePtr, agentTypeName );

            auto agentTypeIt = m_meshesFileCache.find( agentTypeName );
            if ( agentTypeIt == m_meshesFileCache.end() )
                continue;

            AtomsPtr<AtomsCore::MapMetadata> agentTypeHierarchy( new AtomsCore::MapMetadata );
            for ( const auto& variationName: agentTypePtr->getGroupNames() )
            {
                auto variationPtr = agentTypePtr->getGroupPtr( variationName );
                if ( !variationPtr )
                    continue;

                AtomsPtr<AtomsCore::MapMetadata> variationHierarchy( new AtomsCore::MapMetadata );

                for( int cId = 0; cId < variationPtr->numCombinations(); ++cId )
                {
                    auto& combination = variationPtr->getCombinationAtIndex( cId );

                    auto geoPtr = agentTypePtr->getGeometryPtr( combination.first );
                    if (!geoPtr)
                        continue;

                    if (agentTypeIt->second.find( geoPtr->getGeometryFile() + ":" +geoPtr->getGeometryFilter() ) == agentTypeIt->second.end() )
                        continue;

                    // Split the full path geo name to build the full hierarchy
                    std::vector<std::string> objectNames;
                    AtomsUtils::splitString( combination.first, '|', objectNames );

                    auto currentMap = variationHierarchy;
                    for( const auto& objName: objectNames )
                    {
                        auto objIt = currentMap->getTypedEntry<AtomsCore::MapMetadata>( objName );
                        if ( !objIt )
                        {
                            AtomsCore::MapMetadata emptyData;
                            currentMap->addEntry( objName, &emptyData );
                            objIt = currentMap->getTypedEntry<AtomsCore::MapMetadata>( objName );
                        }
                        currentMap = objIt;
                    }
                }

                auto variationHierarchyData = std::static_pointer_cast<AtomsCore::Metadata>( variationHierarchy );
                agentTypeHierarchy->addEntry( variationName, variationHierarchyData, false );
            }

            auto agentTypeHierarchyData = std::static_pointer_cast<AtomsCore::Metadata>( agentTypeHierarchy );
            m_hierarchy->addEntry( agentTypeNames[aTypeId],agentTypeHierarchyData, false );
        }
    }

    void loadAgentTypeMesh( Atoms::AgentTypeVariationCPtr agentTypePtr, const std::string& agentTypeName )
    {
        auto geoNames = agentTypePtr->getGeometryNames();
        for ( size_t geoId = 0; geoId != geoNames.size(); ++geoId )
        {
            auto geoPtr = agentTypePtr->getGeometryPtr( geoNames[geoId] );
            if ( !geoPtr )
                continue;

            if ( geoPtr->getGeometryFile().find(".groom") != std::string::npos )
                continue;

            auto atomsGeoMap = Atoms::loadMesh( geoPtr->getGeometryFile(), geoPtr->getGeometryFilter() );
            if ( !atomsGeoMap )
            {
                throw InvalidArgumentException( "AtomsVariationsReader: Invalid geo: " + geoPtr->getGeometryFile() +":" + geoPtr->getGeometryFilter() );
            }

            //compute the bouding box
            AtomsCore::Box3f bbox;
            for ( auto meshIt = atomsGeoMap->begin(); meshIt != atomsGeoMap->end(); ++meshIt )
            {
                if ( meshIt->second->typeId() !=  AtomsCore::MapMetadata::staticTypeId() )
                    continue;

                auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>( meshIt->second );
                AtomsPtr<const AtomsCore::MapMetadata> atomsMapPtr = geoMap->getTypedEntry<const AtomsCore::MapMetadata>( "atoms" );
                if ( atomsMapPtr )
                {
                    // Skip this mesh if it hase the cloth hide tag
                    auto hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>( ATOMS_CLOTH_HIDE_MESH );
                    if ( hideMeshPtr && hideMeshPtr->value() )
                        continue;

                    // Skip this mesh if it has the preview tag
                    hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>( ATOMS_PREVIEW_MESH );
                    if ( hideMeshPtr && hideMeshPtr->value() )
                        continue;
                }


                auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
                if ( !meshMeta )
                {
                    meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "cloth" );
                    if ( !meshMeta )
                    {
                        continue;
                    }
                }

                for( const auto& p: meshMeta->get().points() )
                    bbox.extendBy(p);

            }

            if ( bbox.isEmpty() )
                continue;

            AtomsCore::Box3Metadata boxMeta;
            boxMeta.get().extendBy( bbox.min );
            boxMeta.get().extendBy( bbox.max );
            atomsGeoMap->addEntry( "boundingBox", &boxMeta );
            m_meshesFileCache[agentTypeName][geoPtr->getGeometryFile() + ":" +geoPtr->getGeometryFilter()] = atomsGeoMap;
        }
    }

    void hash( MurmurHash &h ) const override
    {
        h.append(m_filePath);
    }

    const Atoms::Variations& variations() const
    {
        return m_variations;
    }

    const AtomsPtr<AtomsCore::MapMetadata>& hierarchy() const
    {
        return m_hierarchy;
    }

    const std::map<std::string, std::map<std::string, AtomsPtr<AtomsCore::MapMetadata>>>& meshCache() const
    {
        return m_meshesFileCache;
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

    Atoms::Variations m_variations;

    std::string m_filePath;

    std::map<std::string, std::map<std::string, AtomsPtr<AtomsCore::MapMetadata>>> m_meshesFileCache;

    AtomsPtr<AtomsCore::MapMetadata> m_hierarchy;

};

size_t AtomsVariationReader::g_firstPlugIndex = 0;

AtomsVariationReader::AtomsVariationReader( const std::string &name )
	:	SceneNode( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsAgentFile" ) );
    addChild( new BoolPlug( "Pref" ) );
    addChild( new BoolPlug( "Nref" ) );
	addChild( new IntPlug( "refreshCount" ) );
	addChild( new ObjectPlug( "__engine", Plug::Out, NullObject::defaultNullObject() ) );
}

StringPlug* AtomsVariationReader::atomsAgentFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsVariationReader::atomsAgentFilePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::BoolPlug *AtomsVariationReader::generatePrefPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex +1 );
}

const Gaffer::BoolPlug *AtomsVariationReader::generatePrefPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex +1 );
}

Gaffer::BoolPlug *AtomsVariationReader::generateNrefPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::BoolPlug *AtomsVariationReader::generateNrefPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

IntPlug* AtomsVariationReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

const IntPlug* AtomsVariationReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

Gaffer::ObjectPlug *AtomsVariationReader::enginePlug()
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::ObjectPlug *AtomsVariationReader::enginePlug() const
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 4 );
}

void AtomsVariationReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	if( input == atomsAgentFilePlug() || input == refreshCountPlug() ||
	input == generateNrefPlug() || input == generatePrefPlug() )
	{
		outputs.push_back( enginePlug() );
	}

	if( input == enginePlug() )
	{
		outputs.push_back( outPlug()->boundPlug() );
		outputs.push_back( outPlug()->transformPlug() );
		outputs.push_back( outPlug()->attributesPlug() );
		outputs.push_back( outPlug()->objectPlug() );
		outputs.push_back( outPlug()->childNamesPlug() );
		outputs.push_back( outPlug()->setNamesPlug() );
		outputs.push_back( outPlug()->setPlug() );
	}

    SceneNode::affects( input, outputs );
}

void AtomsVariationReader::hashBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashBound( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

Imath::Box3f AtomsVariationReader::computeBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	if( path.size() < 3 )
	{
		return unionOfTransformedChildBounds( path, parent );
	}

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
	if (!engineData)
        return { { -1, -1, -1 }, { 1, 1, 1 } };

    auto &variations = engineData->variations();
    auto &meshCache = engineData->meshCache();
    auto agentTypeData = variations.getAgentTypeVariationPtr( path[0] );
    auto cacheMeshIt =  meshCache.find( path[0] );
    if ( agentTypeData && cacheMeshIt != meshCache.cend() )
    {
        std::string geoPathName = path[2];
        for ( size_t i = 3; i < path.size(); ++i )
        {
            geoPathName += "|" + path[i].string();
        }

        auto geoData = agentTypeData->getGeometryPtr( geoPathName );
        if ( geoData && ( geoData->getGeometryFile().find( ".groom" ) == std::string::npos ) )
        {
            auto geoCacheMeshIt = cacheMeshIt->second.find(
                    geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
            if ( geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second )
            {
                auto bbox = geoCacheMeshIt->second->getTypedEntry<AtomsCore::Box3Metadata>( "boundingBox" );
                if ( bbox )
                {
                    return Imath::Box3f(bbox->get().min, bbox->get().max);
                }
                else
                {
                    throw InvalidArgumentException( "AtomsVariationsReader: No bound found for " + geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
                }
            }
            else
            {
                throw InvalidArgumentException( "AtomsVariationsReader: No valid geo found " + geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
            }
        }
    }

    return unionOfTransformedChildBounds( path, parent );
}

void AtomsVariationReader::hashTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashTransform( path, context, parent, h );
	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

Imath::M44f AtomsVariationReader::computeTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
    // The Atoms mesh are stored in world space without transformation
	return {};
}

void AtomsVariationReader::hashAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashAttributes( path, context, parent, h );
	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

IECore::ConstCompoundObjectPtr AtomsVariationReader::computeAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
    if( path.size() < 3 )
    {
        return parent->attributesPlug()->defaultValue();
    }

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        return parent->attributesPlug()->defaultValue();
    }

    auto &translator = AtomsMetadataTranslator::instance();
    auto &variations = engineData->variations();
    auto &meshCache = engineData->meshCache();

    auto agentTypeData = variations.getAgentTypeVariationPtr( path[0] );

    auto cacheMeshIt = meshCache.find( path[0] );
    if ( !( agentTypeData && cacheMeshIt != meshCache.cend() ) )
    {
        return parent->attributesPlug()->defaultValue();
    }

    std::string geoPathName = path[2];
    for ( size_t i = 3; i < path.size(); ++i )
    {
        geoPathName += "|" + path[i].string();
    }

    auto geoData = agentTypeData->getGeometryPtr( geoPathName );
    if ( !geoData )
    {
        return parent->attributesPlug()->defaultValue();
    }

    if ( geoData->getGeometryFile().find( ".groom" ) != std::string::npos )
    {
        return parent->attributesPlug()->defaultValue();
    }


    auto geoCacheMeshIt = cacheMeshIt->second.find(
            geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );

    if ( !( geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second ) )
    {
        return parent->attributesPlug()->defaultValue();
    }

    auto atomsGeo = geoCacheMeshIt->second;
    CompoundObjectPtr result = new CompoundObject;

    IntVectorDataPtr indexCountData = new IntVectorData;
    auto& indexCount = indexCountData->writable();
    IntVectorDataPtr indicesData = new IntVectorData;
    auto& indices = indicesData->writable();
    FloatVectorDataPtr weightsData = new FloatVectorData;
    auto& weights = weightsData->writable();

    for ( auto meshIt = atomsGeo->cbegin(); meshIt != atomsGeo->cend(); ++meshIt )
    {
        if ( meshIt->second->typeId() != AtomsCore::MapMetadata::staticTypeId() )
            continue;

        auto geoMap = std::static_pointer_cast<const AtomsCore::MapMetadata>( meshIt->second );
        if ( !geoMap )
            continue;

        // Store the skin data as attributes
        auto jointWeightsAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>("jointWeights");
        auto jointIndicesAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>("jointIndices");
        if ( jointWeightsAttr && jointIndicesAttr && jointWeightsAttr->size() == jointIndicesAttr->size() )
        {
            for( size_t wId = 0; wId < jointWeightsAttr->size(); ++wId )
            {
                auto jointIndices = jointIndicesAttr->getTypedElement<AtomsCore::IntArrayMetadata>( wId );
                auto jointWeights = jointWeightsAttr->getTypedElement<AtomsCore::DoubleArrayMetadata>( wId );

                if (!jointIndices || !jointWeights)
                    continue;

                indexCount.push_back( jointIndices->get().size() );
                indices.insert(indices.end(), jointIndices->get().begin(), jointIndices->get().end());
                weights.insert(weights.end(), jointWeights->get().begin(), jointWeights->get().end());
            }
        }

        // Convert the atoms metadata to gaffer attribute
        auto atomsAttributeMap = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "atoms" );
        if ( atomsAttributeMap )
        {
            for (auto meshAttrIt = atomsAttributeMap->cbegin(); meshAttrIt != atomsAttributeMap->cend(); ++meshAttrIt)
            {
                auto data = translator.translate( meshAttrIt->second );
                if ( data )
                {
                    result->members()["user:atoms:" + meshAttrIt->first] = data;
                }
            }
        }

        // Convert the arnold metadata to gaffer attribute
        atomsAttributeMap = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "arnold" );
        if ( !atomsAttributeMap )
        {
            continue;
        }

        for (auto meshAttrIt = atomsAttributeMap->cbegin(); meshAttrIt != atomsAttributeMap->cend(); ++meshAttrIt)
        {
            auto data = translator.translate( meshAttrIt->second );
            if ( !data )
            {
                continue;
            }

            // Convert the arnold visible_in_* attribute to ai:visibility:*
            std::string aiParameter = meshAttrIt->first;
            auto visIndex = meshAttrIt->first.find( "visible_in_");
            bool handleParam = false;
            if ( visIndex != std::string::npos )
            {
                aiParameter = "visibility:" + meshAttrIt->first.substr(visIndex + 11);
                handleParam = true;
            }

            // Convert the arnold subdiv* attribute to ai:polymesh:*
            visIndex = meshAttrIt->first.find( "subdiv");
            if ( visIndex != std::string::npos )
            {
                aiParameter = "polymesh:" + meshAttrIt->first;
                handleParam = true;
            }

            // Sett only polymesh arnold attributes
            if ( handleParam ||
                (meshAttrIt->first == "sidedness" ||
                meshAttrIt->first == "receive_shadows" ||
                meshAttrIt->first == "self_shadows" ||
                meshAttrIt->first == "invert_normals" ||
                meshAttrIt->first == "opaque" ||
                meshAttrIt->first == "matte" ||
                meshAttrIt->first == "smoothing"
                )
            )
            {
                switch ( data->typeId() )
                {
                    case IECore::TypeId::DoubleDataTypeId:
                    {
                        FloatDataPtr fData = new FloatData;
                        fData->writable() = runTimeCast<const DoubleData>(data)->readable();
                        result->members()["ai:" + aiParameter] = fData;
                        break;
                    }
                    case IECore::TypeId::V3dDataTypeId:
                    {
                        V3fDataPtr fData = new V3fData;
                        fData->writable() = Imath::V3f(runTimeCast<const V3dData>(data)->readable());
                        result->members()["ai:" + aiParameter] = fData;
                        break;
                    }
                    default:
                    {
                        result->members()["ai:" + aiParameter] = data;
                        break;
                    }
                }
            }
        }
    }

    if ( indexCount.size() > 0 )
    {
        // Store the skin data
        result->members()["jointIndexCount"] = indexCountData;
        result->members()["jointIndices"] = indicesData;
        result->members()["jointWeights"] = weightsData;
    }
    return result;
}

void AtomsVariationReader::hashObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashObject( path, context, parent, h );
	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
    generatePrefPlug()->hash( h );
    generateNrefPlug()->hash( h );
	h.append( &path.front(), path.size() );
}

IECore::ConstObjectPtr AtomsVariationReader::computeObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	if( path.size() < 3 )
	{
		return parent->objectPlug()->defaultValue();
	}

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        return parent->objectPlug()->defaultValue();
    }

    auto &variations = engineData->variations();
    auto &meshCache = engineData->meshCache();
    auto agentTypeData = variations.getAgentTypeVariationPtr( path[0] );
    auto cacheMeshIt =  meshCache.find( path[0] );
    if ( !( agentTypeData && cacheMeshIt != meshCache.cend() ) )
    {
        throw InvalidArgumentException( "AtomsVariationsReader: No geo found" );
    }

    std::string geoPathName = path[2];
    for ( size_t i = 3; i < path.size(); ++i )
    {
        geoPathName += "|" + path[i].string();
    }

    auto geoData = agentTypeData->getGeometryPtr( geoPathName );
    if ( !geoData )
    {
        return parent->objectPlug()->defaultValue();
    }

    if ( geoData->getGeometryFile().find( ".groom" ) != std::string::npos  )
    {
        return parent->objectPlug()->defaultValue();
    }


    auto geoCacheMeshIt = cacheMeshIt->second.find(
            geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
    if ( !( geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second ) )
    {
        return parent->objectPlug()->defaultValue();
    }

    auto atomsGeo = geoCacheMeshIt->second;
    AtomsPtr<AtomsCore::MapMetadata> outGeoMap( new AtomsCore::MapMetadata );
    AtomsPtr<AtomsCore::MeshMetadata> outMeshMeta( new AtomsCore::MeshMetadata );
    AtomsPtr<AtomsCore::ArrayMetadata> outBlendMeta( new AtomsCore::ArrayMetadata );
    AtomsUtils::Mesh& mesh = outMeshMeta->get();

    // A single .geos file could contain multiple mesh, se we need to merge those mesh together
    for ( auto meshIt = atomsGeo->begin(); meshIt!= atomsGeo->end(); ++meshIt )
    {
        if ( meshIt->first == "boundingBox" )
            continue;

        auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>( meshIt->second );
        if ( !geoMap )
            continue;

        mergeAtomsMesh( geoMap, outGeoMap, outMeshMeta, outBlendMeta );
    }

    if ( outBlendMeta->size() > 0 )
    {
        AtomsPtr<AtomsCore::Metadata> outBlendMeshMeta = std::static_pointer_cast<AtomsCore::Metadata>( outBlendMeta );
        outGeoMap->addEntry( "blendShapes" , outBlendMeshMeta, false);
    }

    AtomsPtr<AtomsCore::Metadata> outMeshMetaPtr = std::static_pointer_cast<AtomsCore::Metadata>( outMeshMeta );
    outGeoMap->addEntry( "geo" , outMeshMetaPtr, false);

    return convertAtomsMesh( outGeoMap, generatePrefPlug()->getValue(),  generateNrefPlug()->getValue() );
}

void AtomsVariationReader::hashChildNames( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashChildNames( path, context, parent, h );
	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
	h.append( &path.front(), path.size() );
}

IECore::ConstInternedStringVectorDataPtr AtomsVariationReader::computeChildNames( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
    InternedStringVectorDataPtr resultData = new InternedStringVectorData;
	std::vector<InternedString> &result = resultData->writable();

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return resultData;
    }

    auto& hierarchy = engineData->hierarchy();
    if ( path.empty() )
    {
        for ( auto& agentTypeName: hierarchy->getKeys() )
        {
            result.emplace_back( agentTypeName );
        }
    }
    else
    {
        // Use the full hierarchy computed at the beginning to build the locations structure
        AtomsPtr<const AtomsCore::MapMetadata> currentPath = hierarchy;
        for ( const auto& name: path )
        {
            if ( !currentPath )
            {
                return resultData;
            }

            currentPath = currentPath->getTypedEntry<const AtomsCore::MapMetadata>( name );
            if ( currentPath )
                continue;
        }

        if ( currentPath )
        {
            for ( auto& objName: currentPath->getKeys() )
            {
                result.emplace_back( objName );
            }
        }
    }
	return resultData;
}

void AtomsVariationReader::hashGlobals( const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	h = outPlug()->globalsPlug()->defaultValue()->Object::hash();
}

IECore::ConstCompoundObjectPtr AtomsVariationReader::computeGlobals( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return outPlug()->globalsPlug()->defaultValue();
}

void AtomsVariationReader::hashSetNames( const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashSetNames( context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
}

IECore::ConstInternedStringVectorDataPtr AtomsVariationReader::computeSetNames( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	InternedStringVectorDataPtr resultData = new InternedStringVectorData();

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        return resultData;
    }
    auto &result = resultData->writable();
    auto &variations = engineData->variations();
    for ( auto& agentTypeName: variations.getAgentTypeNames() )
    {
        auto variationPtr = variations.getAgentTypeVariationPtr( agentTypeName );
        if (!variationPtr)
            continue;
        for ( auto& variationName: variationPtr->getGroupNames() )
        {
            result.emplace_back(agentTypeName + ":" + variationName);

            auto groupPtr = variationPtr->getGroupPtr(variationName);
            if (!groupPtr)
                continue;

            for ( auto& lodName: groupPtr->getLodNames() )
            {
                result.emplace_back(agentTypeName + ":" + variationName + ":" + lodName);
            }
        }
    }
	return resultData;
}

void AtomsVariationReader::hashSet( const IECore::InternedString &setName, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashSet( setName, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
	h.append( setName );
}

IECore::ConstPathMatcherDataPtr AtomsVariationReader::computeSet( const IECore::InternedString &setName, const Gaffer::Context *context, const ScenePlug *parent ) const
{
    PathMatcherDataPtr resultData = new PathMatcherData;
    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return resultData;
    }
    auto &result = resultData->writable();
    auto &variations = engineData->variations();

    const std::string& pathName = setName;
    auto sepIndex = pathName.find( ':' );
    if ( sepIndex == std::string::npos )
    {
        return resultData;
    }

    std::string agentTypeName = pathName.substr( 0, sepIndex );
    std::string variationName = pathName.substr( sepIndex + 1, pathName.size() - sepIndex );
    std::string lodName;

    auto agentTypeData = variations.getAgentTypeVariationPtr( agentTypeName );
    if ( !agentTypeData )
        return resultData;

    sepIndex = variationName.rfind( ':' );
    if ( sepIndex != std::string::npos )
    {
        std::string variationNameTmp = variationName.substr( 0, sepIndex );
        std::string lodNameTmp = variationName.substr( sepIndex + 1, variationName.size() - sepIndex );
        const auto groupData = agentTypeData->getGroupPtr( variationNameTmp );
        if ( groupData && groupData->getLodPtr( lodNameTmp ) )
        {
            variationName = variationNameTmp;
            lodName = lodNameTmp;
        }
    }

    std::string currentPath;
    const auto groupData = agentTypeData->getGroupPtr( variationName );
    if ( groupData )
    {
        auto lodPtr = groupData->getLodPtr( lodName );
        if ( lodPtr )
        {
            for( size_t i = 0; i < lodPtr->numCombinations(); ++i )
            {
                auto combination = lodPtr->getCombinationAtIndex( i );
                // skip the groom for now
                auto geoPtr = agentTypeData->getGeometryPtr( combination.first );
                if ( geoPtr && geoPtr->getGeometryFile().find( ".groom" ) != std::string::npos )
                {
                    continue;
                }
                currentPath = "/" + agentTypeName + "/" + variationName + ":" + lodName;
                std::vector<std::string> objectNames;
                AtomsUtils::splitString( combination.first, '|', objectNames );
                for ( const auto& objName: objectNames )
                {
                    currentPath += "/" + objName;
                }
                result.addPath( currentPath );
            }
        }
        else
        {
            for ( size_t i = 0; i < groupData->numCombinations(); ++i )
            {
                auto combination = groupData->getCombinationAtIndex( i );
                // skip the groom for now
                auto geoPtr = agentTypeData->getGeometryPtr( combination.first );
                if ( geoPtr && geoPtr->getGeometryFile().find( ".groom" ) != std::string::npos )
                {
                    continue;
                }
                currentPath = "/" + agentTypeName + "/" + variationName;
                std::vector<std::string> objectNames;
                AtomsUtils::splitString( combination.first, '|', objectNames );
                for ( const auto& objName: objectNames )
                {
                    currentPath += "/" + objName;
                }
                result.addPath( currentPath );
            }
        }
    }
	return resultData;
}

void AtomsVariationReader::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	if( output == enginePlug() )
	{
		atomsAgentFilePlug()->hash( h );
		refreshCountPlug()->hash( h );
	}

	if (output == outPlug()->setNamesPlug() ||
            output == outPlug()->setPlug() ||
            output == outPlug()->transformPlug() ||
            output == outPlug()->attributesPlug() ||
            output == outPlug()->objectPlug() ||
            output == outPlug()->childNamesPlug() ||
            output == outPlug()->boundPlug()
            )
    {
	    enginePlug()->hash(h);
    }
    SceneNode::hash( output, context, h );
}

void AtomsVariationReader::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
	// Both the enginePlug and instanceChildNamesPlug are evaluated
	// in a context in which scene:path holds the parent path for a
	// branch.
	if (output == enginePlug()) {

		static_cast<ObjectPlug *>( output )->setValue(new EngineData(atomsAgentFilePlug()->getValue()));
		return;
	}

	SceneNode::compute(output, context);
}

void AtomsVariationReader::mergeUvSets( AtomsUtils::Mesh& mesh, AtomsUtils::Mesh& inMesh, size_t startSize ) const
{
    // Merge uv sets
    for ( auto &uvSet: inMesh.uvSets() )
    {
        AtomsUtils::Mesh::UVData* uvData = nullptr;
        for ( auto &outUvSet: mesh.uvSets() )
        {
            if ( uvSet.name == outUvSet.name )
            {
                uvData = &outUvSet;
                break;
            }
        }

        if ( !uvData)
        {
            mesh.uvSets().resize( mesh.uvSets().size() + 1 );
            uvData = &mesh.uvSets().back();
            uvData->name = uvSet.name;
        }

        // we need to fill the missing uv data before copy the uv set
        if ( uvData->uvs.size() < startSize )
        {
            size_t fillUvsSize = startSize - uvData->uvs.size();
            for ( size_t uvId = 0; uvId < fillUvsSize; ++uvId )
            {
                uvData->uvIndices.push_back( uvData->uvs.size() );
                uvData->uvs.push_back( Imath::V2f( 0.0, 0.0 ) );
            }
        }

        // Flat uv data,
        auto& uvVec = uvSet.uvs;
        auto& uvIndicesVec = uvSet.uvIndices;
        for ( auto id: uvIndicesVec )
        {
            uvData->uvIndices.push_back( uvData->uvs.size() );
            uvData->uvs.push_back( uvVec[id] );
        }
    }

    for ( auto &uvSet: mesh.uvSets() )
    {
        if ( uvSet.uvs.size() < mesh.normals().size() )
        {
            size_t fillUvsSize = mesh.normals().size() - uvSet.uvs.size();
            for ( size_t uvId = 0; uvId < fillUvsSize; ++uvId )
            {
                uvSet.uvs.push_back( Imath::V2f( 0.0, 0.0 ) );
                uvSet.uvIndices.push_back( uvSet.uvs.size() );
            }
        }
    }
}

void AtomsVariationReader::mergeAtomsMesh(
        AtomsPtr<AtomsCore::MapMetadata>& geoMap,
        AtomsPtr<AtomsCore::MapMetadata>& outGeoMap,
        AtomsPtr<AtomsCore::MeshMetadata>& outMeshMeta,
        AtomsPtr<AtomsCore::ArrayMetadata>& outBlendMeta
    ) const
{
    auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
    if ( !meshMeta )
    {
        meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "cloth" );
        if (!meshMeta)
        {
            return;
        }
    }

    AtomsUtils::Mesh& mesh = outMeshMeta->get();
    AtomsUtils::Mesh& inMesh = meshMeta->get();
    size_t currentPointsSize = mesh.points().size();
    size_t currentNormalsSize = mesh.normals().size();
    size_t currentIndicesSize = mesh.indices().size();

    if ( mesh.uvs().size() < currentIndicesSize )
    {
        mesh.uvs().resize( currentIndicesSize );
    }

    mesh.points().insert( mesh.points().end(), inMesh.points().begin(), inMesh.points().end() );
    mesh.normals().insert( mesh.normals().end(), inMesh.normals().begin(), inMesh.normals().end() );
    mesh.vertexCount().insert( mesh.vertexCount().end(), inMesh.vertexCount().begin(), inMesh.vertexCount().end() );
    mesh.jointIndices().insert( mesh.jointIndices().end(), inMesh.jointIndices().begin(), inMesh.jointIndices().end() );
    mesh.jointWeights().insert( mesh.jointWeights().end(), inMesh.jointWeights().begin(), inMesh.jointWeights().end() );


    auto& inIndices = inMesh.indices();
    auto& outIndices = mesh.indices();
    outIndices.resize( currentIndicesSize + inIndices.size() );
    for (size_t id = 0; id < inIndices.size(); ++id)
    {
        outIndices[ currentIndicesSize + id]  = inIndices[id] + currentPointsSize;
    }

    auto& outUV = mesh.uvs();
    outUV.insert( outUV.end(), inMesh.uvs().begin(), inMesh.uvs().end() );
    if ( outUV.size() < mesh.normals().size() )
    {
        outUV.resize( mesh.normals().size() );
    }

    // Merge the uv sets
    mergeUvSets( mesh, inMesh, currentNormalsSize );

    // Merge the blend shapes
    auto blendShapes = geoMap->getTypedEntry<const AtomsCore::ArrayMetadata>("blendShapes");
    if ( !( blendShapes && blendShapes->size() > 0 ) )
    {
        return;
    }

    // Add missing blend shapes
    for ( unsigned int blendId = outBlendMeta->size(); blendId < blendShapes->size(); blendId++ )
    {
        AtomsCore::MapMetadata blendData;
        AtomsCore::Vector3ArrayMetadata emptyVec;
        blendData.addEntry( "P", &emptyVec );
        blendData.addEntry( "N", &emptyVec );
        outBlendMeta->push_back( &blendData );
    }

    for ( unsigned int blendId = 0; blendId < blendShapes->size(); blendId++ )
    {
        auto blendMap = blendShapes->getTypedElement<AtomsCore::MapMetadata>( blendId );
        if (!blendMap)
            continue;

        auto blendPoints = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "P" );
        auto& blendP = blendPoints->get();

        auto blendNormals = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "N" );
        auto& blendN = blendNormals->get();


        auto outBlendMap = outBlendMeta->getTypedElement<AtomsCore::MapMetadata>( blendId );
        if (!outBlendMap)
            continue;

        auto outBlendPoints = outBlendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "P" );
        auto& outP = outBlendPoints->get();
        if ( outP.size() <  currentPointsSize )
        {
            size_t currentSize = outP.size();
            outP.resize(currentPointsSize);
            for (size_t pbId = currentSize; pbId < currentPointsSize; ++pbId)
            {
                outP[pbId] = mesh.points()[pbId];
            }
        }

        outP.insert( outP.begin(), blendP.begin(), blendP.end() );

        auto outBlendNormals = outBlendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "N" );
        auto& outN = outBlendNormals->get();
        if ( outN.size() <  currentPointsSize )
        {
            size_t currentSize = outN.size();
            outN.resize(currentNormalsSize);
            for (size_t pbId = currentSize; pbId < currentNormalsSize; ++pbId)
            {
                outN[pbId] = mesh.normals()[pbId];
            }
        }

        if ( blendN.size() == inMesh.indices().size() )
        {
            outN.insert( outN.begin(), blendN.begin(), blendN.end() );
        }
        else
        {
            // convert normals from vertex to face varying
            outN.resize( outN.size() + inMesh.indices().size() );
            for ( size_t vId = 0; vId < std::min(inMesh.indices().size(), blendN.size()); ++vId )
            {
                outN[vId] = blendN[inMesh.indices()[vId]];
            }
        }
    }
}
