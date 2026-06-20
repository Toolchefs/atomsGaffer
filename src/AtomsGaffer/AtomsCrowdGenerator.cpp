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

#include <AtomsUtils/Logger.h>
#include "AtomsGaffer/AtomsCrowdGenerator.h"

#include "Atoms/GlobalNames.h"

#include "IECoreScene/PointsPrimitive.h"
#include "IECoreScene/MeshPrimitive.h"

#include "IECore/NullObject.h"
#include "IECore/BlindDataHolder.h"

#include "ImathEuler.h"

IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsCrowdGenerator );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

size_t AtomsCrowdGenerator::g_firstPlugIndex = 0;

AtomsCrowdGenerator::AtomsCrowdGenerator( const std::string &name )
	:	BranchCreator( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "name", Plug::In, "agents" ) );
	addChild( new ScenePlug( "variations" ) );
	addChild( new BoolPlug( "useInstances" ) );
	addChild( new FloatPlug( "boundingBoxPadding" ) );

    addChild( new ScenePlug( "clothCache" ) );

	addChild( new AtomicCompoundDataPlug( "__agentChildNames", Plug::Out, new CompoundData ) );
}

Gaffer::StringPlug *AtomsCrowdGenerator::namePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *AtomsCrowdGenerator::namePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

ScenePlug *AtomsCrowdGenerator::variationsPlug()
{
	return getChild<ScenePlug>( g_firstPlugIndex + 1 );
}

const ScenePlug *AtomsCrowdGenerator::variationsPlug() const
{
	return getChild<ScenePlug>( g_firstPlugIndex + 1 );
}

Gaffer::BoolPlug *AtomsCrowdGenerator::useInstancesPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::BoolPlug *AtomsCrowdGenerator::useInstancesPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

Gaffer::FloatPlug *AtomsCrowdGenerator::boundingBoxPaddingPlug()
{
	return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::FloatPlug *AtomsCrowdGenerator::boundingBoxPaddingPlug() const
{
	return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

GafferScene::ScenePlug *AtomsCrowdGenerator::clothCachePlug()
{
    return getChild<ScenePlug>( g_firstPlugIndex + 4 );
}

const GafferScene::ScenePlug *AtomsCrowdGenerator::clothCachePlug() const
{
    return getChild<ScenePlug>( g_firstPlugIndex + 4 );
}

Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug()
{
    return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 5 );
}

const Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug() const
{
    return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 5 );
}

void AtomsCrowdGenerator::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	BranchCreator::affects( input, outputs );

	if( input == inPlug()->objectPlug() )
	{
		outputs.push_back( agentChildNamesPlug() );
	}
}

void AtomsCrowdGenerator::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, MurmurHash &h ) const
{
	BranchCreator::hash( output, context, h );

	if( output == agentChildNamesPlug() )
	{
		inPlug()->objectPlug()->hash( h );
	}
}

void AtomsCrowdGenerator::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
	// The agentChildNamesPlug is evaluated in a context in which
	// scene:path holds the parent path for a branch.
	if( output == agentChildNamesPlug() )
	{
		// Here we compute and cache the child names for all of
		// the /agents/<agentType>/<variation> locations at once. We
		// could instead compute them one at a time in
		// computeBranchChildNames() but that would require N
		// passes over the input points, where N is the number
		// of instances.

		ConstPointsPrimitivePtr crowd = runTimeCast<const PointsPrimitive>( inPlug()->objectPlug()->getValue() );
		if( !crowd )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive Object." );
		}

		// Get the agentId variable that contains the agent cache id
        const auto agentId = crowd->variables.find( "atoms:agentId" );
        if( agentId == crowd->variables.end() )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
        }

        auto agentIdData = runTimeCast<const IntVectorData>( agentId->second.data );
        if ( !agentIdData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
        }
        const std::vector<int>& agentIdVec = agentIdData->readable();


        const std::vector<std::string>* agentTypeVec = nullptr;
        const std::vector<std::string>* agentVariationVec = nullptr;
        const std::vector<std::string>* agentLodVec = nullptr;
        std::string agentTypeDefault;
        std::string variationDefault;
        std::string lodDefault;

        // Extract the agent type from the prim var
		const auto agentType = crowd->variables.find( "atoms:agentType" );
		if( agentType == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"atoms:agentType\" vertex variable" );
		}

        if ( agentType->second.interpolation == PrimitiveVariable::Vertex )
        {
            auto agentTypesData = runTimeCast<const StringVectorData>( agentType->second.data );
            if ( agentTypesData )
                agentTypeVec = &agentTypesData->readable();
        }
        else if ( agentType->second.interpolation == PrimitiveVariable::Constant )
        {
            auto agentTypesData = runTimeCast<const StringData>( agentType->second.data );
            if ( agentTypesData )
                agentTypeDefault = agentTypesData->readable();
        }

        // Extract the agent variation from the prim var
		const auto agentVariation = crowd->variables.find( "atoms:variation" );
		if( agentVariation == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"atoms:variation\" vertex variable" );
		}

        if ( agentVariation->second.interpolation == PrimitiveVariable::Vertex )
        {
            auto agentVariationData = runTimeCast<const StringVectorData>( agentVariation->second.data );
            if ( agentVariationData )
                agentVariationVec = &agentVariationData->readable();
        }
        else if ( agentVariation->second.interpolation == PrimitiveVariable::Constant )
        {
            auto agentVariationData = runTimeCast<const StringData>(agentVariation->second.data);
            if ( agentVariationData )
                variationDefault = agentVariationData->readable();
        }

        // Extract the agent lod from the prim var
		const auto agentLod = crowd->variables.find( "atoms:lod" );
		if( agentLod == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"atoms:lod\" vertex variable" );
		}

		if ( agentLod->second.interpolation == PrimitiveVariable::Vertex )
		{
            auto agentLodData = runTimeCast<const StringVectorData>( agentLod->second.data );
            if ( agentLodData )
                agentLodVec = &agentLodData->readable();
        }
		else if ( agentLod->second.interpolation == PrimitiveVariable::Constant )
		{
            auto agentLodData = runTimeCast<const StringData>( agentLod->second.data );
            if ( agentLodData )
                lodDefault = agentLodData->readable();
        }


		// Agent map continig per every agent type the list of variation used
		// and for every variation the list of agent id
		std::map<std::string, std::map<std::string, std::vector<int>>> agentVariationMap;

        for( size_t agId = 0; agId < agentIdVec.size(); ++agId )
        {
            std::string agentTypeName = agentTypeDefault;
            if (agentTypeVec)
                agentTypeName = ( *agentTypeVec )[agId];

            std::string variationName = variationDefault;
            if (agentVariationVec)
                variationName = ( *agentVariationVec )[agId];

            if ( variationName.empty() )
                continue;

            std::string lodName = lodDefault;
            if ( agentLodVec )
                lodName = ( *agentLodVec )[agId];

        	if ( !lodName.empty())
        	{
				variationName = variationName + ':' + lodName;
			}

			agentVariationMap[agentTypeName][variationName].push_back( agentIdVec[agId] );
        }

        // Convert the variaion map in compound data
        CompoundDataPtr result = new CompoundData;
        auto& resultData = result->writable();
        for( auto typeIt = agentVariationMap.begin(); typeIt != agentVariationMap.end(); ++typeIt )
        {
            CompoundDataPtr variationResult = new CompoundData;
            auto& variationResultData = variationResult->writable();
            for( auto varIt = typeIt->second.begin(); varIt != typeIt->second.end(); ++varIt )
            {
                InternedStringVectorDataPtr idsData = new InternedStringVectorData;
                auto& idsStrVec = idsData->writable();
                idsStrVec.reserve( varIt->second.size() );
                for ( const auto agId: varIt->second )
                {
                    idsStrVec.emplace_back( std::to_string( agId ) );
                }

                variationResultData[varIt->first] = idsData;
            }

            resultData[typeIt->first] = variationResult;
        }

		static_cast<AtomicCompoundDataPlug *>( output )->setValue( result );
		return;
	}

	BranchCreator::compute( output, context );
}

bool AtomsCrowdGenerator::affectsBranchBound( const Gaffer::Plug *input ) const
{
	return ( input == inPlug()->attributesPlug() ||
			 input == namePlug() ||
			 input == variationsPlug()->transformPlug() ||
			 input == boundingBoxPaddingPlug() ||
			 input == clothCachePlug()->objectPlug() );
}

void AtomsCrowdGenerator::hashBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents"
		ScenePath path = parentPath;
		path.insert( path.end(), branchPath.begin(), branchPath.end() );
		if( branchPath.empty() )
		{
			path.push_back( namePlug()->getValue() );
		}
		h = hashOfTransformedChildBounds( path, outPlug() );
	}

	else if( branchPath.size() >= 4 )
	{
		// "/agents/<agentType>/<variation>/<id>"
		BranchCreator::hashBranchBound( parentPath, branchPath, context, h );

		inPlug()->attributesPlug()->hash( h );
        boundingBoxPaddingPlug()->hash( h );
		clothCachePlug()->objectPlug()->hash( h );
		h.append( branchPath.back() );
		{
			AgentScope scope( context, branchPath );
			variationsPlug()->transformPlug()->hash( h );
		}
	}
}

Imath::Box3f AtomsCrowdGenerator::computeBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents"
		ScenePath path = parentPath;
		path.insert( path.end(), branchPath.begin(), branchPath.end() );
		if( branchPath.empty() )
		{
			path.push_back( namePlug()->getValue() );
		}
		return unionOfTransformedChildBounds( path, outPlug() );
	}
	else if ( branchPath.size() >= 4 )
    {
        // "/agents/<agentType>/<variation>/<id>/..."

        // If there is any cloth extract the bounding box
        Imath::Box3d agentClothBBox;
		ConstCompoundObjectPtr crowd;
		{
			ScenePlug::PathScope scope( context, &parentPath );
			crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
            agentClothBBox = agentClothBoudingBox( parentPath, branchPath );
		}


        // Extract the bound from the agent bound stored inside the atoms cache
        // This bound is computed from the agent joints and not from the skinned mesh,
        // so it's not 100% right
        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        // The atoms:agents contain the atoms cache, so extract the bounding box from it
        auto atomsData = crowd->member<const BlindDataHolder>( "atoms:agents" );
        if( !atomsData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator :  computeBranchBound : No agents data found." );
        }

        auto agentsData = atomsData->blindData();
        if( !agentsData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchBound : No agents data found." );
        }

        // Extract the current agent
        auto agentData = agentsData->member<const CompoundData>( branchPath[3].string() );
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchBound : No agent found." );
        }

        Imath::M44d transformMtx;
        Imath::M44d transformInvMtx;
        ScenePath transfromPath;
        if ( branchPath.size() > 2 ) {
            transfromPath.push_back( branchPath[1] );
            transfromPath.push_back( branchPath[2] );
            for ( int ii = 4; ii < branchPath.size(); ++ii ) {
                transfromPath.push_back( branchPath[ii] );
            }
            transformMtx = Imath::M44d( variationsPlug()->fullTransform( transfromPath ) );
            transformInvMtx = transformMtx.inverse();
        }

        // Extract the agent root matrix
        Imath::M44d rootInvMatrix;
        Imath::M44d rootMatrix;
        auto poseData = agentData->member<const M44dData>( "rootMatrix" );
        if ( poseData ) {
            rootMatrix = poseData->readable();
            rootInvMatrix = rootMatrix.inverse();
        }


        auto boxData = agentData->member<const Box3dData>( "boundingBox" );
        Imath::Box3f result;
        if ( boxData )
        {
            float padding  = boundingBoxPaddingPlug()->getValue();
            Imath::Box3d agentBox;
            agentBox.extendBy( boxData->readable().min * transformInvMtx );
            agentBox.extendBy( boxData->readable().max * transformInvMtx );
            if ( !agentClothBBox.isEmpty() )
            {
                // The cloth bounding box is in world space. Convert in local space
                agentBox.extendBy( agentClothBBox.min * transformInvMtx * rootInvMatrix );
                agentBox.extendBy( agentClothBBox.max * transformInvMtx * rootInvMatrix );
            }

            result.extendBy( agentBox.min - Imath::V3f( padding, padding, padding ) );
            result.extendBy( agentBox.max + Imath::V3f( padding, padding, padding ) );
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No boundingBox or rootMatrix data found." );
        }
        return result;
    }

    return Imath::Box3f();
}

bool AtomsCrowdGenerator::affectsBranchTransform( const Gaffer::Plug *input ) const
{
	return ( input == inPlug()->attributesPlug() ||
			 input == variationsPlug()->transformPlug() );
}

void AtomsCrowdGenerator::hashBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{

	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentType> or "/agents/<agentType>/<variation>"
		BranchCreator::hashBranchTransform( parentPath, branchPath, context, h );
	}
	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentType>/<variation>/<id>"
        inPlug()->attributesPlug()->hash( h );
		h.append( branchPath[3] );
	}
	else
	{
		// "/agents/<agentType>/<variation>/<id>/..."
		AgentScope scope( context, branchPath );
		variationsPlug()->transformPlug()->hash( h );
        h.append( branchPath[3] );
	}
}

Imath::M44f AtomsCrowdGenerator::computeBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
    // In atoms all the meshes have identity transformations, so here just return the default matrix
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return Imath::M44f();
	}
	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentType>/<variaiton>/<id>"
		return agentRootMatrix( parentPath, branchPath, context );
	}
	else
	{
		// "/agents/<agentName>/<variaiton>/<id>/..."
		AgentScope scope( context, branchPath );
		return variationsPlug()->transformPlug()->getValue();
	}
}

bool AtomsCrowdGenerator::affectsBranchAttributes( const Gaffer::Plug *input ) const
{
	return ( input == variationsPlug()->attributesPlug() ||
			 input == inPlug()->objectPlug() ||
			 input == inPlug()->attributesPlug() );
}

void AtomsCrowdGenerator::hashBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() < 2 )
	{
        // "/" or "/agents"
		h = outPlug()->attributesPlug()->defaultValue()->Object::hash();
	}
	else if ( branchPath.size() == 2 )
    {
        // "/agents/<agentType>"
        AgentScope instanceScope( context, branchPath );
        variationsPlug()->attributesPlug()->hash( h );
    }
    else if ( branchPath.size() == 3 )
    {
        // "/agents/<agentType>/<variation>"
        AgentScope instanceScope( context, branchPath );
        variationsPlug()->attributesPlug()->hash( h );
    }
    else if( branchPath.size() == 4 )
    {
        BranchCreator::hashBranchAttributes( parentPath, branchPath, context, h );
        {
            AgentScope instanceScope( context, branchPath );
            variationsPlug()->attributesPlug()->hash( h );
        }

        // Some of the attributes are stored as prim var on the input point cloud
        inPlug()->objectPlug()->hash( h );

        // The other attributes are stored inside the agent metadata map inside the cache
        auto agentData = agentCacheData( branchPath );
        auto metadataData = agentData->member<const CompoundData>( "metadata" );
        auto& metadataMap = metadataData->readable();
        for ( auto memberIt = metadataMap.cbegin(); memberIt != metadataMap.cend(); ++memberIt )
        {
            memberIt->second->hash( h );
        }
    }
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope instanceScope( context, branchPath );
		variationsPlug()->attributesPlug()->hash( h );
	}
}

ConstCompoundObjectPtr AtomsCrowdGenerator::computeBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() < 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->attributesPlug()->defaultValue();
	}
    else if ( branchPath.size() == 2 )
    {
        // "/agents/<agentType>"
        AgentScope scope( context, branchPath );
        return variationsPlug()->attributesPlug()->getValue();
    }
    else if ( branchPath.size() == 3 )
    {
        // "/agents/<agentType>/<variation>"
        AgentScope scope( context, branchPath );
        return variationsPlug()->attributesPlug()->getValue();
    }
	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentType>/<variation>/<id>"
		CompoundObjectPtr baseAttributes = new CompoundObject;
        auto& objMap = baseAttributes->members();

        // Get the agent metadata and convert them in gaffer attributes
        auto agentData = agentCacheData(branchPath);
        auto metadataData = agentData->member<const CompoundData>( "metadata" );

        auto& metadataMap = metadataData->readable();
        for (auto memberIt = metadataMap.cbegin(); memberIt != metadataMap.cend(); ++memberIt)
        {
            std::string variableName = "user:atoms:" + memberIt->first.string();
            // Atoms store all the data in double, while gaffer use floats, so convert them
            switch ( memberIt->second->typeId() )
            {
                case IECore::TypeId::DoubleDataTypeId:
                {
                    FloatDataPtr fData = new FloatData;
                    fData->writable() = runTimeCast<const DoubleData>( memberIt->second )->readable();
                    objMap[variableName] = fData;
                    break;
                }
                case IECore::TypeId ::V2dDataTypeId:
                {
                    V2fDataPtr fData = new V2fData;
                    fData->writable() = Imath::V2f( runTimeCast<const V2dData>( memberIt->second )->readable() );
                    objMap[variableName] = fData;
                    break;
                }
                case IECore::TypeId ::V3dDataTypeId:
                {
                    V3fDataPtr fData = new V3fData;
                    fData->writable() = Imath::V3f( runTimeCast<const V3dData>( memberIt->second )->readable() );
                    objMap[variableName] = fData;
                    break;
                }
                case IECore::TypeId ::QuatdDataTypeId:
                {
                    QuatfDataPtr fData = new QuatfData;
                    fData->writable() = Imath::Quatf( runTimeCast<const QuatdData>( memberIt->second )->readable() );
                    objMap[variableName] = fData;
                    break;
                }
                case IECore::TypeId ::M44dDataTypeId:
                {
                    M44fDataPtr fData = new M44fData;
                    fData->writable() = Imath::M44f( runTimeCast<const M44dData>( memberIt->second )->readable() );
                    objMap[variableName] = fData;
                    break;
                }
                default:
                {
                    objMap[variableName] = memberIt->second;
                }
            }
        }


        // Now extract the agent metadata saved on the point cloud prim var
        // Convert only the prim vars that has "atoms:" as prefix
        int currentAgentIndex = std::atoi( branchPath[3].string().c_str() );

        int agentIdPointIndex = -1;
        ScenePlug::PathScope scope( context, &parentPath );
        auto points = runTimeCast<const PointsPrimitive>( inPlug()->objectPlug()->getValue() );

        if ( !points )
        {
            return baseAttributes;
        }

        const auto agentId = points->variables.find( "atoms:agentId" );
        if ( agentId == points->variables.end() )
        {
            throw InvalidArgumentException(
                    "AtomsCrowdGenerator : Input must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
        }

        auto agentIdData = runTimeCast<const IntVectorData>( agentId->second.data );
        if ( !agentIdData ) {
            throw InvalidArgumentException(
                    "AtomsCrowdGenerator : Input must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
        }

        // Need to find with point has the data of the current agent
        const std::vector<int> &agentIdVec = agentIdData->readable();
        if (agentIdVec[currentAgentIndex] == currentAgentIndex)
        {
            agentIdPointIndex = currentAgentIndex;
        }
        else
        {
            for (size_t i = 0; i < agentIdVec.size(); ++i)
            {
                if (agentIdVec[i] == currentAgentIndex)
                {
                    agentIdPointIndex = i;
                    break;
                }
            }
        }

        for ( auto primIt = points->variables.cbegin(); primIt != points->variables.cend(); ++primIt )
        {
            size_t atomsIndex = primIt->first.find( "atoms:" );
            if ( atomsIndex != 0 ) {
                continue;
            }

            // All the data are trasfered to the renderer, so add "user:" as prefix
            std::string variableName = "user:" + primIt->first;

            switch( primIt->second.data->typeId() )
            {
                case IECore::TypeId::BoolVectorDataTypeId:
                {
                    auto data = runTimeCast<const BoolVectorData>( primIt->second.data );
                    BoolDataPtr outData = new BoolData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::IntVectorDataTypeId:
                {
                    auto data = runTimeCast<const IntVectorData>( primIt->second.data );
                    IntDataPtr outData = new IntData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::FloatVectorDataTypeId:
                {
                    auto data = runTimeCast<const FloatVectorData>( primIt->second.data );
                    FloatDataPtr outData = new FloatData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::StringVectorDataTypeId:
                {
                    auto data = runTimeCast<const StringVectorData>( primIt->second.data );
                    StringDataPtr outData = new StringData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::V2fVectorDataTypeId:
                {
                    auto data = runTimeCast<const V2fVectorData>( primIt->second.data );
                    V2fDataPtr outData = new V2fData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::V3fVectorDataTypeId:
                {
                    auto data = runTimeCast<const V3fVectorData>( primIt->second.data );
                    V3fDataPtr outData = new V3fData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::M44fVectorDataTypeId:
                {
                    auto data = runTimeCast<const M44fVectorData>( primIt->second.data );
                    M44fDataPtr outData = new M44fData();
                    outData->writable() = data->readable()[agentIdPointIndex];
                    objMap[variableName] = outData;
                    break;
                }

                case IECore::TypeId::QuatfVectorDataTypeId:
                {
                    auto data = runTimeCast<const QuatfVectorData>( primIt->second.data );
                    V3fDataPtr outData = new V3fData();
                    auto quat = data->readable()[agentIdPointIndex];
                    Imath::Eulerf euler;
                    euler.extract(quat);
                    outData->writable() = Imath::V3f(
                            euler.x * 180.0 / M_PI,
                            euler.y * 180.0 / M_PI,
                            euler.z * 180.0 / M_PI );
                    objMap[variableName] = outData;
                    break;
                }

                default:
                {
                    IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "Unable to set " + primIt->first +
                    " prim var of type: " +  primIt->second.data->typeName() );
                    break;
                }
            }
        }

		return baseAttributes;
	}
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
        CompoundObjectPtr outAttributes = variationsPlug()->attributesPlug()->getValue()->copy();
        // Remove the skin weights data
        outAttributes->members().erase( "jointIndexCount" );
        outAttributes->members().erase( "jointIndices" );
        outAttributes->members().erase( "jointWeights" );
        return outAttributes;
	}
}

bool AtomsCrowdGenerator::affectsBranchObject( const Gaffer::Plug *input ) const
{
	return ( input == variationsPlug()->objectPlug() ||
			 input == variationsPlug()->attributesPlug() ||
			 input == variationsPlug()->transformPlug() ||
			 input == inPlug()->objectPlug() ||
			 input == inPlug()->attributesPlug() ||
			 input == clothCachePlug()->objectPlug() ||
			 input == useInstancesPlug() );
}

void AtomsCrowdGenerator::hashBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() <= 4 )
	{
		// "/" or "/agents" or "/agents/<agentType>" or "/agents/<agentType>/<variation> or "/agents/<agentType>/<variation>/<id>"
		h = outPlug()->objectPlug()->defaultValue()->Object::hash();
	}
	else
	{
		// "/agents/<agentType>/<variation>/<id>/...
        clothCachePlug()->objectPlug()->hash( h );
        AgentScope instanceScope( context, branchPath );
        variationsPlug()->objectPlug()->hash( h );
        variationsPlug()->transformPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
        inPlug()->objectPlug()->hash( h );
		atomsPoseHash( parentPath, branchPath, context, h );
	}
}

ConstObjectPtr AtomsCrowdGenerator::computeBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() <= 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->objectPlug()->defaultValue();
	}

    ConstPointsPrimitivePtr points;
    CompoundData pointVariables;
    int currentAgentIndex = std::atoi( branchPath[3].string().c_str() );

    int agentIdPointIndex = -1;
    auto& pointVariablesData = pointVariables.writable();
    {
        ScenePlug::PathScope scope(context, &parentPath);
        points = runTimeCast<const PointsPrimitive>(inPlug()->objectPlug()->getValue());
    }

    if ( !points )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No input point clound found" );
        return variationsPlug()->objectPlug()->getValue();
    }

    const auto agentId = points->variables.find( "atoms:agentId" );
    if ( agentId == points->variables.end() )
    {
        throw InvalidArgumentException(
                "AtomsAttributes : Input must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
    }

    auto agentIdData = runTimeCast<const IntVectorData>( agentId->second.data );
    if ( !agentIdData ) {
        throw InvalidArgumentException(
                "AtomsAttributes : Input must be a PointsPrimitive containing an \"atoms:agentId\" vertex variable" );
    }


    // Get the point id that contain the agent data
    // We need this only to get blend shapes weights information
    const std::vector<int> &agentIdVec = agentIdData->readable();

    // Fetch agent id. In most cases, the array index matches the agent id.
    if (agentIdVec[currentAgentIndex] == currentAgentIndex)
    {
        agentIdPointIndex = currentAgentIndex;
    }
    else
    {
        for ( size_t i = 0; i < agentIdVec.size(); ++i )
        {
            if ( agentIdVec[i] == currentAgentIndex )
            {
                agentIdPointIndex = i;
                break;
            }
        }
    }

    for ( auto it = points->variables.cbegin(); it != points->variables.cend(); ++it )
    {
        size_t atomsIndex = it->first.find("atoms:");
        if ( atomsIndex == 0 )
        {
            pointVariablesData[it->first.substr( atomsIndex + 6,
                                                it->first.size() - 6 )] = it->second.data;
        }
    }

    // Extract cloth data
    auto cloth = agentClothMeshData( parentPath, branchPath );
    Imath::M44f rootMatrix = agentRootMatrix( parentPath, branchPath, context );

    // "/agents/<agentType>/<variation>/<id>/...
    AgentScope scope( context, branchPath );
    auto meshPrim = runTimeCast<const MeshPrimitive>( variationsPlug()->objectPlug()->getValue() );
    auto meshAttributes = runTimeCast<const CompoundObject>( variationsPlug()->attributesPlug()->getValue() );
    if ( !meshPrim || !meshAttributes )
    {
        return variationsPlug()->objectPlug()->getValue();
    }


    Imath::M44f transformMtx;
    ScenePath transfromPath;
    if (branchPath.size() > 2) {
        transfromPath.push_back(branchPath[1]);
        transfromPath.push_back(branchPath[2]);
        for (int ii = 4; ii < branchPath.size(); ++ii) {
            transfromPath.push_back(branchPath[ii]);
        }
        transformMtx = variationsPlug()->fullTransform(transfromPath);
    }


    auto agentData = agentCacheData( branchPath );
    auto metadataData = agentData->member<const CompoundData>( "metadata" );

    // Extract the pose matricies. Every matrix must be worldBindPoseInverseMatrix * worldMatrix * rootMatrixInverse
    auto poseData = agentData->member<const M44dVectorData>( "poseWorldMatrices" );
    if ( !poseData )
    {

        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No poseWorldMatrices found" );
        return variationsPlug()->objectPlug()->getValue();
    }

    auto poseNormalData = agentData->member<const M44dVectorData>( "poseNormalWorldMatrices" );
    if ( !poseNormalData )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No poseNormalWorldMatrices found" );
        return variationsPlug()->objectPlug()->getValue();
    }
    auto& worldMatrices = poseData->readable();
    auto& worldNormalMatrices = poseNormalData->readable();
    if ( worldMatrices.empty() || worldNormalMatrices.empty() )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "Empty poseWorldMatrices or poseNormalWorldMatrices attribute" );
        return variationsPlug()->objectPlug()->getValue();
    }


    MeshPrimitivePtr result = meshPrim->copy();

    auto pVarIt = result->variables.find( "P" );
    if ( pVarIt == result->variables.end() )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No P variable found" );
        return variationsPlug()->objectPlug()->getValue();
    }

    if ( cloth )
    {
        // Check the cloth stack order
        std::string stackOrder = "last";
        auto& clothData = cloth->readable();
        auto clothIt = clothData.find( "stackOrder" );
        if ( clothIt != clothData.cend() )
        {
            auto clothStackOrderData = runTimeCast<StringData>( clothIt->second );
            if ( clothStackOrderData )
            {
                stackOrder = clothStackOrderData->readable();
            }
        }

        // If the stack order is "first" then it contains transformation in T pose,
        // so the root matrix should be null
        if ( stackOrder == "first" )
        {
            rootMatrix = Imath::M44f();
        }

        // Apply the cloth deformation
        if ( !applyClothDeformer( branchPath, result, cloth, rootMatrix, transformMtx  ) )
        {
            return variationsPlug()->objectPlug()->getValue();
        }

        if ( stackOrder == "first" )
        {
            // It's a first stack order mesh, so apply the skinning
            applySkinDeformer( branchPath, result, meshPrim, meshAttributes, worldMatrices, transformMtx );
        }
    }
    else
    {
        // Apply blend shapes
        applyBlendShapesDeformer( branchPath, result, metadataData, pointVariablesData, agentIdPointIndex, transformMtx  );
        // Apply skinning
        applySkinDeformer( branchPath, result, meshPrim, meshAttributes, worldMatrices, transformMtx );
    }

    return result;
}

bool AtomsCrowdGenerator::affectsBranchChildNames( const Gaffer::Plug *input ) const
{
	return ( input == namePlug() ||
			 input == agentChildNamesPlug() ||
			 input == variationsPlug()->childNamesPlug() );
}

void AtomsCrowdGenerator::hashBranchChildNames( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.empty() )
	{
		// "/"
		BranchCreator::hashBranchChildNames( parentPath, branchPath, context, h );
		namePlug()->hash( h );
	}
	else if( branchPath.size() == 1 )
	{
		// "/agents"
		BranchCreator::hashBranchChildNames( parentPath, branchPath, context, h );
		agentChildNamesHash( parentPath, context, h );
	}
	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentType>"
        BranchCreator::hashBranchChildNames( parentPath, branchPath, context, h );
        agentChildNamesHash( parentPath, context, h );
        h.append( branchPath.back() );
	}
	else if( branchPath.size() == 3 )
	{
		// "/agents/<agentType>/<variation>"
		BranchCreator::hashBranchChildNames( parentPath, branchPath, context, h );
		agentChildNamesHash( parentPath, context, h );
		h.append( branchPath[1] );
		h.append( branchPath.back() );
	}
	else
	{
		// "/agents/<agentType>/<variation>/<id>/..."
		AgentScope scope( context, branchPath );
		h = variationsPlug()->childNamesPlug()->hash();
	}
}

ConstInternedStringVectorDataPtr AtomsCrowdGenerator::computeBranchChildNames( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{

	if( branchPath.empty() )
	{
		// "/"
		std::string name = namePlug()->getValue();
		if( name.empty() )
		{
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input name attribute is empty" );
		}
		InternedStringVectorDataPtr result = new InternedStringVectorData();
		result->writable().emplace_back( name );
        return result;
	}
	else if( branchPath.size() == 1 )
	{
		// "/agents"
        IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
        auto& varData = children->readable();
        InternedStringVectorDataPtr result = new InternedStringVectorData();
        auto& names = result->writable();
        for ( auto it = varData.cbegin(); it != varData.cend(); ++it )
        {
            names.push_back( it->first );
        }
        return result;
	}

	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentType>"
		IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );

		auto variationsData = children->member<CompoundData>( branchPath.back() );
        InternedStringVectorDataPtr result = new InternedStringVectorData();

        auto& names = result->writable();
        if ( variationsData )
        {
            auto& varData = variationsData->readable();
            for (auto it = varData.cbegin(); it != varData.cend(); ++it)
            {
                names.push_back(it->first);
            }
        }
        return result;
	}
    else if( branchPath.size() == 3 )
    {
        // "/agents/<agentType>/variation"
        IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
        auto variationsData = children->member<CompoundData>( branchPath[1] );
        InternedStringVectorDataPtr result = new InternedStringVectorData();
        if ( variationsData )
        {
            return variationsData->member<InternedStringVectorData>( branchPath.back() );
        }
        return result;
    }
	else
	{
		// "/agents/<agentType>/<variation>/<id>/..."
		AgentScope scope( context, branchPath );
		return variationsPlug()->childNamesPlug()->getValue();
	}
}

bool AtomsCrowdGenerator::affectsBranchSetNames( const Gaffer::Plug *input ) const
{
	return ( input == variationsPlug()->setNamesPlug() );
}

void AtomsCrowdGenerator::hashBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	h = variationsPlug()->setNamesPlug()->hash();
}

ConstInternedStringVectorDataPtr AtomsCrowdGenerator::computeBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context ) const
{
	return variationsPlug()->setNamesPlug()->getValue();
}

bool AtomsCrowdGenerator::affectsBranchSet( const Gaffer::Plug *input ) const
{
	return ( input == variationsPlug()->childNamesPlug() ||
			 input == agentChildNamesPlug() ||
			 input == variationsPlug()->setPlug() ||
			 input == namePlug() );
}

void AtomsCrowdGenerator::hashBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context, MurmurHash &h ) const
{
	BranchCreator::hashBranchSet( parentPath, setName, context, h );

	h.append( variationsPlug()->childNamesHash( ScenePath() ) );
	agentChildNamesHash( parentPath, context, h );
	variationsPlug()->setPlug()->hash( h );
	namePlug()->hash( h );
}

ConstPathMatcherDataPtr AtomsCrowdGenerator::computeBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context ) const
{
	ConstInternedStringVectorDataPtr agentNames = variationsPlug()->childNames( ScenePath() );
	IECore::ConstCompoundDataPtr instanceChildNames = agentChildNames( parentPath, context );
	ConstPathMatcherDataPtr inputSet = variationsPlug()->setPlug()->getValue();

	PathMatcherDataPtr outputSetData = new PathMatcherData;
	PathMatcher &outputSet = outputSetData->writable();

	std::vector<InternedString> branchPath( { namePlug()->getValue() } );
	std::vector<InternedString> agentPath( 1 );

	for( const auto &agentName : agentNames->readable() )
	{
		agentPath.back() = agentName;

		PathMatcher instanceSet = inputSet->readable().subTree( agentPath );
		auto variationNamesData = instanceChildNames->member<CompoundData>( agentName );
		if ( !variationNamesData )
			continue;

		auto variationNames = variationNamesData->readable();

		for( auto it = variationNames.cbegin(); it != variationNames.cend(); ++it )
		{
			branchPath.resize( 2 );
			branchPath.back() = agentName;
            PathMatcher variationInstanceSet = instanceSet.subTree( it->first );


			auto childNamesData = runTimeCast<InternedStringVectorData>( it->second );
			if ( !childNamesData )
				continue;

			const std::vector<InternedString> &childNames = childNamesData->readable();
			branchPath.emplace_back( InternedString() );
			branchPath.back() = it->first;
			branchPath.emplace_back( InternedString() );
            for( const auto &instanceChildName : childNames )
            {
                branchPath.back() = instanceChildName;
                outputSet.addPaths( variationInstanceSet, branchPath );
            }
		}
	}
	return outputSetData;
}

IECore::ConstCompoundDataPtr AtomsCrowdGenerator::agentChildNames( const ScenePath &parentPath, const Gaffer::Context *context ) const
{
	ScenePlug::PathScope scope( context, &parentPath );
	return agentChildNamesPlug()->getValue();
}

void AtomsCrowdGenerator::agentChildNamesHash( const ScenePath &parentPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ScenePlug::PathScope scope( context, &parentPath );
	agentChildNamesPlug()->hash( h );
}

AtomsCrowdGenerator::AgentScope::AgentScope( const Gaffer::Context *context, const ScenePath &branchPath )
	:	EditableScope( context )
{
    m_agentPath.reserve( 1 + ( branchPath.size() > 4 ? branchPath.size() - 4 : 0 ) );
    if( branchPath.size() > 1 )
        m_agentPath.push_back( branchPath[1] );
    if( branchPath.size() > 2 )
        m_agentPath.push_back( branchPath[2] );
	if( branchPath.size() > 4 )
	{
		m_agentPath.insert( m_agentPath.end(), branchPath.begin() + 4, branchPath.end() );
	}

    set( ScenePlug::scenePathContextName, &m_agentPath );
}

void AtomsCrowdGenerator::atomsPoseHash( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
    if ( !useInstancesPlug()->getValue() )
    {
        h.append( branchPath[3] );
        return;
    }
    auto meshAttributes = runTimeCast<const CompoundObject>( variationsPlug()->attributesPlug()->getValue() );
    if ( meshAttributes )
    {
        auto agentData = agentCacheData(branchPath);
        auto hashPoseData = agentData->member<const UInt64Data>( "hash" );
        if ( hashPoseData )
        {
            h.append( hashPoseData->readable() );
            return;
        }
    }
    h.append( branchPath[3] );
}

ConstCompoundDataPtr AtomsCrowdGenerator::agentCacheData(const ScenePath &branchPath) const
{
    auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
    if( !crowd )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
    }

    auto atomsData = crowd->member<const BlindDataHolder>( "atoms:agents" );
    if( !atomsData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator :  No agents data found." );
    }

    auto agentsData = atomsData->blindData();
    if( !agentsData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator :  No agents data found." );
    }

    auto agentData = agentsData->member<const CompoundData>( branchPath[3].string() );
    if( !agentData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : No agent found." );
    }

    return agentData;
}

ConstCompoundDataPtr AtomsCrowdGenerator::agentClothMeshData( const ScenePath &parentPath, const ScenePath &branchPath ) const
{
    ConstCompoundDataPtr result;
    auto cloth = runTimeCast<const BlindDataHolder>( clothCachePlug()->objectPlug()->getValue() );
    if ( !cloth )
    {
        return result;
    }

    auto clothBlindData = cloth->blindData();
    if ( !clothBlindData )
    {
        return result;
    }

    auto& clothData = clothBlindData->readable();
    auto clothAgentIt = clothData.find( branchPath[3] );
    if ( clothAgentIt == clothData.cend() )
    {
        clothAgentIt = clothData.find( "-1" );
        if ( clothAgentIt == clothData.cend() )
        {
            return result;
        }
    }

    auto clothAgent = runTimeCast<const CompoundData>( clothAgentIt->second );
    if ( !clothAgent )
    {
        return result;
    }

    for (auto clothMeshIt = clothAgent->readable().cbegin(); clothMeshIt != clothAgent->readable().cend(); ++clothMeshIt)
    {
        std::string key = clothMeshIt->first.string();
        auto pathSplitPos = key.rfind('|');
        if (pathSplitPos != std::string::npos)
        {
            key = key.substr(pathSplitPos + 1);
        }

        if (key == branchPath.back().string())
        {
            return runTimeCast<const CompoundData>( clothMeshIt->second );
        }

    }

     return result;
}

Imath::Box3d AtomsCrowdGenerator::agentClothBoudingBox( const ScenePath &parentPath, const ScenePath &branchPath ) const
{
    Imath::Box3d result;
    auto cloth = runTimeCast<const BlindDataHolder>( clothCachePlug()->objectPlug()->getValue() );
    if ( !cloth )
    {
        return result;
    }

    auto clothBlindData = cloth->blindData();
    if ( !clothBlindData )
    {
        return result;
    }

    auto& clothData = clothBlindData->readable();
    auto clothAgentIt = clothData.find( branchPath[3] );
    if ( clothAgentIt == clothData.cend() )
    {
        return result;
    }

    auto clothAgent = runTimeCast<const CompoundData>( clothAgentIt->second );
    if ( !clothAgent )
    {
        return result;
    }

    auto clothMeshIt = clothAgent->readable().find( "boundingBox" );
    if ( clothMeshIt == clothAgent->readable().cend() )
    {
        return result;
    }


    auto boxData = runTimeCast<const Box3dData>( clothMeshIt->second );
    if ( boxData ) {
        result = boxData->readable();
    }
    return result;
}

void AtomsCrowdGenerator::applySkinDeformer(
        const ScenePath &branchPath,
        MeshPrimitivePtr& result,
        ConstMeshPrimitivePtr& meshPrim,
        ConstCompoundObjectPtr& meshAttributes,
        const std::vector<Imath::M44d>& worldMatrices,
        const Imath::M44f& transformMatrix
        ) const
{
    auto jointIndexCountData = meshAttributes->member<const IntVectorData>( "jointIndexCount" );
    auto jointIndicesData = meshAttributes->member<const IntVectorData>( "jointIndices" );
    auto jointWeightsData = meshAttributes->member<const FloatVectorData>( "jointWeights" );
    auto vertexIdsData = meshPrim->vertexIds();
    if ( !jointIndexCountData || !jointIndicesData || !jointWeightsData || !vertexIdsData )
    {
        return;
    }

    auto& vertexIds = vertexIdsData->readable();
    auto &jointIndexCountVec = jointIndexCountData->readable();
    auto &jointIndicesVec = jointIndicesData->readable();
    auto &jointWeightsVec = jointWeightsData->readable();

    std::vector<size_t> offsetData( jointIndexCountVec.size() );
    size_t counter = 0;

    auto pVarIt = result->variables.find( "P" );
    if ( pVarIt == result->variables.end() )
        return;

    auto pData = runTimeCast<V3fVectorData>( pVarIt->second.data );
    if ( !pData )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No points found" );
        return;
    }

    auto &pointsData = pData->writable();

    if ( jointIndexCountVec.size() != pointsData.size() )
    {
        throw InvalidArgumentException( "AtomsAttributes : " + branchPath[3].string() + "Invalid jointIndexCount data of length " +
                                        std::to_string( jointIndexCountVec.size() ) + ", expected " + std::to_string( pointsData.size() ) +
                                        ". ALl points must be skinned, please check your setup scene" );
    }

    Imath::V4d newP, currentPoint;
    auto transformInverseMatrix = transformMatrix.inverse();
    auto transfromNormalMatrix = transformInverseMatrix.transposed();
    auto transfromNormalInverseMatrix = transfromNormalMatrix.inverse();
    for ( size_t vId = 0; vId < jointIndexCountVec.size(); ++vId )
    {
        offsetData[vId] = counter;
        Imath::V3f& inPoint = pointsData[vId];
        inPoint *= transformMatrix;
        newP.x = 0.0;
        newP.y = 0.0;
        newP.z = 0.0;
        newP.w = 1.0;

        currentPoint.x = inPoint.x;
        currentPoint.y = inPoint.y;
        currentPoint.z = inPoint.z;
        currentPoint.w = 1.0;

        for ( size_t jId = 0; jId < jointIndexCountVec[vId]; ++jId, ++counter )
        {
            int jointId = jointIndicesVec[counter];
            newP += currentPoint * jointWeightsVec[counter] * worldMatrices[jointId];
        }

        inPoint.x = newP.x;
        inPoint.y = newP.y;
        inPoint.z = newP.z;

        inPoint *= transformInverseMatrix;
    }


    auto nVarIt = result->variables.find( "N" );
    if ( nVarIt == result->variables.end() ) {
        return;
    }

    auto nData = runTimeCast<V3fVectorData>( nVarIt->second.data );
    if ( !nData )
    {
        return;
    }

    auto &normals = nData->writable();
    Imath::V4f newN, currentNormal;
    for ( size_t vtxId = 0; vtxId < vertexIds.size(); ++vtxId )
    {
        int pointId = vertexIds[vtxId];
        size_t offset = offsetData[pointId];

        Imath::V3f &inNormal = normals[vtxId];

        newN.x = 0.0f;
        newN.y = 0.0f;
        newN.z = 0.0f;
        newN.w = 0.0f;

        currentNormal.x = inNormal.x;
        currentNormal.y = inNormal.y;
        currentNormal.z = inNormal.z;
        currentNormal.w = 0.0;

        currentNormal *= transfromNormalMatrix;
        currentNormal.w = 0.0;

        for ( size_t jId = 0; jId < jointIndexCountVec[pointId]; ++jId ) {
            int jointId = jointIndicesVec[offset];
            newN += currentNormal * jointWeightsVec[offset] * worldMatrices[jointId];
        }

        newN *= transfromNormalInverseMatrix;

        inNormal.x = newN.x;
        inNormal.y = newN.y;
        inNormal.z = newN.z;
        inNormal.normalize();
    }
}

void AtomsCrowdGenerator::applyBlendShapesDeformer(
        const ScenePath &branchPath,
        MeshPrimitivePtr& result,
        const IECore::CompoundData* metadataData,
        const CompoundDataMap& pointVariablesData,
        const int agentIdPointIndex,
        const Imath::M44f& transformMatrix
        ) const
{
    auto pVarIt = result->variables.find( "P" );
    if ( pVarIt == result->variables.end() )
    {
        return;
    }

    auto meshPointData = runTimeCast<V3fVectorData>( pVarIt->second.data );
    auto blendShapeCountDataIt = result->variables.find( "blendShapeCount" );
    if ( blendShapeCountDataIt == result->variables.end() || !metadataData || !meshPointData )
    {
        return;
    }

    auto &points = meshPointData->writable();
    auto &metadataMap = metadataData->readable();
    auto blendShapeCountData = runTimeCast<IntData>( blendShapeCountDataIt->second.data );
    int numBlendShapes = blendShapeCountData->readable();

    std::vector<const std::vector<Imath::V3f>*> blendPoints;
    std::vector<double> blendWeights;
    for ( size_t blendId = 0; blendId < numBlendShapes; ++blendId )
    {
        double weight = 0.0;
        std::string blendWeightMetaName = std::string( branchPath[1].c_str()) +
                                          "_" + std::string( branchPath.back().c_str()) + "_" +
                                          std::to_string(blendId);

        auto pointsVariableIt = pointVariablesData.find( blendWeightMetaName );
        if ( ( agentIdPointIndex != -1 ) && ( pointsVariableIt != pointVariablesData.end() ) &&
            ( pointsVariableIt->second->typeId() == FloatVectorData::staticTypeId() ) )
        {
            auto &primWeightVec = runTimeCast<FloatVectorData>( pointsVariableIt->second )->readable();
            weight = primWeightVec[agentIdPointIndex];
        }
        else
        {
            auto blendWeightDataIt = metadataMap.find( blendWeightMetaName );
            if (blendWeightDataIt == metadataMap.cend())
                continue;

            auto blendWeightData = runTimeCast<const DoubleData>( blendWeightDataIt->second );
            if (!blendWeightData)
                continue;

            weight = blendWeightData->readable();
        }

        if (weight < 0.00001)
            continue;

        auto blendPointsData = runTimeCast<V3fVectorData>(
                result->variables["blendShape_" + std::to_string( blendId ) + "_P"].data);
        if (!blendPointsData)
            continue;


        auto &blendPointsVec = blendPointsData->readable();

        if ( blendPointsVec.size() != points.size() )
            continue;

        blendPoints.push_back( &blendPointsVec );
        blendWeights.push_back(weight);
    }


    Imath::V3f currentPoint;
    for ( size_t ptId = 0; ptId < points.size(); ++ptId )
    {
        auto& meshPoint = points[ptId];
        currentPoint.x = meshPoint.x;
        currentPoint.y = meshPoint.y;
        currentPoint.z = meshPoint.z;

        for ( size_t blendId = 0; blendId < blendPoints.size(); blendId++ )
        {
            auto& blendP = *blendPoints[blendId];
            double weight = blendWeights[blendId];
            currentPoint.x += ( blendP[ptId].x - points[ptId].x ) * weight;
            currentPoint.y += ( blendP[ptId].y - points[ptId].y ) * weight;
            currentPoint.z += ( blendP[ptId].z - points[ptId].z ) * weight;
        }

        meshPoint.x = currentPoint.x;
        meshPoint.y = currentPoint.y;
        meshPoint.z = currentPoint.z;
    }

    auto nVarIt = result->variables.find( "N" );
    if ( nVarIt != result->variables.end() )
    {
        auto meshNormalData = runTimeCast<V3fVectorData>( nVarIt->second.data );
        if (meshNormalData) {
            std::vector<const std::vector<Imath::V3f> *> blendNormals;
            auto &normals = meshNormalData->writable();
            for ( size_t blendId = 0; blendId < numBlendShapes; ++blendId ) {
                auto blendNormalsData = runTimeCast<V3fVectorData>(
                        result->variables["blendShape_" + std::to_string( blendId ) + "_N"].data);
                if ( !blendNormalsData )
                    continue;

                auto &blendNormalsVec = blendNormalsData->readable();
                if ( blendNormalsVec.size() != normals.size() )
                    continue;

                blendNormals.push_back( &blendNormalsVec );

            }

            Imath::V3f currentNormal;
            if ( blendWeights.size() > 0 && blendNormals.size() == blendWeights.size() ) {
                for ( size_t ptId = 0; ptId < normals.size(); ++ptId ) {
                    auto &meshNormal = normals[ptId];
                    currentNormal.x = meshNormal.x;
                    currentNormal.y = meshNormal.y;
                    currentNormal.z = meshNormal.z;

                    Imath::V3f pNormals( currentNormal.x, currentNormal.y, currentNormal.z );
                    Imath::V3f blendNormal( 0.0f, 0.0f, 0.0f );
                    size_t counterN = 0;

                    for ( size_t blendId = 0; blendId < blendNormals.size(); blendId++ ) {
                        auto &blendN = *blendNormals[blendId];
                        double weight = blendWeights[blendId];
                        blendNormal += pNormals * ( 1.0f - weight ) + blendN[ptId] * weight;
                        counterN++;
                    }
                    if ( counterN > 0 ) {
                        blendNormal = blendNormal / static_cast<float>( counterN );
                        blendNormal.normalize();
                        currentNormal.x = blendNormal.x;
                        currentNormal.y = blendNormal.y;
                        currentNormal.z = blendNormal.z;
                    }

                    meshNormal.x = currentNormal.x;
                    meshNormal.y = currentNormal.y;
                    meshNormal.z = currentNormal.z;
                }
            }
        }
    }

    // remove blend shape data
    result->variables.erase( "blendShapeCount" );
    for (int blendId = 0; blendId < numBlendShapes; ++blendId )
    {
        result->variables.erase( "blendShape_" + std::to_string( blendId ) + "_P" );
        result->variables.erase( "blendShape_" + std::to_string( blendId ) + "_N" );
    }
}

Imath::M44f AtomsCrowdGenerator::agentRootMatrix(
        const ScenePath &parentPath,
        const ScenePath &branchPath,
        const Gaffer::Context *context
        ) const
{
    ConstCompoundObjectPtr crowd;
    {
        ScenePlug::PathScope scope( context, &parentPath );
        crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
    }

    if( !crowd )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
    }

    auto atomsData = crowd->member<const BlindDataHolder>( "atoms:agents" );
    if( !atomsData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator :  computeBranchTransform : No agents data found." );
    }

    auto agentsData = atomsData->blindData();
    if( !agentsData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchTransform : No agents data found." );
    }

    auto agentData = agentsData->member<const CompoundData>( branchPath[3].string() );
    if( !agentData )
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchTransform : No agent found." );
    }

    auto poseData = agentData->member<const M44dData>( "rootMatrix" );
    if ( poseData )
    {
        return Imath::M44f( poseData->readable() );
    }
    else
    {
        throw InvalidArgumentException( "AtomsCrowdGenerator : No rootMatrix data found." );
    }
}

bool AtomsCrowdGenerator::applyClothDeformer(
        const ScenePath &branchPath,
        MeshPrimitivePtr& result,
        ConstCompoundDataPtr& cloth,
        const Imath::M44f& rootMatrix,
        const Imath::M44f& transformMatrix
        ) const
{
    Imath::M44f rootInvMatrix = ( transformMatrix * rootMatrix ).inverse();
    Imath::M44f rootNormalMatrix = ( transformMatrix * rootMatrix ).transposed();

    auto transformInverseMatrix = transformMatrix.inverse();
    auto transformNormalMatrix = transformInverseMatrix.transposed();
    auto transformNormalInverseMatrix = transformNormalMatrix.inverse();

    auto pVarIt = result->variables.find( "P" );
    auto pData = runTimeCast<V3fVectorData>( pVarIt->second.data );
    auto& clothData = cloth->readable();
    auto clothPIt = clothData.find( "P" );
    if ( clothPIt == clothData.cend() ) {
        return false;
    }

    auto pClothData = runTimeCast<V3dVectorData>( clothPIt->second );
    if ( !pClothData )
    {
        return false;
    }

    auto& outputP = pData->writable();
    auto& inputP = pClothData->readable();
    if ( outputP.size() != inputP.size() )
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "Invalid cloth mesh points" );
        return false;
    }

    for ( size_t pId = 0; pId < outputP.size(); ++pId )
    {
        outputP[pId] = inputP[pId] * rootInvMatrix;
    }

    auto nVarIt = result->variables.find( "N" );
    auto vertexIdsData = result->vertexIds();
    if ( !vertexIdsData )
        return true;

    if ( nVarIt == result->variables.end() )
    {
        return true;
    }

    auto nData = runTimeCast<V3fVectorData>( nVarIt->second.data );
    if ( !nData )
    {
        return true;
    }

    auto& vertexIds = vertexIdsData->readable();
    auto clothNIt = clothData.find( "N" );
    if (clothNIt == clothData.cend())
    {
        return true;
    }

    auto nClothData = runTimeCast<V3dVectorData>( clothNIt->second );
    if ( !nClothData )
    {
        return true;
    }

    auto &outputN = nData->writable();
    auto &inputN = nClothData->readable();

    // Check if the normals are as Vertex or FaceVarying interpolation
    if ( inputN.size() == pData->writable().size() )
    {
        for ( size_t vId = 0; vId < vertexIds.size(); ++vId ) {
            size_t nId = vertexIds[vId];
            const auto &currentN = inputN[nId];
            auto &outN = outputN[vId];
            Imath::V4f newN( currentN.x, currentN.y, currentN.z, 0.0f );
            newN = newN * rootNormalMatrix;
            outN.x = newN.x;
            outN.y = newN.y;
            outN.z = newN.z;
            outN.normalize();
        }
    }
    else if ( inputN.size() == vertexIds.size() )
    {
        for ( size_t nId = 0; nId < outputN.size(); ++nId )
        {
            const auto &currentN = inputN[nId];
            auto &outN = outputN[nId];
            Imath::V4f newN( currentN.x, currentN.y, currentN.z, 0.0f );
            newN = newN * transformNormalInverseMatrix * rootNormalMatrix;
            outN.x = newN.x;
            outN.y = newN.y;
            outN.z = newN.z;
            outN.normalize();
        }
    }
    else
    {
        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "Agent " + branchPath[3].string() +
        "Invalid cloth mesh normals" );
    }

    return true;
}
