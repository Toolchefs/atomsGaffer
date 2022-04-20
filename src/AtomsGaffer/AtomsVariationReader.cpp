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

#include "AtomsGaffer/AtomsVariationReader.h"
#include "AtomsGaffer/AtomsMetadataTranslator.h"
#include "AtomsGaffer/AtomsMathTranaslator.h"

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
#include "AtomsCore/Metadata/MatrixMetadata.h"
#include "AtomsCore/Metadata/StringMetadata.h"
#include "AtomsCore/Metadata/IntArrayMetadata.h"
#include "AtomsCore/Metadata/DoubleArrayMetadata.h"
#include "AtomsCore/Metadata/Vector3ArrayMetadata.h"
#include "AtomsCore/Metadata/StringArrayMetadata.h"
#include "AtomsCore/Metadata/BoolArrayMetadata.h"

#include <tbb/tbb.h>
#include <list>

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
    convertFromAtoms( normals, inNormals );
    return PrimitiveVariable( PrimitiveVariable::FaceVarying, normalsData );
}

IECoreScene::PrimitiveVariable convertUvs( AtomsUtils::Mesh& mesh, AtomsUtils::Mesh::UVData& uvSet )
{
    auto& inIndices = uvSet.uvIndices;

    V2fVectorDataPtr uvData = new V2fVectorData;
    uvData->setInterpretation( GeometricData::UV );
    convertFromAtoms( uvData->writable(), uvSet.uvs );
    IECore::IntVectorDataPtr uvIndicesData = new IECore::IntVectorData;
    auto &uvIndices = uvIndicesData->writable();
    uvIndices.insert( uvIndices.end(), inIndices.begin(), inIndices.end() );

    return PrimitiveVariable( PrimitiveVariable::FaceVarying, uvData, uvIndicesData );
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
    convertFromAtoms( outP, points );

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

            if (i == 0) {
                meshPtr->variables["uv"] = uvPrim;
            }
            else
            {
                meshPtr->variables[uvSet.name] = uvPrim;
            }
        }
    }
    else
    {
        V2fVectorDataPtr uvData = new V2fVectorData;
        uvData->setInterpretation( GeometricData::UV );
        convertFromAtoms( uvData->writable(), mesh.uvs() );

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
                convertFromAtoms( normals, blendN );
            }
            else
            {
                // convert normals from vertex to face varying
                normals.resize( vertexIndices.size() );
                for ( size_t vId = 0; vId < std::min(vertexIndices.size(), blendN.size()); ++vId )
                {
                    convertFromAtoms( normals[vId], blendN[vertexIndices[vId]] );
                }
            }

            meshPtr->variables["blendShape_" + std::to_string( blendId ) + "_N"] =
                    PrimitiveVariable( PrimitiveVariable::FaceVarying, normalsData );

            V3fVectorDataPtr blendPointsData = new V3fVectorData;
            auto &blendPts = blendPointsData->writable();
            blendPts.reserve( blendP.size() );
            convertFromAtoms( blendPts, blendP );
            meshPtr->variables["blendShape_" + std::to_string( blendId ) + "_P"] =
                    PrimitiveVariable( PrimitiveVariable::Vertex, blendPointsData );
        }

        IntDataPtr blendShapeCount = new IntData( blendShapes->size() );
        meshPtr->variables["blendShapeCount"] =  PrimitiveVariable( PrimitiveVariable::Constant, blendShapeCount );
    }

    // Convert the atoms prim vars
    auto &translator = AtomsMetadataTranslator::instance();
    auto primVarsMap = geoMap->getTypedEntry<const AtomsCore::MapMetadata>( "primVars" );
    if ( primVarsMap )
    {
        for ( auto primIt = primVarsMap->cbegin(); primIt != primVarsMap->cend(); ++primIt )
        {
            if ( primIt->second->typeId() != AtomsCore::MapMetadata::staticTypeId() )
                continue;

            auto primPtr = std::static_pointer_cast<const AtomsCore::MapMetadata>(primIt->second);
            auto interpolationPtr = primPtr->getTypedEntry<AtomsCore::StringMetadata>( "interpolation" );
            auto dataPtr = primPtr->getEntry( "data" );
            if ( interpolationPtr && dataPtr )
            {
                auto interpolation = PrimitiveVariable::Constant;
                if ( interpolationPtr->get() == "uniform" )
                    interpolation = PrimitiveVariable::Uniform;
                else if ( interpolationPtr->get() == "vertex" )
                    interpolation = PrimitiveVariable::Vertex;
                if ( interpolationPtr->get() == "varying" )
                    interpolation = PrimitiveVariable::Varying;
                if ( interpolationPtr->get() == "faceVarying" )
                    interpolation = PrimitiveVariable::FaceVarying;

                meshPtr->variables[primIt->first] = PrimitiveVariable( interpolation,
                                                                  translator.translate( dataPtr ) );
            }
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

    class AtomsDagNode
    {
    public:
        std::string name;
        AtomsPtr<AtomsCore::MapMetadata> data;
        std::set<std::string> variations;
        std::map<std::string, AtomsDagNode> children;

        size_t memSize() const
        {
            size_t totalMemory = AtomsCore::memSize( name );
            if ( data )
                totalMemory += data->memSize();
            totalMemory += AtomsCore::memSize( variations );
            for (auto it = children.cbegin(); it != children.cend(); ++it)
            {
                totalMemory += AtomsCore::memSize( it->first );
                totalMemory += it->second.memSize();
            }

            totalMemory += sizeof( this );
            return totalMemory;
        }
    };

    EngineData( const std::string& filePath ):
        m_filePath( filePath ),
        m_totalMemory( 0 )
    {
        m_hierarchy = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
        m_root.children.clear();
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
            auto& agentTypeRoot = m_root.children[agentTypeName];
            agentTypeRoot.name = agentTypeName;
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

                m_defaultSets.push_back( agentTypeName + ":" + variationName );

                AtomsPtr<AtomsCore::MapMetadata> variationHierarchy( new AtomsCore::MapMetadata );

                for(unsigned int cId = 0; cId < variationPtr->numCombinations(); ++cId )
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
                    AtomsDagNode* currentRoot = &agentTypeRoot;
                    currentRoot->variations.insert(variationName);
                    for( const auto& objName: objectNames )
                    {
                        if ( objName == "" )
                            continue;
                        currentRoot = &currentRoot->children[objName];
                        currentRoot->name = objName;
                        currentRoot->variations.insert( variationName );
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


                for ( const auto& lodName: variationPtr->getLodNames() )
                {
                    auto lodPtr = variationPtr->getLodPtr( lodName );
                    if ( !lodPtr )
                        continue;

                    m_defaultSets.push_back( agentTypeName + ":" + variationName + ":" + lodName );
                    AtomsPtr<AtomsCore::MapMetadata> lodHierarchy( new AtomsCore::MapMetadata );

                    for( unsigned int cId = 0; cId < lodPtr->numCombinations(); ++cId ) {
                        auto &combinationLod = lodPtr->getCombinationAtIndex( cId );

                        auto geoPtr = agentTypePtr->getGeometryPtr( combinationLod.first );
                        if (!geoPtr)
                            continue;

                        if ( agentTypeIt->second.find(
                                geoPtr->getGeometryFile() + ":" + geoPtr->getGeometryFilter() ) ==
                            agentTypeIt->second.end())
                            continue;

                        // Split the full path geo name to build the full hierarchy
                        std::vector<std::string> lodObjectNames;
                        AtomsUtils::splitString( combinationLod.first, '|', lodObjectNames );

                        auto currentMap = lodHierarchy;
                        AtomsDagNode* currentRoot = &agentTypeRoot;
                        currentRoot->variations.insert( variationName + ":" + lodName );
                        for ( const auto &objName: lodObjectNames ) {
                            if ( objName == "" )
                                continue;
                            currentRoot = &currentRoot->children[objName];
                            currentRoot->name = objName;
                            currentRoot->variations.insert( variationName + ":" + lodName );
                            auto objIt = currentMap->getTypedEntry<AtomsCore::MapMetadata>(objName);
                            if (!objIt) {
                                AtomsCore::MapMetadata emptyData;
                                currentMap->addEntry(objName, &emptyData);
                                objIt = currentMap->getTypedEntry<AtomsCore::MapMetadata>(objName);
                            }
                            currentMap = objIt;
                        }
                    }

                    auto lodHierarchyData = std::static_pointer_cast<AtomsCore::Metadata>( lodHierarchy );
                    agentTypeHierarchy->addEntry( variationName + ":" + lodName, lodHierarchyData, false );
                }

            }

            auto agentTypeHierarchyData = std::static_pointer_cast<AtomsCore::Metadata>( agentTypeHierarchy );
            m_hierarchy->addEntry( agentTypeNames[aTypeId],agentTypeHierarchyData, false );
        }

        m_totalMemory = 0;
        for (const auto& agentTypeName: m_variations.getAgentTypeNames() )
        {
            m_totalMemory += AtomsCore::memSize( agentTypeName );
            auto aTypePtr = m_variations.getAgentTypeVariationPtr( agentTypeName );
            if ( !aTypePtr )
                continue;

            for (const auto& name: aTypePtr->getGeometryNames() )
            {
                m_totalMemory +=  AtomsCore::memSize( name );
                auto dataPtr = aTypePtr->getGeometryPtr( name );
                if ( !dataPtr )
                    continue;

                m_totalMemory +=  AtomsCore::memSize( dataPtr->getGeometryFile() );
                m_totalMemory += AtomsCore::memSize( dataPtr->getGeometryFilter() );
                m_totalMemory += sizeof(*dataPtr);
            }

            for ( const auto& name: aTypePtr->getMaterialNames() )
            {
                m_totalMemory += AtomsCore::memSize( name );
                auto dataPtr = aTypePtr->getMaterialPtr( name );
                if ( !dataPtr )
                    continue;

                m_totalMemory += AtomsCore::memSize( dataPtr->getMaterialFile() );
                m_totalMemory += AtomsCore::memSize( dataPtr->getDiffuseFile() );
                m_totalMemory += AtomsCore::memSize( dataPtr->getSpecularFile() );
                m_totalMemory += AtomsCore::memSize( dataPtr->getNormalMapFile() );
                m_totalMemory += sizeof(*dataPtr);
            }

            for (const auto& name: aTypePtr->getGroupNames() )
            {
                m_totalMemory += AtomsCore::memSize( name );
                auto dataPtr = aTypePtr->getGroupPtr( name );
                if ( !dataPtr )
                    continue;

                for ( unsigned int id = 0; id < dataPtr->numCombinations(); ++id )
                {
                    auto variation = dataPtr->getCombinationAtIndex( id );
                    m_totalMemory += AtomsCore::memSize( variation.first );
                    m_totalMemory += AtomsCore::memSize( variation.second );
                    m_totalMemory += sizeof(variation);
                }

                for ( const auto& lodName: dataPtr->getLodNames() )
                {
                    m_totalMemory += AtomsCore::memSize( lodName );
                    auto lodDataPtr = dataPtr->getLodPtr( lodName );
                    if ( !lodDataPtr )
                        continue;

                    for ( unsigned int id = 0; id < lodDataPtr->numCombinations(); ++id )
                    {
                        auto variation = dataPtr->getCombinationAtIndex( id );
                        m_totalMemory += AtomsCore::memSize( variation.first );
                        m_totalMemory += AtomsCore::memSize( variation.second );
                        m_totalMemory += sizeof( variation );
                    }
                }


                m_totalMemory +=  sizeof(*dataPtr);
            }
        }


        m_totalMemory += AtomsCore::memSize( m_filePath );

        for (auto aTypeIt = m_meshesFileCache.cbegin(); aTypeIt != m_meshesFileCache.cend(); ++aTypeIt)
        {
            for (auto meshIt = aTypeIt->second.cbegin(); meshIt != aTypeIt->second.cend(); ++meshIt)
            {
                if ( meshIt->second )
                    m_totalMemory += meshIt->second->memSize();
            }
        }

        if ( m_hierarchy )
            m_totalMemory += m_hierarchy->memSize();

        m_totalMemory += AtomsCore::memSize( m_defaultSets );

        m_totalMemory += m_root.memSize();
    }

    virtual ~EngineData()
    {

    }

    void flatMeshHierarchy( AtomsCore::MapMetadata* geos, AtomsPtr<AtomsCore::MapMetadata>& outMap, AtomsDagNode* rootNode )
    {
        auto childrenPtr = geos->getTypedEntry<AtomsCore::MapMetadata>( "children" );
        if ( !childrenPtr )
            return;

        for ( auto it = childrenPtr->begin(); it != childrenPtr->end(); it++ )
        {
            auto meshGroup = std::static_pointer_cast<AtomsCore::MapMetadata>( it->second );
            auto meshMeta = meshGroup->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
            if ( meshMeta ) {
                outMap->addEntry( it->first, it->second, false );
            }

            AtomsDagNode* childRootNode = nullptr;
            if ( rootNode ) {
                std::vector<std::string> objectNames;
                AtomsUtils::splitString( it->first, '|', objectNames );

                if ( objectNames.size() > 0)
                {
                    childRootNode = &rootNode->children[objectNames.back()];
                    childRootNode->data = meshGroup;
                }
            }
            flatMeshHierarchy( meshGroup.get(), outMap, childRootNode );
        }
    }

    void swapMapGeoData( AtomsPtr<AtomsCore::MapMetadata>& inputGeoMap, AtomsPtr<AtomsCore::MapMetadata>& outMap,
                        const std::string& name )
    {
        auto atomsAttr = inputGeoMap->getEntry( name );
        if ( atomsAttr ) {
            outMap->addEntry( name, atomsAttr, false );
            inputGeoMap->eraseEntry(name);
        }
    }

    void swapPrimVarData( AtomsPtr<AtomsCore::MapMetadata>& inputGeoMap, AtomsPtr<AtomsCore::MapMetadata>& outMap,
                         const std::string& name )
    {
        auto atomsAttr = inputGeoMap->getEntry( name );
        if ( atomsAttr ) {
            AtomsPtr<AtomsCore::MapMetadata> data( new AtomsCore::MapMetadata );
            AtomsCore::StringMetadata interpolation("vertex");
            data->addEntry("interpolation", &interpolation);
            data->addEntry("data", atomsAttr, false);
            auto dataPtr = std::static_pointer_cast<AtomsCore::Metadata>( data );
            outMap->addEntry( name, dataPtr, false );
            inputGeoMap->eraseEntry(name);
        }
    }

    AtomsPtr<AtomsCore::MapMetadata> convertMeshFormatV2( AtomsPtr<AtomsCore::MapMetadata> inputMap )
    {
        if ( !inputMap )
            return inputMap;

        auto versionPtr = inputMap->getTypedEntry<AtomsCore::IntMetadata>( "version" );
        if ( versionPtr && versionPtr->value() == 2 )
            return inputMap;

        AtomsPtr<AtomsCore::MapMetadata> geoMap( new AtomsCore::MapMetadata );

        AtomsCore::IntMetadata version( 2 );
        geoMap->addEntry( "version", &version );

        AtomsPtr<AtomsCore::MapMetadata> childrenMap( new AtomsCore::MapMetadata );
        auto childrenMapBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( childrenMap );
        geoMap->addEntry( "children", childrenMapBasePtr, false );

        for ( auto it = inputMap->begin(); it != inputMap->end(); ++it ) {
            if ( it->second->typeId() == AtomsCore::MeshMetadata::staticTypeId() ) {
                AtomsPtr<AtomsCore::MapMetadata> geoContainer( new AtomsCore::MapMetadata );
                auto geoContainerBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( geoContainer );
                childrenMap->addEntry( it->first, geoContainerBasePtr, false );

                auto materialAttributes = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                auto materialAttributesBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( materialAttributes );
                geoContainer->addEntry( "material", materialAttributesBasePtr, false );

                auto attributes = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                auto attributesBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( attributes );
                geoContainer->addEntry( "attributes", attributesBasePtr, false );

                auto primVars = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                auto primVarsBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( primVars );
                geoContainer->addEntry( "primVars", primVarsBasePtr, false );

                AtomsCore::MatrixMetadata mtx;
                geoContainer->addEntry( "matrix", &mtx );

                AtomsCore::MatrixMetadata wmtx;
                geoContainer->addEntry( "worldMatrix", &wmtx );

                AtomsCore::Vector3 emptyVec( 0.0, 0.0, 0.0 );
                AtomsCore::Vector3Metadata pivotMeta( emptyVec );
                geoContainer->addEntry( "pivot", &pivotMeta );

                AtomsCore::Vector3Metadata translationMeta( emptyVec );
                geoContainer->addEntry( "translation", &translationMeta );

                AtomsCore::Vector3 scaleVec( 1.0, 1.0, 1.0 );
                AtomsCore::Vector3Metadata scaleMeta( scaleVec );
                geoContainer->addEntry( "scale", &scaleMeta );

                AtomsCore::Vector3Metadata rotateMeta( emptyVec );
                geoContainer->addEntry( "rotation", &rotateMeta );

                AtomsCore::IntMetadata rotateOrderMeta;
                rotateOrderMeta.set( AtomsCore::Euler::XYZ );
                geoContainer->addEntry( "rotationOrder", &rotateOrderMeta );

                AtomsCore::StringArrayMetadata sets;
                geoContainer->addEntry( "sets", &sets );

                auto basePtr = std::static_pointer_cast<AtomsCore::Metadata>( it->second );
                geoContainer->addEntry( "geo", basePtr, false );
                continue;
            }

            if ( it->second->typeId() != AtomsCore::MapMetadata::staticTypeId() ) {
                continue;
            }

            auto inputGeoMap = std::static_pointer_cast<AtomsCore::MapMetadata>( it->second );

            auto cloth = inputGeoMap->getTypedEntry<AtomsCore::MeshMetadata>( "cloth" );
            if ( cloth ) {
                auto clothBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( cloth );
                inputGeoMap->addEntry( "geo", clothBasePtr, false );
                AtomsCore::BoolMetadata clothFlag( true );
                inputGeoMap->addEntry( "cloth", &clothFlag );
            }

            auto materialAttributes = inputGeoMap->getTypedEntry<AtomsCore::MapMetadata>( "material" );
            if ( !materialAttributes ) {
                materialAttributes = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                auto materialBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( materialAttributes );
                inputGeoMap->addEntry( "material", materialBasePtr, false );
            }

            swapMapGeoData( inputGeoMap, materialAttributes, "color" );
            swapMapGeoData( inputGeoMap, materialAttributes, "colorTexture" );
            swapMapGeoData( inputGeoMap, materialAttributes, "cosinePower" );
            swapMapGeoData( inputGeoMap, materialAttributes, "specularPower" );
            swapMapGeoData( inputGeoMap, materialAttributes, "specularRollOff" );
            swapMapGeoData( inputGeoMap, materialAttributes, "eccentricity" );
            swapMapGeoData( inputGeoMap, materialAttributes, "specularColor" );
            swapMapGeoData( inputGeoMap, materialAttributes, "specularTexture" );

            auto attributes = inputGeoMap->getTypedEntry<AtomsCore::MapMetadata>( "attributes" );
            if ( !attributes ) {
                attributes = AtomsPtr<AtomsCore::MapMetadata>(new AtomsCore::MapMetadata);
                auto attributesBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( attributes );
                inputGeoMap->addEntry( "attributes", attributesBasePtr, false );
            }

            swapMapGeoData( inputGeoMap, attributes, "atoms" );
            swapMapGeoData( inputGeoMap, attributes, "arnold" );
            swapMapGeoData( inputGeoMap, attributes, "vray" );
            swapMapGeoData( inputGeoMap, attributes, "renderman" );

            auto primVars = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "primVars" );
            if ( !primVars ) {
                primVars = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                auto primVarsBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( primVars );
                inputGeoMap->addEntry( "primVars", primVarsBasePtr, false );
            }

            swapPrimVarData( inputGeoMap, primVars, "xgen_Pref" );
            swapPrimVarData( inputGeoMap, primVars, "__Nref" );
            swapPrimVarData( inputGeoMap, primVars, "__WNref" );
            swapPrimVarData( inputGeoMap, primVars, "__Pref" );
            swapPrimVarData( inputGeoMap, primVars, "__WPref" );


            AtomsCore::MatrixMetadata mtx;
            inputGeoMap->addEntry( "matrix", &mtx );

            AtomsCore::MatrixMetadata wmtx;
            inputGeoMap->addEntry( "worldMatrix", &wmtx );

            AtomsCore::Vector3 emptyVec( 0.0, 0.0, 0.0 );
            AtomsCore::Vector3Metadata pivotMeta( emptyVec );
            inputGeoMap->addEntry( "pivot", &pivotMeta );

            AtomsCore::Vector3Metadata translationMeta( emptyVec );
            inputGeoMap->addEntry( "translation", &translationMeta );

            AtomsCore::Vector3 scaleVec( 1.0, 1.0, 1.0 );
            AtomsCore::Vector3Metadata scaleMeta( scaleVec );
            inputGeoMap->addEntry( "scale", &scaleMeta );

            AtomsCore::Vector3Metadata rotateMeta( emptyVec );
            inputGeoMap->addEntry( "rotation", &rotateMeta );

            AtomsCore::IntMetadata rotateOrderMeta;
            rotateOrderMeta.set( AtomsCore::Euler::XYZ );
            inputGeoMap->addEntry( "rotationOrder", &rotateOrderMeta );

            AtomsCore::StringArrayMetadata sets;
            inputGeoMap->addEntry( "sets", &sets );

            auto inputGeoMapBasePtr = std::static_pointer_cast<AtomsCore::Metadata>( inputGeoMap );
            childrenMap->addEntry( it->first, inputGeoMapBasePtr, false );
        }

        return geoMap;
    }

    void loadAgentTypeMesh( Atoms::AgentTypeVariationCPtr agentTypePtr, const std::string& agentTypeName )
    {
        auto geoNames = agentTypePtr->getGeometryNames();
        size_t nbGeos = geoNames.size();

        std::vector<AtomsPtr<AtomsCore::MapMetadata>> atomsMeshes(nbGeos);
        std::vector<Atoms::VariationGeometryCPtr> atomsGeoPtrs(nbGeos, nullptr);

        // Load atoms meshes in parallel
        auto loadGeoMeshesFunc = [&](const tbb::blocked_range<size_t>& range) {
            for (size_t geoId = range.begin(); geoId < range.end(); ++geoId)
            {
                auto geoPtr = agentTypePtr->getGeometryPtr( geoNames[geoId] );
                if ( !geoPtr )
                    continue;

                if ( geoPtr->getGeometryFile().find(".groom") != std::string::npos )
                    continue;

                auto inGeoMap = Atoms::loadMesh( geoPtr->getGeometryFile(), geoPtr->getGeometryFilter() );
                if ( !inGeoMap )
                {
                    throw InvalidArgumentException( "AtomsVariationsReader: Invalid geo: " +
                                                    geoPtr->getGeometryFile() +":" + geoPtr->getGeometryFilter() );
                }

                atomsMeshes[geoId] = inGeoMap;
                atomsGeoPtrs[geoId] = geoPtr;
            }
        };

        tbb::task_group_context taskGroupContext(tbb::task_group_context::isolated);
        tbb::blocked_range<size_t> tbb_range(0, nbGeos, 1);
        tbb::parallel_for(tbb_range, loadGeoMeshesFunc, taskGroupContext);

        // Process loaded meshes
        for ( size_t geoId = 0; geoId != geoNames.size(); ++geoId )
        {
            auto inGeoMap = atomsMeshes[geoId];
            if (!inGeoMap)
                continue;

            auto geoPtr = atomsGeoPtrs[geoId];
            auto* agentTypeRoot = &m_root.children[agentTypeName];
            AtomsPtr<AtomsCore::MapMetadata> atomsGeoMap( new AtomsCore::MapMetadata );
            flatMeshHierarchy(inGeoMap.get(), atomsGeoMap, agentTypeRoot);

            //compute the bouding box
            AtomsCore::Box3f bbox;
            for ( auto meshIt = atomsGeoMap->begin(); meshIt != atomsGeoMap->end(); ++meshIt )
            {
                if ( meshIt->second->typeId() !=  AtomsCore::MapMetadata::staticTypeId() )
                    continue;

                auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>( meshIt->second );
                AtomsPtr<const AtomsCore::MapMetadata> attributesMapPtr =
                        geoMap->getTypedEntry<const AtomsCore::MapMetadata>( "atoms" );

                if ( attributesMapPtr )
                {
                    AtomsPtr<const AtomsCore::MapMetadata> atomsMapPtr =
                            attributesMapPtr->getTypedEntry<const AtomsCore::MapMetadata>(
                            "atoms");
                    if (atomsMapPtr) {
                        // Skip this mesh if it hase the cloth hide tag
                        auto hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>(
                                ATOMS_CLOTH_HIDE_MESH);
                        if (hideMeshPtr && hideMeshPtr->value())
                            continue;

                        // Skip this mesh if it has the preview tag
                        hideMeshPtr = atomsMapPtr->getTypedEntry<const AtomsCore::BoolMetadata>(ATOMS_PREVIEW_MESH);
                        if (hideMeshPtr && hideMeshPtr->value())
                            continue;
                    }
                }


                auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
                if ( !meshMeta )
                {
                    continue;
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

    const AtomsDagNode& root() const
    {
        return m_root;
    }

    const std::map<std::string, std::map<std::string, AtomsPtr<AtomsCore::MapMetadata>>>& meshCache() const
    {
        return m_meshesFileCache;
    }

    void collectAllPathFromSetName(
            const std::string& setName,
            const AtomsDagNode* node,
            std::vector<std::string>& paths,
            const std::string& currentPath ) const
    {
        if ( node->data ) {

            auto sets = node->data->getTypedEntry<const AtomsCore::StringArrayMetadata>( "sets" );
            if ( sets ) {
                auto& setVec = sets->get();
                for ( auto& currentSet: setVec ) {
                    if ( currentSet == setName ) {
                        for ( auto& variation: node->variations ) {
                            paths.push_back(variation + "/" + currentPath);
                        }
                        break;
                    }
                }
            }
        }

        for ( auto it = node->children.cbegin(); it != node->children.cend(); ++it ) {
            if (currentPath.empty() || currentPath == "/")
            {
                collectAllPathFromSetName(setName, &it->second, paths, it->first);
            } else {
                collectAllPathFromSetName(setName, &it->second, paths, currentPath + "/" + it->first);
            }
        }
    }

    bool isDefaultSet( const std::string& setName ) const
    {
        return std::find( m_defaultSets.cbegin(), m_defaultSets.cend(), setName) != m_defaultSets.cend();
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
        accumulator.accumulate( m_totalMemory );
    }

private :

    Atoms::Variations m_variations;

    std::string m_filePath;

    std::map<std::string, std::map<std::string, AtomsPtr<AtomsCore::MapMetadata>>> m_meshesFileCache;

    AtomsPtr<AtomsCore::MapMetadata> m_hierarchy;

    std::vector<std::string> m_defaultSets;

    AtomsDagNode m_root;

    size_t m_totalMemory;
};

size_t AtomsVariationReader::g_firstPlugIndex = 0;

AtomsVariationReader::AtomsVariationReader( const std::string &name )
	:	SceneNode( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsVariationFile" ) );
    addChild( new BoolPlug( "Pref" ) );
    addChild( new BoolPlug( "Nref" ) );
	addChild( new IntPlug( "refreshCount" ) );
	addChild( new ObjectPlug( "__engine", Plug::Out, NullObject::defaultNullObject() ) );
}

StringPlug* AtomsVariationReader::atomsVariationFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsVariationReader::atomsVariationFilePlug() const
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
	if( input == atomsVariationFilePlug() || input == refreshCountPlug() ||
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

Gaffer::ValuePlug::CachePolicy AtomsVariationReader::computeCachePolicy( const Gaffer::ValuePlug *output ) const
{
	if( output == enginePlug() )
	{
		// Request blocking compute for the engine, to avoid concurrent threads
        // loading the same engine redundantly.
		return ValuePlug::CachePolicy::TaskCollaboration;
	}

	return SceneNode::computeCachePolicy( output );
}

void AtomsVariationReader::hashBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashBound( path, context, parent, h );

	h.append( atomsVariationFilePlug()->getValue() );
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
                    Imath::Box3f outBox;
                    convertFromAtoms( outBox, bbox->get() );
                    return outBox;
                }
                else
                {
                    throw InvalidArgumentException( "AtomsVariationsReader: No bound found for " +
                    geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
                }
            }
            else
            {
                throw InvalidArgumentException( "AtomsVariationsReader: No valid geo found " +
                geoData->getGeometryFile() + ":" + geoData->getGeometryFilter() );
            }
        }
    }

    return unionOfTransformedChildBounds( path, parent );
}

void AtomsVariationReader::hashTransform( const ScenePath &path, const Gaffer::Context *context,
        const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashTransform( path, context, parent, h );
	h.append( atomsVariationFilePlug()->getValue() );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

AtomsPtr<const AtomsCore::MapMetadata> AtomsVariationReader::getNodeDataFromHierarchy( const ScenePath &path ) const
{
    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if ( !engineData )
    {
        return nullptr;
    }

    auto& root = engineData->root();
    auto aTypeIt = root.children.find( path[0] );
    if ( aTypeIt == root.children.end() )
        return nullptr;

    const EngineData::AtomsDagNode* currentNode = &aTypeIt->second;
    for ( size_t i = 2; i < path.size(); ++i )
    {
        auto meshIt = currentNode->children.find( path[i] );
        if ( meshIt == currentNode->children.end())
        {
            return nullptr;
        }
        currentNode = &meshIt->second;
    }

    if ( currentNode )
    {
        return currentNode->data;
    }

    return nullptr;
}

Imath::M44f AtomsVariationReader::computeTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
    // The Atoms mesh are stored in world space without transformation
    if( path.size() < 3 )
    {
        return parent->transformPlug()->defaultValue();
    }

    auto data = getNodeDataFromHierarchy( path );
    if ( data )
    {
        auto matrixPtr = data->getTypedEntry<const AtomsCore::MatrixMetadata>( "matrix" );
        if ( matrixPtr )
        {
            Imath::M44f outMatrix;
            convertFromAtoms( outMatrix, matrixPtr->get() );
            return outMatrix;
        }
    }

	return {};
}

void AtomsVariationReader::hashAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashAttributes( path, context, parent, h );
	h.append( atomsVariationFilePlug()->getValue() );
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
        // It a transform so set get eh attributes from the dag node
        auto dagNodeMap = getNodeDataFromHierarchy( path );
        if ( dagNodeMap )
        {
            auto attributesMap = dagNodeMap->getTypedEntry<AtomsCore::MapMetadata>( "attributes" );
            if ( attributesMap )
            {
                CompoundObjectPtr result = new CompoundObject;
                for (auto attrIt = attributesMap->cbegin(); attrIt != attributesMap->cend(); ++attrIt)
                {
                    if ( attrIt->first == "arnold" || attrIt->first == "vray" || attrIt->first == "renderman" )
                        continue;
                    auto atomsAttributeMap = std::dynamic_pointer_cast<const AtomsCore::MapMetadata>( attrIt->second );
                    if ( atomsAttributeMap )
                    {
                        for ( auto meshAttrIt = atomsAttributeMap->cbegin();
                             meshAttrIt != atomsAttributeMap->cend(); ++meshAttrIt )
                        {
                            auto data = translator.translate( meshAttrIt->second );
                            if (data)
                            {
                               result->members()["user:" + attrIt->first + ":" + meshAttrIt->first] = data;
                            }
                        }
                    }
                }
                return result;
            }
        }
        else {
            return parent->attributesPlug()->defaultValue();
        }

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
        auto jointWeightsAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointWeights" );
        auto jointIndicesAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointIndices" );
        if ( jointWeightsAttr && jointIndicesAttr && jointWeightsAttr->size() == jointIndicesAttr->size() )
        {
            for( size_t wId = 0; wId < jointWeightsAttr->size(); ++wId )
            {
                auto jointIndices = jointIndicesAttr->getTypedElement<AtomsCore::IntArrayMetadata>( wId );
                auto jointWeights = jointWeightsAttr->getTypedElement<AtomsCore::DoubleArrayMetadata>( wId );

                if (!jointIndices || !jointWeights)
                    continue;

                indexCount.push_back( jointIndices->get().size() );
                indices.insert( indices.end(), jointIndices->get().begin(), jointIndices->get().end() );
                weights.insert( weights.end(), jointWeights->get().begin(), jointWeights->get().end() );
            }
        }


        // Convert the atoms metadata to gaffer attribute
        auto attributesMap = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "attributes" );
        if ( attributesMap ) {
            for (auto attrIt = attributesMap->cbegin(); attrIt != attributesMap->cend(); ++attrIt)
            {
                if ( attrIt->first == "arnold" || attrIt->first == "vray" || attrIt->first == "renderman" )
                    continue;
                auto atomsAttributeMap = std::dynamic_pointer_cast<const AtomsCore::MapMetadata>( attrIt->second );
                if ( atomsAttributeMap ) {
                    for ( auto meshAttrIt = atomsAttributeMap->cbegin();
                         meshAttrIt != atomsAttributeMap->cend(); ++meshAttrIt ) {
                        auto data = translator.translate( meshAttrIt->second );
                        if ( data ) {
                            result->members()["user:" + attrIt->first + ":" + meshAttrIt->first] = data;
                        }
                    }
                }
            }
            // Convert the arnold metadata to gaffer attribute
            auto atomsAttributeMap = attributesMap->getTypedEntry<AtomsCore::MapMetadata>( "arnold" );
            if ( !atomsAttributeMap ) {
                continue;
            }

            for ( auto meshAttrIt = atomsAttributeMap->cbegin(); meshAttrIt != atomsAttributeMap->cend(); ++meshAttrIt ) {
                auto data = translator.translate( meshAttrIt->second );
                if (!data) {
                    continue;
                }

                // Convert the arnold visible_in_* attribute to ai:visibility:*
                std::string aiParameter = meshAttrIt->first;
                auto visIndex = meshAttrIt->first.find( "visible_in_" );
                bool handleParam = false;
                if ( visIndex != std::string::npos ) {
                    aiParameter = "visibility:" + meshAttrIt->first.substr( visIndex + 11 );
                    handleParam = true;
                }

                // Convert the arnold subdiv* attribute to ai:polymesh:*
                visIndex = meshAttrIt->first.find( "subdiv" );
                if ( visIndex != std::string::npos ) {
                    aiParameter = "polymesh:" + meshAttrIt->first;
                    handleParam = true;
                }

                // Sett only polymesh arnold attributes
                if ( handleParam ||
                    ( meshAttrIt->first == "sidedness" ||
                     meshAttrIt->first == "receive_shadows" ||
                     meshAttrIt->first == "self_shadows" ||
                     meshAttrIt->first == "invert_normals" ||
                     meshAttrIt->first == "opaque" ||
                     meshAttrIt->first == "matte" ||
                     meshAttrIt->first == "smoothing"
                    )
                        ) {
                    switch ( data->typeId() ) {
                        case IECore::TypeId::DoubleDataTypeId: {
                            FloatDataPtr fData = new FloatData;
                            fData->writable() = runTimeCast<const DoubleData>( data )->readable();
                            result->members()["ai:" + aiParameter] = fData;
                            break;
                        }
                        case IECore::TypeId::V3dDataTypeId: {
                            V3fDataPtr fData = new V3fData;
                            fData->writable() = Imath::V3f( runTimeCast<const V3dData>( data )->readable() );
                            result->members()["ai:" + aiParameter] = fData;
                            break;
                        }
                        default: {
                            result->members()["ai:" + aiParameter] = data;
                            break;
                        }
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

void AtomsVariationReader::hashObject( const ScenePath &path, const Gaffer::Context *context,
        const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashObject( path, context, parent, h );
	h.append( atomsVariationFilePlug()->getValue() );
	refreshCountPlug()->hash( h );
    generatePrefPlug()->hash( h );
    generateNrefPlug()->hash( h );
	h.append( &path.front(), path.size() );
}

IECore::ConstObjectPtr AtomsVariationReader::computeObject( const ScenePath &path,
        const Gaffer::Context *context, const ScenePlug *parent ) const
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

    // A single .geos file could contain multiple mesh, se we need to merge those mesh together
    std::vector<AtomsPtr<AtomsCore::MapMetadata>> inGeosMap;
    for ( auto meshIt = atomsGeo->begin(); meshIt!= atomsGeo->end(); ++meshIt )
    {
        if ( meshIt->first == "boundingBox" )
            continue;

        auto geoMap = std::static_pointer_cast<AtomsCore::MapMetadata>( meshIt->second );
        if ( !geoMap )
            continue;

        inGeosMap.push_back( geoMap );
    }

    if ( inGeosMap.size() == 0 )
        return parent->objectPlug()->defaultValue();

    if (inGeosMap.size() == 1)
    {
        return convertAtomsMesh( inGeosMap[0], generatePrefPlug()->getValue(),  generateNrefPlug()->getValue() );
    }

    inGeosMap.push_back(inGeosMap.back());
    mergeAtomsMesh( outGeoMap, inGeosMap );
    return convertAtomsMesh( outGeoMap, generatePrefPlug()->getValue(),  generateNrefPlug()->getValue() );
}

void AtomsVariationReader::hashChildNames( const ScenePath &path, const Gaffer::Context *context,
        const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashChildNames( path, context, parent, h );
	h.append( atomsVariationFilePlug()->getValue() );
	refreshCountPlug()->hash( h );
	h.append( &path.front(), path.size() );
}

IECore::ConstInternedStringVectorDataPtr AtomsVariationReader::computeChildNames( const ScenePath &path,
        const Gaffer::Context *context, const ScenePlug *parent ) const
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

	h.append( atomsVariationFilePlug()->getValue() );
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

    const EngineData::AtomsDagNode* rootNode = &engineData->root();
    std::list<const EngineData::AtomsDagNode*> nodes;
    nodes.push_back(rootNode);
    while ( nodes.size() > 0 )
    {
        const EngineData::AtomsDagNode* node = nodes.front();
        nodes.pop_front();
        if ( node->data )
        {
            auto setsData = node->data->getTypedEntry<const AtomsCore::StringArrayMetadata>("sets");
            if ( setsData )
            {
                for (const auto& setName: setsData->get())
                {
                    if ( std::find(result.begin(), result.end(), setName) == result.end() )
                        result.emplace_back(setName);
                }
            }
        }

        for ( auto it = node->children.cbegin(); it != node->children.cend(); ++it ) {
            nodes.push_back( &it->second );
        }
    }
	return resultData;
}

void AtomsVariationReader::hashSet( const IECore::InternedString &setName, const Gaffer::Context *context,
        const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashSet( setName, context, parent, h );

	h.append( atomsVariationFilePlug()->getValue() );
	refreshCountPlug()->hash( h );
	h.append( setName );
}

IECore::ConstPathMatcherDataPtr AtomsVariationReader::computeSet( const IECore::InternedString &setName,
        const Gaffer::Context *context, const ScenePlug *parent ) const
{
    PathMatcherDataPtr resultData = new PathMatcherData;
    ConstEngineDataPtr engineData = static_pointer_cast<const EngineData>( enginePlug()->getValue() );
    if (!engineData) {
        return resultData;
    }
    auto &result = resultData->writable();


    if ( !engineData->isDefaultSet( setName ) )  {
        const EngineData::AtomsDagNode *rootNode = &engineData->root();
        for ( auto it = rootNode->children.cbegin(); it != rootNode->children.cend(); ++it ) {
            const std::string &agentTypeName = it->first;
            std::vector<std::string> paths;
            engineData->collectAllPathFromSetName( setName.string(), &it->second, paths, "/" );

            for ( auto &path: paths )
                result.addPath( "/" + agentTypeName + "/" + path );
        }
        return resultData;
    }

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
		h.append( atomsVariationFilePlug()->getValue() );
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

		static_cast<ObjectPlug *>( output )->setValue(new EngineData(atomsVariationFilePlug()->getValue()));
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
                uvData->uvs.push_back( AtomsCore::Vector2f( 0.0, 0.0 ) );
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
                uvSet.uvs.push_back( AtomsCore::Vector2f( 0.0, 0.0 ) );
                uvSet.uvIndices.push_back( uvSet.uvs.size() );
            }
        }
    }
}

void AtomsVariationReader::mergeBlendShapes(
        AtomsPtr<AtomsCore::MapMetadata>& geoMap,
        AtomsPtr<AtomsCore::ArrayMetadata>& outBlendMeta,
        AtomsUtils::Mesh &inMesh,
        size_t pointSize,
        size_t normalSize ) const
{
    auto outMeshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
    AtomsUtils::Mesh &mesh = outMeshMeta->get();

    // Merge the blend shapes
    auto blendShapes = geoMap->getTypedEntry<const AtomsCore::ArrayMetadata>("blendShapes");
    if (!(blendShapes && blendShapes->size() > 0)) {
        return;
    }

    // Add missing blend shapes
    for ( unsigned int blendId = outBlendMeta->size(); blendId < blendShapes->size(); blendId++ ) {
        AtomsCore::MapMetadata blendData;
        AtomsCore::Vector3ArrayMetadata emptyVec;
        blendData.addEntry("P", &emptyVec);
        blendData.addEntry("N", &emptyVec);
        outBlendMeta->push_back(&blendData);
    }

    for ( unsigned int blendId = 0; blendId < blendShapes->size(); blendId++ ) {
        auto blendMap = blendShapes->getTypedElement<AtomsCore::MapMetadata>( blendId );
        if (!blendMap)
            continue;

        auto blendPoints = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "P" );
        auto &blendP = blendPoints->get();

        auto blendNormals = blendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>( "N" );
        auto &blendN = blendNormals->get();


        auto outBlendMap = outBlendMeta->getTypedElement<AtomsCore::MapMetadata>(blendId);
        if (!outBlendMap)
            continue;

        auto outBlendPoints = outBlendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>("P");
        auto &outP = outBlendPoints->get();
        if (outP.size() < pointSize) {
            size_t currentSize = outP.size();
            outP.resize(pointSize);
            for (size_t pbId = currentSize; pbId < pointSize; ++pbId) {
                outP[pbId] = mesh.points()[pbId];
            }
        }

        outP.insert(outP.begin(), blendP.begin(), blendP.end());

        auto outBlendNormals = outBlendMap->getTypedEntry<AtomsCore::Vector3ArrayMetadata>("N");
        auto &outN = outBlendNormals->get();
        if (outN.size() < normalSize) {
            size_t currentSize = outN.size();
            outN.resize(normalSize);
            for (size_t pbId = currentSize; pbId < normalSize; ++pbId) {
                outN[pbId] = mesh.normals()[pbId];
            }
        }

        if (blendN.size() == inMesh.indices().size()) {
            outN.insert(outN.begin(), blendN.begin(), blendN.end());
        } else {
            // convert normals from vertex to face varying
            outN.resize(outN.size() + inMesh.indices().size());
            for (size_t vId = 0; vId < std::min(inMesh.indices().size(), blendN.size()); ++vId) {
                outN[vId] = blendN[inMesh.indices()[vId]];
            }
        }
    }
}

template<typename T>
void mergeAtomsPrimVariableData(
        AtomsPtr<AtomsCore::MapMetadata>& outPrimVar,
        const std::string& interpolationType,
        AtomsUtils::Mesh& mesh,
        AtomsPtr<AtomsCore::TypedArrayMetadata<T>>& inVectorData)
{
    auto vectorData = outPrimVar->getTypedEntry<AtomsCore::TypedArrayMetadata<T>>( "data" );
    if ( !vectorData )
    {
        vectorData = AtomsPtr<AtomsCore::TypedArrayMetadata<T>>( new AtomsCore::TypedArrayMetadata<T> );
        auto vectorDataPtr = std::static_pointer_cast<AtomsCore::Metadata>( vectorData );
        outPrimVar->addEntry( "data", vectorDataPtr, false );
    }


    size_t startSize = 0;
    if (interpolationType == "uniform") {
        startSize = mesh.numberOfFaces() - inVectorData->get().size();
    } else if (interpolationType == "vertex") {
        startSize = mesh.numberOfVertices() - inVectorData->get().size();
    } else if (interpolationType == "varying") {
        startSize = mesh.numberOfVertices() - inVectorData->get().size();
    } else if (interpolationType == "faceVarying") {
        startSize = mesh.indices().size() - inVectorData->get().size();
    }

    if ( vectorData->get().size() < startSize)
    {
        vectorData->get().resize(startSize);
    }
    vectorData->get().insert(
            vectorData->get().end(),
            inVectorData->get().begin(),
            inVectorData->get().end()
    );
}

void AtomsVariationReader::mergeAtomsMesh(
        AtomsPtr<AtomsCore::MapMetadata>& outGeoMap,
        std::vector<AtomsPtr<AtomsCore::MapMetadata>>& geos
) const
{
    auto outMeshMeta = outGeoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
    if ( !outMeshMeta ) {
        outMeshMeta = AtomsPtr<AtomsCore::MeshMetadata>( new AtomsCore::MeshMetadata );
        auto outMeshMetaPtr = std::static_pointer_cast<AtomsCore::Metadata>( outMeshMeta );
        outGeoMap->addEntry( "geo", outMeshMetaPtr, false);
    }

    auto outBlendMeta = outGeoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "blendShapes" );
    if ( !outBlendMeta ) {
        outBlendMeta = AtomsPtr<AtomsCore::ArrayMetadata>( new AtomsCore::ArrayMetadata );
        auto outBlendMetaPtr = std::static_pointer_cast<AtomsCore::Metadata>( outBlendMeta );
        outGeoMap->addEntry( "blendShapes", outBlendMetaPtr, false);
    }

    auto outAttributes = outGeoMap->getTypedEntry<AtomsCore::MapMetadata>( "attributes" );
    if ( !outAttributes ) {
        outAttributes = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
        auto outAttributesPtr = std::static_pointer_cast<AtomsCore::Metadata>( outAttributes );
        outGeoMap->addEntry( "attributes", outAttributesPtr, false);
    }

    auto outPrimVars = outGeoMap->getTypedEntry<AtomsCore::MapMetadata>( "primVars" );
    if ( !outPrimVars ) {
        outPrimVars = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
        auto outPrimVarsPtr = std::static_pointer_cast<AtomsCore::Metadata>( outPrimVars );
        outGeoMap->addEntry( "primVars", outPrimVarsPtr, false);
    }

    auto outJointWeightsAttr = outGeoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointWeights" );
    if ( !outJointWeightsAttr ) {
        outJointWeightsAttr = AtomsPtr<AtomsCore::ArrayMetadata>( new AtomsCore::ArrayMetadata );
        auto outJointWeightsAttrPtr = std::static_pointer_cast<AtomsCore::Metadata>( outJointWeightsAttr );
        outGeoMap->addEntry( "jointWeights", outJointWeightsAttrPtr, false);
    }

    auto outJointIndicesAttr = outGeoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointIndices" );
    if ( !outJointIndicesAttr ) {
        outJointIndicesAttr = AtomsPtr<AtomsCore::ArrayMetadata>( new AtomsCore::ArrayMetadata );
        auto outJointIndicesAttrPtr = std::static_pointer_cast<AtomsCore::Metadata>( outJointIndicesAttr );
        outGeoMap->addEntry( "jointIndices", outJointIndicesAttrPtr, false);
    }


    for (auto geoId = 0; geoId < geos.size(); ++geoId) {
        auto geoMap = geos[geoId];
        auto meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "geo" );
        if (!meshMeta) {
            meshMeta = geoMap->getTypedEntry<AtomsCore::MeshMetadata>( "cloth" );
            if (!meshMeta) {
                continue;
            }
        }


        AtomsUtils::Mesh &mesh = outMeshMeta->get();
        AtomsUtils::Mesh &inMesh = meshMeta->get();
        size_t currentPointsSize = mesh.points().size();
        size_t currentNormalsSize = mesh.normals().size();
        size_t currentIndicesSize = mesh.indices().size();

        if (mesh.uvs().size() < currentIndicesSize) {
            mesh.uvs().resize(currentIndicesSize);
        }

        mesh.points().insert(mesh.points().end(), inMesh.points().begin(), inMesh.points().end());
        mesh.normals().insert(mesh.normals().end(), inMesh.normals().begin(), inMesh.normals().end());
        mesh.vertexCount().insert(mesh.vertexCount().end(), inMesh.vertexCount().begin(), inMesh.vertexCount().end());
        mesh.jointIndices().insert(mesh.jointIndices().end(), inMesh.jointIndices().begin(),
                                   inMesh.jointIndices().end());
        mesh.jointWeights().insert(mesh.jointWeights().end(), inMesh.jointWeights().begin(),
                                   inMesh.jointWeights().end());

        auto &inIndices = inMesh.indices();
        auto &outIndices = mesh.indices();
        outIndices.resize(currentIndicesSize + inIndices.size());
        for (size_t id = 0; id < inIndices.size(); ++id) {
            outIndices[currentIndicesSize + id] = inIndices[id] + currentPointsSize;
        }

        auto &outUV = mesh.uvs();
        outUV.insert(outUV.end(), inMesh.uvs().begin(), inMesh.uvs().end());
        if (outUV.size() < mesh.normals().size()) {
            outUV.resize(mesh.normals().size());
        }

        // Merge the uv sets
        mergeUvSets(mesh, inMesh, currentNormalsSize);
        mergeBlendShapes(
                geoMap,
                outBlendMeta,
                inMesh,
                currentPointsSize,
                currentNormalsSize );


        auto jointWeightsAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointWeights" );
        if ( jointWeightsAttr )
        {
            for ( auto jwIt = 0; jwIt < jointWeightsAttr->size(); ++jwIt )
                outJointWeightsAttr->push_back( jointWeightsAttr->operator[]( jwIt ) );
        }

        auto jointIndicesAttr = geoMap->getTypedEntry<AtomsCore::ArrayMetadata>( "jointIndices" );
        if ( jointIndicesAttr )
        {
            for ( auto jwIt = 0; jwIt < jointIndicesAttr->size(); ++jwIt )
                outJointIndicesAttr->push_back( jointIndicesAttr->operator[]( jwIt ) );
        }

        // merge attributes
        auto inAttributes = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "attributes" );
        {
            for ( auto attrContainerIt = inAttributes->begin(); attrContainerIt != inAttributes->cend(); ++attrContainerIt )
            {
                auto outAttrContainer = outAttributes->getTypedEntry<AtomsCore::MapMetadata>( attrContainerIt->first );
                if ( !outAttrContainer )
                {
                    outAttrContainer = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                    auto outAttrContainerPtr = std::static_pointer_cast<AtomsCore::Metadata>( outAttrContainer );
                    outAttributes->addEntry( attrContainerIt->first, outAttrContainerPtr, false );
                }

                auto inAttrContainer = std::static_pointer_cast<AtomsCore::MapMetadata>( attrContainerIt->second );

                for ( auto attrIt = inAttrContainer->begin(); attrIt != inAttrContainer->cend(); ++attrIt )
                {
                    outAttrContainer->addEntry( attrIt->first, attrIt->second );
                }
            }
        }

        // merge primvars
        auto inPrimVars = geoMap->getTypedEntry<AtomsCore::MapMetadata>( "primVars" );
        {
            for ( auto primVarIt = inPrimVars->begin(); primVarIt != inPrimVars->cend(); ++primVarIt )
            {
                if ( primVarIt->second->typeId() != AtomsCore::MapMetadata::staticTypeId() )
                    continue;

                auto primVarMap = std::static_pointer_cast<AtomsCore::MapMetadata>( primVarIt->second );
                auto interpolationPtr = primVarMap->getTypedEntry<AtomsCore::StringMetadata>( "interpolation" );
                if ( !interpolationPtr )
                    continue;

                if (interpolationPtr->get() == "constant") {
                    outPrimVars->addEntry(primVarIt->first, primVarIt->second);
                    continue;
                }

                auto outPrimVar = outPrimVars->getTypedEntry<AtomsCore::MapMetadata>( primVarIt->first );
                if ( !outPrimVar )
                {
                    outPrimVar = AtomsPtr<AtomsCore::MapMetadata>( new AtomsCore::MapMetadata );
                    outPrimVar->addEntry( "interpolation", interpolationPtr);
                    auto outPrimVarPtr = std::static_pointer_cast<AtomsCore::Metadata>( outPrimVar );
                    outPrimVars->addEntry(primVarIt->first, outPrimVarPtr, false);

                }


                switch ( primVarIt->second->typeId() ) {
                    case kMetadataIntArrayTypeId: {
                        auto inVectorData = std::static_pointer_cast<AtomsCore::IntArrayMetadata>( primVarIt->second );
                        mergeAtomsPrimVariableData( outPrimVar, interpolationPtr->get(), mesh, inVectorData );
                        break;
                    }
                    case kMetadataDoubleArrayTypeId: {
                        auto inVectorData = std::static_pointer_cast<AtomsCore::DoubleArrayMetadata>( primVarIt->second );
                        mergeAtomsPrimVariableData( outPrimVar, interpolationPtr->get(), mesh, inVectorData );
                        break;
                    }
                    case kMetadataVector3ArrayTypeId: {
                        auto inVectorData = std::static_pointer_cast<AtomsCore::Vector3ArrayMetadata>( primVarIt->second );
                        mergeAtomsPrimVariableData( outPrimVar, interpolationPtr->get(), mesh, inVectorData );
                        break;
                    }
                    case kMetadataBoolArrayTypeId: {
                        auto inVectorData = std::static_pointer_cast<AtomsCore::BoolArrayMetadata>( primVarIt->second );
                        mergeAtomsPrimVariableData( outPrimVar, interpolationPtr->get(), mesh, inVectorData );
                        break;
                    }
                }
            }
        }
    }
}
