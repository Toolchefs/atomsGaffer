#include <AtomsUtils/Logger.h>
#include "AtomsGaffer/AtomsCrowdGenerator.h"
#include "Atoms/GlobalNames.h"

#include "IECoreScene/PointsPrimitive.h"
#include "IECoreScene/MeshPrimitive.h"

#include "IECore/NullObject.h"

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

Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug()
{
	return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 3);
}

const Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug() const
{
	return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 3 );
}

Gaffer::BoolPlug *AtomsCrowdGenerator::useInstancesPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::BoolPlug *AtomsCrowdGenerator::useInstancesPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

void AtomsCrowdGenerator::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	BranchCreator::affects( input, outputs );

	if(
		input == inPlug()->objectPlug() ||
        input == inPlug()->attributesPlug() ||
		input == variationsPlug()->childNamesPlug()
	)
	{
		outputs.push_back( agentChildNamesPlug() );
	}

	if(
		input == namePlug() ||
		input == agentChildNamesPlug() ||
		input == variationsPlug()->childNamesPlug()
	)
	{
		outputs.push_back( outPlug()->childNamesPlug() );
	}

	if(
		input == inPlug()->objectPlug() ||
		input == inPlug()->attributesPlug() ||
		input == namePlug() ||
		input == variationsPlug()->boundPlug() ||
		input == variationsPlug()->transformPlug() ||
		input == agentChildNamesPlug()
	)
	{
		outputs.push_back( outPlug()->boundPlug() );
	}

	if(
		input == inPlug()->objectPlug() ||
		input == inPlug()->attributesPlug() ||
		input == variationsPlug()->transformPlug()
	)
	{
		outputs.push_back( outPlug()->transformPlug() );
	}

	if(
		input == variationsPlug()->attributesPlug() ||
		input == inPlug()->objectPlug() ||
		input == inPlug()->attributesPlug()
	)
	{
		outputs.push_back( outPlug()->attributesPlug() );
	}
}

void AtomsCrowdGenerator::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, MurmurHash &h ) const
{
	BranchCreator::hash( output, context, h );

	if( output == agentChildNamesPlug() )
	{
		inPlug()->objectPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
		h.append( variationsPlug()->childNamesHash( ScenePath() ) );
		useInstancesPlug()->hash( h );
	}
}

void AtomsCrowdGenerator::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
	// The instanceChildNamesPlug is evaluated in a context in which
	// scene:path holds the parent path for a branch.
	if( output == agentChildNamesPlug() )
	{
		// Here we compute and cache the child names for all of
		// the /agents/<agentName> locations at once. We
		// could instead compute them one at a time in
		// computeBranchChildNames() but that would require N
		// passes over the input points, where N is the number
		// of instances.

		ConstPointsPrimitivePtr crowd = runTimeCast<const PointsPrimitive>( inPlug()->objectPlug()->getValue() );
		if( !crowd )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive Object." );
		}


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

	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentName>"
		BranchCreator::hashBranchBound( parentPath, branchPath, context, h );

		inPlug()->attributesPlug()->hash( h );
        inPlug()->objectPlug()->hash( h );
		agentChildNamesHash( parentPath, context, h );
		h.append( branchPath.back() );
		{
			AgentScope scope( context, branchPath );
			variationsPlug()->transformPlug()->hash( h );
			variationsPlug()->boundPlug()->hash( h );
		}
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope instanceScope( context, branchPath );
		variationsPlug()->boundPlug()->hash( h );
        inPlug()->boundPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
        inPlug()->objectPlug()->hash( h );
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
		ConstCompoundObjectPtr crowd;
		{
			ScenePlug::PathScope scope(context, parentPath);
			crowd = runTimeCast<const CompoundObject>(inPlug()->attributesPlug()->getValue());
		}

        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        auto agentData = crowd->member<const CompoundData>( branchPath[3] );
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchBound : No agent found." );
        }

        auto boxData = agentData->member<const Box3dData>( "boundingBox" );
        Imath::Box3f result;
        if ( boxData )
        {
            auto agentBox = boxData->readable();
            result.min = agentBox.min;
            result.max = agentBox.max;
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No boundingBox or rootMatrix data found." );
        }
        return result;
    }
}

void AtomsCrowdGenerator::hashBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		BranchCreator::hashBranchTransform( parentPath, branchPath, context, h );
	}
	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentName>/<id>"
		inPlug()->objectPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
		h.append( branchPath[3] );
	}

	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		variationsPlug()->transformPlug()->hash(h);
		inPlug()->attributesPlug()->hash(h);
        inPlug()->objectPlug()->hash( h );
        h.append( branchPath[3] );
	}
}

Imath::M44f AtomsCrowdGenerator::computeBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return Imath::M44f();
	}

	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentType>/<variaiton>/<id>"
		Imath::M44f result;

		ConstCompoundObjectPtr crowd;
		{
			ScenePlug::PathScope scope(context, parentPath);
			crowd = runTimeCast<const CompoundObject>(inPlug()->attributesPlug()->getValue());
		}

        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        auto agentData = crowd->member<const CompoundData>( branchPath[3] );
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchTransform : No agent found." );
        }

        auto poseData = agentData->member<const M44dData>( "rootMatrix" );
        if ( poseData )
        {
            result = Imath::M44f(poseData->readable());
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No rootMatrix data found." );
        }

		return result;
	}

	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		return variationsPlug()->transformPlug()->getValue();
	}

    return Imath::M44f();
}

void AtomsCrowdGenerator::hashBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		h = outPlug()->attributesPlug()->defaultValue()->Object::hash();
	}
    else if( branchPath.size() == 4 )
    {
        BranchCreator::hashBranchAttributes( parentPath, branchPath, context, h );
        inPlug()->attributesPlug()->hash( h );
        {
            AgentScope scope( context, branchPath );
            variationsPlug()->attributesPlug()->hash( h );
        }
        // \todo: Hash promoted attributes. See GafferScene::Instancer::EngineData for hints.
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
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->attributesPlug()->defaultValue();
	}

	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentName>/<id>"
		ConstCompoundObjectPtr baseAttributes;
		{
			AgentScope instanceScope( context, branchPath );
			baseAttributes = variationsPlug()->attributesPlug()->getValue();
		}

		// \todo: Append promoted attributes. See GafferScene::Instancer::EngineData for hints.
		return baseAttributes;
	}

	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
		return variationsPlug()->attributesPlug()->getValue();
	}

}

void AtomsCrowdGenerator::hashBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() <= 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		h = outPlug()->objectPlug()->defaultValue()->Object::hash();
	}
	else
	{
		// "/agents/<agentName>/<id>/...
        AgentScope instanceScope( context, branchPath );
        variationsPlug()->objectPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
        inPlug()->objectPlug()->hash( h );
		poseAttributesHash( branchPath, h );
	}
}

ConstObjectPtr AtomsCrowdGenerator::computeBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() <= 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->objectPlug()->defaultValue();
	}
	else
	{
	    ConstPointsPrimitivePtr points;
        CompoundData pointVariables;
        int currentAgentIndex = std::atoi( branchPath[3].string().c_str() );

        int agentIdPointIndex = -1;
        auto& pointVariablesData = pointVariables.writable();
        {
            ScenePlug::PathScope scope( context, parentPath );
            points = runTimeCast<const PointsPrimitive>( inPlug()->objectPlug()->getValue() );

            if ( points )
            {
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
                std::vector<int> agentIdVec = agentIdData->readable();

                for ( size_t i = 0; i < agentIdVec.size(); ++i )
                {
                    if ( agentIdVec[i] == currentAgentIndex )
                    {
                        agentIdPointIndex = i;
                        break;
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
            }
        }

		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
		auto meshPrim = runTimeCast<const MeshPrimitive>( variationsPlug()->objectPlug()->getValue() );
        auto meshAttributes = runTimeCast<const CompoundObject>( variationsPlug()->attributesPlug()->getValue() );
		if ( meshPrim && meshAttributes )
        {
            auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
            if( !crowd )
            {
                throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
            }


            auto agentData = crowd->member<const CompoundData>( branchPath[3] );
            if( !agentData )
            {
                throw InvalidArgumentException( "AtomsCrowdGenerator : computeBranchObject : No agent found." );
            }

            auto metadataData = agentData->member<const CompoundData>( "metadata" );
            auto poseData = agentData->member<const M44dVectorData>( "poseWorldMatrices" );
            auto poseNormalData = agentData->member<const M44dVectorData>( "poseNormalWorldMatrices" );
            if ( poseData && !poseData->readable().empty() )
            {
                auto& worldMatrices = poseData->readable();
                auto& worldNormalMatrices = poseNormalData->readable();

                MeshPrimitivePtr result = meshPrim->copy();
				auto meshPointData = runTimeCast<V3fVectorData>( result->variables["P"].data );
				auto meshNormalData = runTimeCast<V3fVectorData>( result->variables["N"].data );
                auto blendShapeCountDataIt = result->variables.find( "blendShapeCount" );

                if ( blendShapeCountDataIt != result->variables.end() && metadataData && meshPointData && meshNormalData )
                {
					auto &points = meshPointData->writable();
					auto &normals = meshNormalData->writable();
                	auto &metadataMap = metadataData->readable();
                    auto blendShapeCountData = runTimeCast<IntData>( blendShapeCountDataIt->second.data );
                    int numBlendShapes = blendShapeCountData->readable();

                    std::vector<const std::vector<Imath::V3f>*> blendPoints;
					std::vector<const std::vector<Imath::V3f>*> blendNormals;
					std::vector<double> blendWeights;
                    for ( size_t blendId=0; blendId < numBlendShapes; ++blendId )
					{
                        double weight = 0.0;
                        std::string blendWeightMetaName = std::string( branchPath[1].c_str() ) +
                        		"_" + std::string( branchPath.back().c_str() ) +"_" + std::to_string( blendId );

                        auto pointsVariableIt = pointVariablesData.find( blendWeightMetaName );
                        if ( ( agentIdPointIndex != -1 ) && ( pointsVariableIt != pointVariablesData.end() ) &&
                                ( pointsVariableIt->second->typeId() == FloatVectorData::staticTypeId() ) )
                        {
                            auto& primWeightVec = runTimeCast<FloatVectorData>( pointsVariableIt->second )->readable();
                            weight = primWeightVec[agentIdPointIndex];
                        }
                        else
						{
                            auto blendWeightDataIt = metadataMap.find( blendWeightMetaName );
                            if ( blendWeightDataIt == metadataMap.cend() )
                                continue;

                            auto blendWeightData = runTimeCast<const DoubleData>( blendWeightDataIt->second );
                            if ( !blendWeightData )
                                continue;

                            weight = blendWeightData->readable();
                        }
						if ( weight < 0.00001 )
						    continue;

						auto blendPointsData = runTimeCast<V3fVectorData>( result->variables["blendShape_" + std::to_string(blendId) + "_P"].data );
						if ( !blendPointsData )
							continue;

						auto blendNormalsData = runTimeCast<V3fVectorData>( result->variables["blendShape_" + std::to_string(blendId) + "_N"].data );
						if ( !blendNormalsData )
							continue;

						auto& blendPointsVec = blendPointsData->readable();

						if ( blendPointsVec.size() != points.size() )
							continue;


						auto& blendNormalsVec = blendNormalsData->readable();

						if ( blendNormalsVec.size() != normals.size() )
							continue;

                        blendPoints.push_back(&blendPointsVec);
						blendNormals.push_back(&blendNormalsVec);
						blendWeights.push_back(weight);
					}

					for ( size_t ptId=0; ptId < points.size(); ++ptId )
                    {
					    Imath::V3f currentPoint;
					    auto& meshPoint = points[ptId];
                        currentPoint.x = meshPoint.x;
                        currentPoint.y = meshPoint.y;
                        currentPoint.z = meshPoint.z;


                        for ( size_t blendId = 0; blendId < blendPoints.size(); blendId++ )
                        {
                            auto& blendP = *blendPoints[blendId];
                            double weight = blendWeights[blendId];
                            currentPoint.x += (blendP[ptId].x - points[ptId].x) * weight;
                            currentPoint.y += (blendP[ptId].y - points[ptId].y) * weight;
                            currentPoint.z += (blendP[ptId].z - points[ptId].z) * weight;
                        }

                        meshPoint.x = currentPoint.x;
                        meshPoint.y = currentPoint.y;
                        meshPoint.z = currentPoint.z;
                    }

					for ( size_t ptId=0; ptId < points.size(); ++ptId )
					{
						Imath::V3f currentNormal;
						auto &meshNormal = normals[ptId];
						currentNormal.x = meshNormal.x;
						currentNormal.y = meshNormal.y;
						currentNormal.z = meshNormal.z;

						Imath::V3f pNormals( currentNormal.x, currentNormal.y, currentNormal.z );
						Imath::V3f  blendNormal(0.0, 0.0, 0.0);
						size_t counterN = 0;

						for ( size_t blendId = 0; blendId < blendNormals.size(); blendId++ )
						{
							auto &blendN = *blendNormals[blendId];
							double weight = blendWeights[blendId];
							blendNormal += pNormals * ( 1.0f - weight ) + blendN[ptId] * weight;
							counterN++;
						}
						if ( counterN > 0 )
						{
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


                auto jointIndexCountData = meshAttributes->member<const IntVectorData>( "jointIndexCount" );
                auto jointIndicesData = meshAttributes->member<const IntVectorData>( "jointIndices" );
                auto jointWeightsData = meshAttributes->member<const FloatVectorData>( "jointWeights" );
                auto vertexIdsData = meshPrim->vertexIds();
                if ( jointIndexCountData && jointIndicesData && jointWeightsData && vertexIdsData )
                {
                    auto& vertexIds = vertexIdsData->readable();
                    auto &jointIndexCountVec = jointIndexCountData->readable();
                    auto &jointIndicesVec = jointIndicesData->readable();
                    auto &jointWeightsVec = jointWeightsData->readable();

                    std::vector<size_t> offsetData( jointIndexCountVec.size() );
                    size_t counter = 0;
                    auto pData = runTimeCast<V3fVectorData>( result->variables["P"].data );
                    if ( pData )
                    {
                        auto &pointsData = pData->writable();
                        for ( size_t vId = 0; vId < jointIndexCountVec.size(); ++vId )
                        {
                            offsetData[vId] = counter;
                            Imath::V4d newP, currentPoint;
                            Imath::V3f& inPoint = pointsData[vId];
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
                        }
                    }
                    else
                    {
						IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No points found" );
                    }


                    auto nData = runTimeCast<V3fVectorData>(result->variables["N"].data);
                    if ( nData )
                    {
                        auto &normals = nData->writable();
                        for ( size_t vtxId = 0; vtxId < vertexIds.size(); ++vtxId )
                        {
                            int pointId = vertexIds[vtxId];
                            size_t offset = offsetData[pointId];

                            Imath::V4d newP, currentPoint;
                            Imath::V3f& inNormal = normals[vtxId];

                            newP.x = 0.0;
                            newP.y = 0.0;
                            newP.z = 0.0;
                            newP.w = 0.0;

                            currentPoint.x = inNormal.x;
                            currentPoint.y = inNormal.y;
                            currentPoint.z = inNormal.z;
                            currentPoint.w = 0.0;

                            for (size_t jId = 0; jId < jointIndexCountVec[pointId]; ++jId) {
                                int jointId = jointIndicesVec[offset];
                                newP += currentPoint * jointWeightsVec[offset] * worldMatrices[jointId];
                            }

                            inNormal.x = newP.x;
                            inNormal.y = newP.y;
                            inNormal.z = newP.z;
                            inNormal.normalize();
                        }
                    }
                    else
                    {
                        IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "No normals found" );
                    }
                }

				// add metadata to the mesh prim
                auto metaDataPtr = agentData->member<const CompoundData>( "metadata" );
                if ( metaDataPtr )
                {
                    auto metaData = metaDataPtr->readable();
                    for ( auto metaIt = metaData.cbegin(); metaIt != metaData.cend(); ++metaIt )
                    {
                        if ( metaIt->first == "id" )
                            continue;

                        if ( metaIt->second->typeId() == V3dData::staticTypeId() )
                        {
                            V3fDataPtr vec3fData = new V3fData();
                            auto vec3dData = runTimeCast<const V3dData>( metaIt->second );
                            vec3fData->writable() = vec3dData->readable();
                            continue;
                        }
                        if ( metaIt->second->typeId() == DoubleData::staticTypeId() )
                        {
                            FloatDataPtr fData = new FloatData();
                            auto dData = runTimeCast<const DoubleData>( metaIt->second );
                            fData->writable() = dData->readable();
                            continue;
                        }

                        if ( metaIt->second->typeId() == V3fVectorData::staticTypeId() ||
                             metaIt->second->typeId() == V3fVectorDataBase::staticTypeId() ||
                             metaIt->second->typeId() == FloatVectorData::staticTypeId() ||
                             metaIt->second->typeId() == BoolVectorData::staticTypeId() ||
                             metaIt->second->typeId() == IntVectorData::staticTypeId() ||
                             metaIt->second->typeId() == V2fVectorData::staticTypeId() ||
                             metaIt->second->typeId() == M44fVectorData::staticTypeId() )
                        {
                            continue;
                        }
                        result->variables[metaIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, metaIt->second );
                    }
                }

                if ( agentIdPointIndex != -1 )
                {
                    for ( auto primIt = pointVariablesData.begin(); primIt != pointVariablesData.end(); ++primIt )
                    {
                        switch( primIt->second->typeId() )
                        {
                            case IECore::TypeId::BoolVectorDataTypeId:
                            {
                                auto data = runTimeCast<const BoolVectorData>( primIt->second );
                                BoolDataPtr outData = new BoolData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::IntVectorDataTypeId:
                            {
                                auto data = runTimeCast<const IntVectorData>( primIt->second );
                                IntDataPtr outData = new IntData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::FloatVectorDataTypeId:
                            {
                                auto data = runTimeCast<const FloatVectorData>( primIt->second );
                                FloatDataPtr outData = new FloatData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::StringVectorDataTypeId:
                            {
                                auto data = runTimeCast<const StringVectorData>( primIt->second );
                                StringDataPtr outData = new StringData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::V2fVectorDataTypeId:
                            {
                                auto data = runTimeCast<const V2fVectorData>( primIt->second );
                                V2fDataPtr outData = new V2fData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::V3fVectorDataTypeId:
                            {
                                auto data = runTimeCast<const V3fVectorData>( primIt->second );
                                V3fDataPtr outData = new V3fData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::M44fVectorDataTypeId:
                            {
                                auto data = runTimeCast<const M44fVectorData>( primIt->second );
                                M44fDataPtr outData = new M44fData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            case IECore::TypeId::QuatfVectorDataTypeId:
                            {
                                auto data = runTimeCast<const QuatfVectorData>(primIt->second);
                                QuatfDataPtr outData = new QuatfData();
                                outData->writable() = data->readable()[agentIdPointIndex];
                                result->variables[primIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, outData );
                                break;
                            }

                            default:
                            {
                                IECore::msg( IECore::Msg::Warning, "AtomsCrowdGenerator", "Unable to set " + primIt->first.string() + "prim var" );
                                break;
                            }
                        }
                    }
                }

                return result;
            }
        }

		return variationsPlug()->objectPlug()->getValue();
	}
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
        h.append( branchPath.back() );
	}
	else if( branchPath.size() <= 3 )
	{
		// "/agents/<agentName>"
		BranchCreator::hashBranchChildNames( parentPath, branchPath, context, h );
		agentChildNamesHash( parentPath, context, h );
		h.append( branchPath.back() );
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
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
			return outPlug()->childNamesPlug()->defaultValue();
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

		auto variationsData = children->member<CompoundData>(branchPath.back());
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
        IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
        auto variationsData = children->member<CompoundData>( branchPath[1] );
        InternedStringVectorDataPtr result = new InternedStringVectorData();
        if (variationsData)
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

void AtomsCrowdGenerator::hashBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	h = variationsPlug()->setNamesPlug()->hash();
}
ConstInternedStringVectorDataPtr AtomsCrowdGenerator::computeBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context ) const
{
	return variationsPlug()->setNamesPlug()->getValue();
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

        std::vector<std::string> paths;
        inputSet->readable().paths( paths );

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
	ScenePlug::PathScope scope( context, parentPath );
	return agentChildNamesPlug()->getValue();
}

void AtomsCrowdGenerator::agentChildNamesHash( const ScenePath &parentPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ScenePlug::PathScope scope( context, parentPath );
	agentChildNamesPlug()->hash( h );
}

AtomsCrowdGenerator::AgentScope::AgentScope( const Gaffer::Context *context, const ScenePath &branchPath )
	:	EditableScope( context )
{
    assert( branchPath.size() >= 3 );
	ScenePath agentPath;
	agentPath.reserve( 1 + ( branchPath.size() > 4 ? branchPath.size() - 4 : 0 ) );
	agentPath.push_back( branchPath[1] );
    agentPath.push_back( branchPath[2] );
	if( branchPath.size() > 4 )
	{
		agentPath.insert( agentPath.end(), branchPath.begin() + 4, branchPath.end() );
	}
	set( ScenePlug::scenePathContextName, agentPath );
}

bool AtomsCrowdGenerator::isDefaultAtomsMetadataName( const char* attrName ) const
{
	const char *ALL_ATOMS_ATTRS[] = {
	ATOMS_AGENT_STATE,
	ATOMS_AGENT_UP,
	ATOMS_AGENT_POSITION,
	ATOMS_AGENT_DIRECTION,
	ATOMS_AGENT_SCALE,
	ATOMS_AGENT_GROUPID,
	ATOMS_AGENT_ID,
	ATOMS_AGENT_LOCAL_DIRECTION,
	ATOMS_AGENT_GROUPNAME,
	ATOMS_AGENT_SELECTED,
	ATOMS_AGENT_TYPE,
	ATOMS_AGENT_FRAMERATE,
	ATOMS_AGENT_TURN_ANGLE,
	ATOMS_AGENT_VELOCITY,
	ATOMS_AGENT_ANIMATED_HF,
	ATOMS_AGENT_GRAVITY,
	ATOMS_AGENT_CACHE_ID,
	ATOMS_AGENT_BIRTH,
	ATOMS_AGENT_RETARGETING_FACTOR,
	ATOMS_AGENT_COLLECTOR_DIRECTIONS,
	ATOMS_AGENT_CLOTH_SETUP_OVERRIDE,
	ATOMS_AGENT_COLOR,
	ATOMS_AGENT_DISABLE_IK,
	ATOMS_AGENT_USE_CLIP_DIRECTION,
	"_prevState"
	};
	size_t size = sizeof( ALL_ATOMS_ATTRS ) / sizeof( ALL_ATOMS_ATTRS[0] );
	for ( unsigned int i = 0; i < size; ++i )
		if ( !strcmp( ALL_ATOMS_ATTRS[i], attrName ) )
			return true;
	return false;
}

void AtomsCrowdGenerator::poseAttributesHash( const ScenePath &branchPath, IECore::MurmurHash &h) const
{
    if ( !useInstancesPlug()->getValue() )
    {
        h.append( branchPath[3] );
        return;
    }
    auto meshAttributes = runTimeCast<const CompoundObject>( variationsPlug()->attributesPlug()->getValue() );
    if ( meshAttributes )
    {
        auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        auto agentData = crowd->member<const CompoundData>( branchPath[3] );
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No agent found while hashing " + branchPath[3].string());
        }

        auto hashPoseData = agentData->member<const UInt64Data>( "hash" );
        if ( hashPoseData )
        {
            h.append( hashPoseData->readable() );
        }
        else
		{
            h.append( branchPath[3] );
        }

        auto metadataData = agentData->member<const CompoundData>( "metadata" );
        if ( metadataData )
        {
            auto &metadataMap = metadataData->readable();
            for ( auto metaIt = metadataMap.cbegin(); metaIt != metadataMap.cend(); ++metaIt )
            {
                if ( isDefaultAtomsMetadataName( metaIt->first.c_str() ) )
                    continue;
                metaIt->second->hash( h );
            }
        }
    }
    else
    {
        h.append( branchPath[3] );
    }
}
