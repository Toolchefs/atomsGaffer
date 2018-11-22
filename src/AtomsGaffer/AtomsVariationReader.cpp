#include "AtomsGaffer/AtomsVariationReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECoreScene/MeshPrimitive.h"
#include "IECore/NullObject.h"
#include "IECoreScene/PointsPrimitive.h"

#include "boost/lexical_cast.hpp"

#include <functional>
#include <unordered_map>

#include "Atoms/Variations.h"
#include "Atoms/Loaders/MeshLoader.h"
#include "Atoms/GlobalNames.h"
#include "AtomsUtils/PathSolver.h"
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

IECoreScene::PrimitiveVariable convertNormals(AtomsUtils::Mesh& mesh)
{
    auto& inNormals = mesh.normals();
    auto& inIndices = mesh.indices();

    V3fVectorDataPtr normalsData = new V3fVectorData;
    normalsData->setInterpretation( GeometricData::Normal );
    auto &normals = normalsData->writable();
    normals.reserve( inIndices.size() );
    normals.insert(normals.begin(), inNormals.begin(), inNormals.end());
    return PrimitiveVariable( PrimitiveVariable::FaceVarying, normalsData );
}

IECoreScene::PrimitiveVariable convertUvs( AtomsUtils::Mesh& mesh, AtomsUtils::Mesh::UVData& uvSet)
{
    auto& inIndices = uvSet.uvIndices;

    IntVectorDataPtr indexData = new IntVectorData;
    vector<int> &indices = indexData->writable();
    indices.insert(indices.begin(),  inIndices.begin(), inIndices.end());

    V2fVectorDataPtr uvData = new V2fVectorData;
    uvData->setInterpretation( GeometricData::UV );
    uvData->writable() = uvSet.uvs;

    return PrimitiveVariable( PrimitiveVariable::FaceVarying, uvData, indexData );
}

MeshPrimitivePtr convertAtomsMesh(AtomsPtr<AtomsCore::MapMetadata>& geoMap)
{
    if (!geoMap)
    {
        return MeshPrimitivePtr();
    }

    auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>("geo");
    if (!meshMeta)
    {
        meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>("cloth");
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
    outCount.insert(outCount.begin(), vertexCount.begin(), vertexCount.end());

    IntVectorDataPtr vertexIds = new IntVectorData;
    auto &outIndices = vertexIds->writable();
    outIndices.insert(outIndices.begin(), vertexIndices.begin(), vertexIndices.end());

    V3fVectorDataPtr p = new V3fVectorData;
    auto &outP = p->writable();
    outP = points;

    MeshPrimitivePtr meshPtr = new MeshPrimitive(verticesPerFace, vertexIds, "linear", p);

    meshPtr->variables["N"] = convertNormals(mesh);
    meshPtr->variables["Nref"] = meshPtr->variables["N"];
    meshPtr->variables["Pref"] = meshPtr->variables["P"];

    auto& uvSets = mesh.uvSets();
    if (uvSets.size() > 0) {
        for (int i = 0; i < uvSets.size(); ++i) {
            auto &uvSet = uvSets[i];
            auto uvPrim = convertUvs(mesh, uvSet);
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

        meshPtr->variables["uv"] = PrimitiveVariable( PrimitiveVariable::FaceVarying, uvData, vertexIds );
    }

    auto blendShapes = geoMap->getTypedEntry<const AtomsCore::ArrayMetadata>("blendShapes");
    if (blendShapes && blendShapes->size() > 0)
    {
        for (unsigned int blendId = 0; blendId < blendShapes->size(); blendId++)
        {
            auto blendMap = blendShapes->getTypedElement<AtomsCore::MapMetadata>(blendId);
            if (!blendMap)
                continue;


            auto blendPoints = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>("P");
            auto& blendP = blendPoints->get();



            auto blendNormals = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>("N");
            auto& blendN = blendNormals->get();


            V3fVectorDataPtr normalsData = new V3fVectorData;
            normalsData->setInterpretation( GeometricData::Normal );
            auto &normals = normalsData->writable();
            normals.reserve( blendN.size() );
            if (blendN.size() == vertexIndices.size()) {
                normals.insert(normals.begin(), blendN.begin(), blendN.end());
            }
            else
            {
                // convert normals from vertex to face varying
                normals.resize(vertexIndices.size());
                for (size_t vId = 0; vId < std::min(vertexIndices.size(), blendN.size()); ++vId)
                {
                    normals[vId] = blendN[vertexIndices[vId]];
                }
            }

            meshPtr->variables["blendShape_" + std::to_string(blendId) + "_N"] = PrimitiveVariable(
                    PrimitiveVariable::FaceVarying, normalsData);

            V3fVectorDataPtr blendPointsData = new V3fVectorData;
            auto &blendPts = blendPointsData->writable();
            blendPts.reserve( blendP.size() );
            blendPts.insert(blendPts.begin(), blendP.begin(), blendP.end());
            meshPtr->variables["blendShape_" + std::to_string(blendId) + "_P"] =  PrimitiveVariable( PrimitiveVariable::Vertex, blendPointsData );
        }

        IntDataPtr blendShapeCount = new IntData(blendShapes->size());
        meshPtr->variables["blendShapeCount"] =  PrimitiveVariable( PrimitiveVariable::Constant, blendShapeCount );
    }

    auto atomsMap = geoMap->getTypedEntry<const AtomsCore::MapMetadata>("atoms");
    if (atomsMap && atomsMap->size() > 0)
    {
        auto &translator = AtomsMetadataTranslator::instance();
        for(auto it = atomsMap->cbegin(); it != atomsMap->cend(); ++it)
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

    EngineData(const std::string& filePath):
        m_filePath(filePath)
    {
        m_variations = Atoms::loadVariationFromFile(AtomsUtils::solvePath((filePath)));

        auto agentTypeNames = m_variations.getAgentTypeNames();
        for (size_t aTypeId = 0; aTypeId != agentTypeNames.size(); ++aTypeId)
        {
            auto agentTypePtr = m_variations.getAgentTypeVariationPtr(agentTypeNames[aTypeId]);
            if (!agentTypePtr)
                continue;


            auto geoNames = agentTypePtr->getGeometryNames();
            for (size_t geoId = 0; geoId != geoNames.size(); ++geoId) {
                auto geoPtr = agentTypePtr->getGeometryPtr(geoNames[geoId]);
                if (!geoPtr)
                    continue;

                if (geoPtr->getGeometryFile().find(".groom") != std::string::npos)
                    continue;

                auto atomsGeoMap = Atoms::loadMesh(geoPtr->getGeometryFile(), geoPtr->getGeometryFilter());
                if (!atomsGeoMap) {
                    throw InvalidArgumentException("AtomsVariationsReader: Invalid geo: " + geoPtr->getGeometryFile() +":" + geoPtr->getGeometryFilter());
                }

                //compute the bouding box
                AtomsCore::Box3f bbox;
                for (auto meshIt = atomsGeoMap->begin(); meshIt != atomsGeoMap->end(); ++meshIt) {
                    if (meshIt->second->typeId() !=  AtomsCore::MapMetadata::staticTypeId())
                        continue;

                    auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>(meshIt->second);
                    AtomsPtr<const AtomsCore::MapMetadata> atomsMapPtr = geoMap->getTypedEntry<const AtomsCore::MapMetadata>("atoms");
                    if (atomsMapPtr)
                    {
                        auto hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>(ATOMS_CLOTH_HIDE_MESH);
                        if (hideMeshPtr && hideMeshPtr->value())
                            continue;

                        hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>(ATOMS_PREVIEW_MESH);
                        if (hideMeshPtr && hideMeshPtr->value())
                            continue;
                    }


                    auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>("geo");
                    if (!meshMeta) {
                        meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>("cloth");
                        if (!meshMeta) {
                            continue;
                        }
                    }

                    for(const auto& p: meshMeta->get().points())
                        bbox.extendBy(p);

                }

                if (bbox.isEmpty())
                    continue;

                AtomsCore::Box3Metadata boxMeta;
                boxMeta.get().extendBy(bbox.min);
                boxMeta.get().extendBy(bbox.max);
                atomsGeoMap->addEntry("boundingBox", &boxMeta);
                m_meshesFileCache[agentTypeNames[aTypeId]][geoPtr->getGeometryFile() + ":" +geoPtr->getGeometryFilter()] = atomsGeoMap;
            }
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

};

size_t AtomsVariationReader::g_firstPlugIndex = 0;

AtomsVariationReader::AtomsVariationReader( const std::string &name )
	:	SceneNode( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsAgentFile" ) );
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

IntPlug* AtomsVariationReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const IntPlug* AtomsVariationReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::ObjectPlug *AtomsVariationReader::enginePlug()
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::ObjectPlug *AtomsVariationReader::enginePlug() const
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 2 );
}

void AtomsVariationReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{


	if( input == atomsAgentFilePlug() || input == refreshCountPlug() )
	{
		outputs.push_back( enginePlug() );
	}

	if(input == enginePlug())
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
    auto agentTypeData = variations.getAgentTypeVariationPtr(path[0]);
    auto cacheMeshIt =  meshCache.find(path[0]);
    if (agentTypeData && cacheMeshIt != meshCache.cend()) {
        auto geoData = agentTypeData->getGeometryPtr(path[2]);
        if (geoData &&  (geoData->getGeometryFile().find(".groom") == std::string::npos)) {
            auto geoCacheMeshIt = cacheMeshIt->second.find(
                    geoData->getGeometryFile() + ":" + geoData->getGeometryFilter());
            if (geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second) {
                // todo merge multiple mesh here
                auto bbox = geoCacheMeshIt->second->getTypedEntry<AtomsCore::Box3Metadata>("boundingBox");
                if(bbox) {
                    return Imath::Box3f(bbox->get().min, bbox->get().max);
                }
                else
                {
                    throw InvalidArgumentException("AtomsVariationsReader: No bound found for " + geoData->getGeometryFile() + ":" + geoData->getGeometryFilter());
                }
            }
            else
            {
                throw InvalidArgumentException("AtomsVariationsReader: No valid geo found " + geoData->getGeometryFile() + ":" + geoData->getGeometryFilter());
            }
        }
    }

    return { { -1, -1, -1 }, { 1, 1, 1 } };
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
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
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

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>(enginePlug()->getValue());
    if (!engineData) {
        return parent->attributesPlug()->defaultValue();
    }

    CompoundObjectPtr result = new CompoundObject;
    auto &translator = AtomsMetadataTranslator::instance();
    auto &variations = engineData->variations();
    auto &meshCache = engineData->meshCache();

    auto agentTypeData = variations.getAgentTypeVariationPtr(path[0]);
    auto cacheMeshIt = meshCache.find(path[0]);
    if (agentTypeData && cacheMeshIt != meshCache.cend()) {
        auto geoData = agentTypeData->getGeometryPtr(path[2]);
        if (geoData && (geoData->getGeometryFile().find(".groom") == std::string::npos)) {
            auto geoCacheMeshIt = cacheMeshIt->second.find(
                    geoData->getGeometryFile() + ":" + geoData->getGeometryFilter());

            if (geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second) {
                /// todo merge multiple mesh here
                auto atomsGeo = geoCacheMeshIt->second;

                for (auto meshIt = atomsGeo->cbegin(); meshIt != atomsGeo->cend(); ++meshIt) {
                    if (meshIt->second->typeId() != AtomsCore::MapMetadata::staticTypeId())
                        continue;

                    auto geoMap = std::static_pointer_cast<const AtomsCore::MapMetadata>(meshIt->second);
                    if (!geoMap)
                        continue;

                    auto jointWeightsAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>("jointWeights");
                    auto jointIndicesAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>("jointIndices");
                    if (jointWeightsAttr && jointIndicesAttr && jointWeightsAttr->size() == jointIndicesAttr->size())
                    {
                        IntVectorDataPtr indexCountData = new IntVectorData;
                        auto& indexCount = indexCountData->writable();
                        indexCount.reserve(jointWeightsAttr->size());
                        IntVectorDataPtr indicesData = new IntVectorData;
                        auto& indices = indicesData->writable();
                        FloatVectorDataPtr weightsData = new FloatVectorData;
                        auto& weights = weightsData->writable();

                        for(size_t wId=0; wId < jointWeightsAttr->size(); ++wId)
                        {
                            auto jointIndices = jointIndicesAttr->getTypedElement<AtomsCore::IntArrayMetadata>(wId);
                            auto jointWeights = jointWeightsAttr->getTypedElement<AtomsCore::DoubleArrayMetadata>(wId);

                            if (!jointIndices || !jointWeights)
                                continue;

                            indexCount.push_back(jointIndices->get().size());
                            indices.insert(indices.end(), jointIndices->get().begin(), jointIndices->get().end());
                            weights.insert(weights.end(), jointWeights->get().begin(), jointWeights->get().end());

                        }

                        result->members()["jointIndexCount"] = indexCountData;
                        result->members()["jointIndices"] = indicesData;
                        result->members()["jointWeights"] = weightsData;
                    }

                    for (auto meshAttrIt = geoMap->cbegin(); meshAttrIt != geoMap->cend(); ++meshAttrIt) {
                        if (meshAttrIt->first == "jointIndices" ||
                                meshAttrIt->first == "jointIndices" ||
                                meshAttrIt->first == "jointWeights" ||
                                meshAttrIt->first == "geo" ||
                                meshAttrIt->first == "cloth" ||
                                meshAttrIt->first == "blendShapes")
                        {
                            continue;
                        }

                        auto data = translator.translate(meshAttrIt->second);
                        if (data)
                            result->members()[meshAttrIt->first] = data;
                    }
                }
            }
        }
    }

	return result;
}

void AtomsVariationReader::hashObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashObject( path, context, parent, h );
	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

IECore::ConstObjectPtr AtomsVariationReader::computeObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	if( path.size() < 3 )
	{
		return parent->objectPlug()->defaultValue();
	}

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return MeshPrimitive::createBox( Imath::Box3f( { -1, -1, -1 }, { 1, 1, 1 } ) );
    }

    auto &variations = engineData->variations();
    auto &meshCache = engineData->meshCache();
    auto agentTypeData = variations.getAgentTypeVariationPtr(path[0]);
    auto cacheMeshIt =  meshCache.find(path[0]);
    if (agentTypeData && cacheMeshIt != meshCache.cend()) {
        auto geoData = agentTypeData->getGeometryPtr(path[2]);
        if (geoData && (geoData->getGeometryFile().find(".groom") == std::string::npos)) {
            auto geoCacheMeshIt = cacheMeshIt->second.find(
                    geoData->getGeometryFile() + ":" + geoData->getGeometryFilter());
            if (geoCacheMeshIt != cacheMeshIt->second.cend() && geoCacheMeshIt->second) {
                /// todo merge multiple mesh here
                auto atomsGeo = geoCacheMeshIt->second;
                for (auto meshIt = atomsGeo->begin(); meshIt!= atomsGeo->end(); ++meshIt) {
                    if (meshIt->first == "boundingBox")
                        continue;
                    auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>(meshIt->second);
                    return convertAtomsMesh(geoMap);
                }
                return MeshPrimitive::createSphere( 1 );
            }
        }
    } else {
        throw InvalidArgumentException("AtomsVariationsReader: No geo found");
    }

    return MeshPrimitive::createBox( Imath::Box3f( { -1, -1, -1 }, { 1, 1, 1 } ) );
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
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.

	InternedStringVectorDataPtr resultData = new InternedStringVectorData;
	std::vector<InternedString> &result = resultData->writable();

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return resultData;
    }
    auto &variations = engineData->variations();
	if (path.empty())
    {

        auto agentTypeNames = variations.getAgentTypeNames();
        for (auto& agentTypeName: agentTypeNames)
        {
            result.emplace_back(agentTypeName);
        }
    }
    else if (path.size() == 1)
    {
        auto agentTypeData = variations.getAgentTypeVariationPtr(path[0]);
        if (agentTypeData)
        {
            auto variationNames = agentTypeData->getGroupNames();
            for (auto& variationName: variationNames)
            {
                result.emplace_back(variationName);

                const auto groupData = agentTypeData->getGroupPtr(variationName);
                if (groupData) {
                    for (const auto &lodName: groupData->getLodNames()) {
                        auto lodPtr = groupData->getLodPtr(lodName);
                        if (!lodPtr)
                            continue;
                        result.emplace_back(variationName +':' + lodName);
                    }
                }
            }


        }
    }
    else if (path.size() == 2)
    {
        auto agentTypeData = variations.getAgentTypeVariationPtr(path[0]);
        if (agentTypeData)
        {
            std::string pathName = path[1].string();
            std::string variationName = path[1].string();
            std::string lodName = "";
            auto sepIndex = variationName.rfind(':');
            if (sepIndex != std::string::npos)
            {
                std::string variationNameTmp = pathName.substr(0, sepIndex);
                std::string lodNameTmp = pathName.substr(sepIndex + 1, pathName.size() - sepIndex );
                const auto groupData = agentTypeData->getGroupPtr(variationNameTmp);
                if (groupData && groupData->getLodPtr(lodNameTmp))
                {
                    variationName = variationNameTmp;
                    lodName = lodNameTmp;
                }
            }

            const auto groupData = agentTypeData->getGroupPtr(variationName);
            if (groupData)
            {
                auto lodPtr = groupData->getLodPtr(lodName);
                if (lodPtr)
                {
                    for(size_t i = 0; i < lodPtr->numCombinations(); ++i)
                    {
                        auto combination = lodPtr->getCombinationAtIndex(i);
                        // skip the groom for now
                        auto geoPtr = agentTypeData->getGeometryPtr(combination.first);
                        if (geoPtr && geoPtr->getGeometryFile().find(".groom") != std::string::npos)
                        {
                            continue;
                        }
                        result.emplace_back(combination.first);
                    }
                } else {
                    for (size_t i = 0; i < groupData->numCombinations(); ++i) {
                        auto combination = groupData->getCombinationAtIndex(i);
                        // skip the groom for now
                        auto geoPtr = agentTypeData->getGeometryPtr(combination.first);
                        if (geoPtr && geoPtr->getGeometryFile().find(".groom") != std::string::npos) {
                            continue;
                        }
                        result.emplace_back(combination.first);
                    }
                }
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
    if (!engineData) {
        return resultData;
    }
    auto &result = resultData->writable();
    auto &variations = engineData->variations();
    for (auto& agentTypeName: variations.getAgentTypeNames())
    {
        auto variationPtr = variations.getAgentTypeVariationPtr(agentTypeName);
        if (!variationPtr)
            continue;
        for (auto& variationName: variationPtr->getGroupNames()) {
            result.emplace_back(agentTypeName + ":" + variationName);

            auto groupPtr = variationPtr->getGroupPtr(variationName);
            if (!groupPtr)
                continue;

            for (auto& lodName: groupPtr->getLodNames()) {
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
    enginePlug()->getValue();
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	PathMatcherDataPtr resultData = new PathMatcherData;

    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return resultData;
    }
    auto &result = resultData->writable();
    auto &variations = engineData->variations();

    std::string pathName = setName;
    auto sepIndex = pathName.find(':');
    if (sepIndex == std::string::npos)
    {
        return resultData;
    }


    std::string agentTypeName = pathName.substr(0, sepIndex);
    std::string variationName = pathName.substr(sepIndex + 1, pathName.size() - sepIndex );
    std::string lodName = "";

    auto agentTypeData = variations.getAgentTypeVariationPtr(agentTypeName);
    if (!agentTypeData)
        return resultData;

    sepIndex = variationName.rfind(':');
    if (sepIndex != std::string::npos)
    {
        std::string variationNameTmp = variationName.substr(0, sepIndex);
        std::string lodNameTmp = variationName.substr(sepIndex + 1, variationName.size() - sepIndex );
        const auto groupData = agentTypeData->getGroupPtr(variationNameTmp);
        if (groupData && groupData->getLodPtr(lodNameTmp))
        {
            variationName = variationNameTmp;
            lodName = lodNameTmp;
        }
    }

    std::string currentPath;
    const auto groupData = agentTypeData->getGroupPtr(variationName);
    if (groupData)
    {
        auto lodPtr = groupData->getLodPtr(lodName);
        if (lodPtr)
        {
            for(size_t i = 0; i < lodPtr->numCombinations(); ++i)
            {
                auto combination = lodPtr->getCombinationAtIndex(i);
                // skip the groom for now
                auto geoPtr = agentTypeData->getGeometryPtr(combination.first);
                if (geoPtr && geoPtr->getGeometryFile().find(".groom") != std::string::npos)
                {
                    continue;
                }
                currentPath = "/" + agentTypeName + "/" + variationName + ":" + lodName + "/" + combination.first;
                result.addPath(currentPath);
            }
        } else {
            for (size_t i = 0; i < groupData->numCombinations(); ++i) {
                auto combination = groupData->getCombinationAtIndex(i);
                // skip the groom for now
                auto geoPtr = agentTypeData->getGeometryPtr(combination.first);
                if (geoPtr && geoPtr->getGeometryFile().find(".groom") != std::string::npos) {
                    continue;
                }
                currentPath = "/" + agentTypeName + "/" + variationName + "/" + combination.first;
                result.addPath(currentPath);

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