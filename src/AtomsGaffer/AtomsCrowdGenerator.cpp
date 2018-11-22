#include <AtomsUtils/Logger.h>
#include "AtomsGaffer/AtomsCrowdGenerator.h"

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
	addChild( new ScenePlug( "agents" ) );
	addChild( new StringPlug( "attributes", Plug::In ) );
	addChild( new IntPlug( "mode" ) );

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

ScenePlug *AtomsCrowdGenerator::agentsPlug()
{
	return getChild<ScenePlug>( g_firstPlugIndex + 1 );
}

const ScenePlug *AtomsCrowdGenerator::agentsPlug() const
{
	return getChild<ScenePlug>( g_firstPlugIndex + 1 );
}

Gaffer::StringPlug *AtomsCrowdGenerator::attributesPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::StringPlug *AtomsCrowdGenerator::attributesPlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex + 2 );
}

Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug()
{
	return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 4);
}

const Gaffer::AtomicCompoundDataPlug *AtomsCrowdGenerator::agentChildNamesPlug() const
{
	return getChild<AtomicCompoundDataPlug>( g_firstPlugIndex + 4 );
}

Gaffer::IntPlug *AtomsCrowdGenerator::modePlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::IntPlug *AtomsCrowdGenerator::modePlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

void AtomsCrowdGenerator::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	BranchCreator::affects( input, outputs );
	if(
		input == inPlug()->objectPlug() ||
		input == attributesPlug() ||
		input == agentsPlug()->childNamesPlug()
	)
	{
		outputs.push_back( agentChildNamesPlug() );
	}

	if(
		input == namePlug() ||
		input == agentChildNamesPlug() ||
		input == agentsPlug()->childNamesPlug()
	)
	{
		outputs.push_back( outPlug()->childNamesPlug() );
	}

	if(
		input == inPlug()->objectPlug() ||
		input == attributesPlug() ||
		input == namePlug() ||
		input == agentsPlug()->boundPlug() ||
		input == agentsPlug()->transformPlug() ||
		input == agentChildNamesPlug()
	)
	{
		outputs.push_back( outPlug()->boundPlug() );
	}

	if(
		input == inPlug()->objectPlug() ||
		input == inPlug()->attributesPlug() ||
		input == attributesPlug() ||
		input == agentsPlug()->transformPlug()
	)
	{
		outputs.push_back( outPlug()->transformPlug() );
	}

	if(
		input == agentsPlug()->attributesPlug() ||
		input == inPlug()->objectPlug() ||
		input == inPlug()->attributesPlug() ||
		input == attributesPlug()
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
		h.append( agentsPlug()->childNamesHash( ScenePath() ) );
		modePlug()->hash( h );
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


        const auto agentId = crowd->variables.find( "agentId");
        if( agentId == crowd->variables.end() )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"agentId\" vertex variable" );
        }

        auto agentIdData = runTimeCast<const IntVectorData>(agentId->second.data);
        if (!agentIdData)
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"agentId\" vertex variable" );
        }
        const std::vector<int>& agentIdVec = agentIdData->readable();


        const std::vector<std::string>* agentTypeVec = nullptr;
        const std::vector<std::string>* agentVariationVec = nullptr;
        const std::vector<std::string>* agentLodVec = nullptr;
        std::string agentTypeDefault = "";
        std::string variationDefault = "";
        std::string lodDefault = "";

		const auto agentType = crowd->variables.find( "agentType" );
		if( agentType == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"agentType\" vertex variable" );
		}

        if ( agentType->second.interpolation == PrimitiveVariable::Vertex ) {
            auto agentTypesData = runTimeCast<const StringVectorData>(agentType->second.data);
            if (agentTypesData)
                agentTypeVec = &agentTypesData->readable();
        }else if (agentType->second.interpolation == PrimitiveVariable::Constant) {
            auto agentTypesData = runTimeCast<const StringData>(agentType->second.data);
            if (agentTypesData)
                agentTypeDefault = agentTypesData->readable();
        }

		const auto agentVariation = crowd->variables.find( "variation");
		if( agentVariation == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"variation\" vertex variable" );
		}

        if ( agentVariation->second.interpolation == PrimitiveVariable::Vertex ) {
            auto agentVariationData = runTimeCast<const StringVectorData>(agentVariation->second.data);
            if (agentVariationData)
                agentVariationVec = &agentVariationData->readable();
        }else if (agentVariation->second.interpolation == PrimitiveVariable::Constant) {
            auto agentVariationData = runTimeCast<const StringData>(agentVariation->second.data);
            if (agentVariationData)
                variationDefault = agentVariationData->readable();
        }


		const auto agentLod = crowd->variables.find( "lod");
		if( agentLod == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"lod\" vertex variable" );
		}

		if ( agentLod->second.interpolation == PrimitiveVariable::Vertex ) {
            auto agentLodData = runTimeCast<const StringVectorData>(agentLod->second.data);
            if (agentLodData)
                agentLodVec = &agentLodData->readable();
        }else if (agentLod->second.interpolation == PrimitiveVariable::Constant) {
            auto agentLodData = runTimeCast<const StringData>(agentLod->second.data);
            if (agentLodData)
                lodDefault = agentLodData->readable();
        }


		std::map<std::string, std::map<std::string, std::vector<int>>> agentVariationMap;

        for( size_t agId = 0; agId < agentIdVec.size(); ++agId )
        {
            std::string agentTypeName = agentTypeDefault;
            if (agentTypeVec)
                agentTypeName = (*agentTypeVec)[agId];

            std::string variationName = variationDefault;
            if (agentVariationVec)
                variationName = (*agentVariationVec)[agId];

            if (variationName == "")
                continue;

            std::string lodName = lodDefault;
            if (agentLodVec)
                lodName = (*agentLodVec)[agId];

        	if ( lodName != "" ) {
				variationName = variationName + ':' + lodName;
			}

			agentVariationMap[agentTypeName][variationName].push_back(agentIdVec[agId]);
        }

        CompoundDataPtr result = new CompoundData;
        for( auto typeIt = agentVariationMap.begin(); typeIt != agentVariationMap.end(); ++typeIt ) {
            CompoundDataPtr variationResult = new CompoundData;
            for( auto varIt = typeIt->second.begin(); varIt != typeIt->second.end(); ++varIt )
            {
                InternedStringVectorDataPtr idsData = new InternedStringVectorData;
                auto& idsStrVec = idsData->writable();
                idsStrVec.reserve(varIt->second.size());
                for(const auto agId: varIt->second)
                {
                    idsStrVec.push_back(std::to_string(agId));
                }

                variationResult->writable()[varIt->first] = idsData;
            }

            result->writable()[typeIt->first] = variationResult;
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
		if( branchPath.size() == 0 )
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
		attributesPlug()->hash( h );
		agentChildNamesHash( parentPath, context, h );
		h.append( branchPath.back() );
		{
			AgentScope scope( context, branchPath );
			agentsPlug()->transformPlug()->hash( h );
			agentsPlug()->boundPlug()->hash( h );
		}
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope instanceScope( context, branchPath );
		agentsPlug()->boundPlug()->hash(h);
        inPlug()->boundPlug()->hash(h);
        inPlug()->attributesPlug()->hash(h);
        //h.append(branchPath[3]);
        //h.append( context->getFrame() );
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
	else if ( branchPath.size() == 4 )
    {
        auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        auto agentData = crowd->member<const CompoundData>(branchPath.back());
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No agent found." );
        }

        auto boxData = agentData->member<const Box3dData>("boundingBox");
        auto rootMatrixData = agentData->member<const M44dData>("rootMatrix");
        Imath::Box3f result;
        if (boxData && rootMatrixData)
        {
            auto agentBox = boxData->readable();
            auto rootMtx = rootMatrixData->readable();

            rootMtx.invert();

            agentBox.min = agentBox.min * rootMtx;
            agentBox.max = agentBox.max * rootMtx;

            result.extendBy(agentBox.min);
            result.extendBy(agentBox.max);
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No boundingBox or rootMatrix data found." );
        }
        return result;
    }
	/*
	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentName>"
		AgentScope scope( context, branchPath );
		// \todo: Implement using Atoms API instead if possible. Otherwise optimize as in GafferScene::Instancer.
		return unionOfTransformedChildBounds( branchPath, outPlug() );
	}
	 */
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		return agentsPlug()->boundPlug()->getValue();
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
		/*
		BranchCreator::hashBranchTransform( parentPath, branchPath, context, h );
		{
			AgentScope instanceScope( context, branchPath );
			agentsPlug()->transformPlug()->hash( h );
		}
		 */
		inPlug()->objectPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
		attributesPlug()->hash( h );
		h.append( branchPath[3] );
	}

	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		agentsPlug()->transformPlug()->hash(h);
		inPlug()->attributesPlug()->hash(h);
        h.append(branchPath[3]);
        //h.append( context->getFrame() );
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

        // todo: check why the object is null while the attributes are valid
        //AtomsUtils::Logger::info() << inPlug()->objectPlug()->getValue()->typeName();
        //AtomsUtils::Logger::info() << inPlug()->attributesPlug()->getValue()->typeName();

        auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
        if( !crowd )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
        }

        auto agentData = crowd->member<const CompoundData>(branchPath.back());
        if( !agentData )
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No agent found." );
        }

        auto poseData = agentData->member<const M44dData>("rootMatrix");
        if (poseData)
        {

            result = Imath::M44f(poseData->readable());
        }
        else
        {
            throw InvalidArgumentException( "AtomsCrowdGenerator : No rootMatrix data found." );
        }

		// \todo: Implement using Atoms API. See GafferScene::Instancer::EngineData for hints.
		return result;
	}

	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		return agentsPlug()->transformPlug()->getValue();
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
            agentsPlug()->attributesPlug()->hash( h );
        }
        // \todo: Hash promoted attributes. See GafferScene::Instancer::EngineData for hints.
    }
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope instanceScope( context, branchPath );
		agentsPlug()->attributesPlug()->hash( h );
	}

}
ConstCompoundObjectPtr AtomsCrowdGenerator::computeBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
	if( branchPath.size() < 4 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->attributesPlug()->defaultValue();
        //ConstCompoundObjectPtr baseAttributes;
        //return baseAttributes;
	}

	else if( branchPath.size() == 4 )
	{
		// "/agents/<agentName>/<id>"
		ConstCompoundObjectPtr baseAttributes;
		{
			AgentScope instanceScope( context, branchPath );
			baseAttributes = agentsPlug()->attributesPlug()->getValue();
		}

		// \todo: Append promoted attributes. See GafferScene::Instancer::EngineData for hints.

		return baseAttributes;
	}

	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
		return agentsPlug()->attributesPlug()->getValue();
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
        AgentScope instanceScope(context, branchPath);
        agentsPlug()->objectPlug()->hash( h );
        inPlug()->attributesPlug()->hash( h );
        // todo: use the hash from the pose
        h.append(branchPath[3]);
        //h.append( context->getFrame() );

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
		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
		//AtomsUtils::Logger::info() << agentsPlug()->objectPlug()->getValue()->typeName();
		auto meshPrim = runTimeCast<const MeshPrimitive>(agentsPlug()->objectPlug()->getValue());
        auto meshAttributes = runTimeCast<const CompoundObject>(agentsPlug()->attributesPlug()->getValue());
		if (meshPrim && meshAttributes)
        {
            auto crowd = runTimeCast<const CompoundObject>( inPlug()->attributesPlug()->getValue() );
            if( !crowd )
            {
                throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
            }


            auto agentData = crowd->member<const CompoundData>(branchPath[3]);
            if( !agentData )
            {
                throw InvalidArgumentException( "AtomsCrowdGenerator : No agent found." );
            }

            auto metadataData = agentData->member<const CompoundData>("metadata");
            auto poseData = agentData->member<const M44dVectorData>("poseWorldMatrices");
            auto poseNormalData = agentData->member<const M44dVectorData>("poseNormalWorldMatrices");
            if (poseData && poseData->readable().size() > 0)
            {
                auto& worldMatrices = poseData->readable();
                auto& worldNormalMatrices = poseNormalData->readable();




                MeshPrimitivePtr result = meshPrim->copy();
				auto meshPointData = runTimeCast<V3fVectorData>(result->variables["P"].data);
				auto meshNormalData = runTimeCast<V3fVectorData>(result->variables["N"].data);
                auto blendShapeCountDataIt = result->variables.find("blendShapeCount");

                if (blendShapeCountDataIt != result->variables.end() && metadataData && meshPointData && meshNormalData)
                {
					auto &points = meshPointData->writable();
					auto &normals = meshNormalData->writable();
                	auto& metadataMap = metadataData->readable();
                    auto blendShapeCountData = runTimeCast<IntData>(blendShapeCountDataIt->second.data);
                    int numBlendShapes = blendShapeCountData->readable();

                    std::vector<const std::vector<Imath::V3f>*> blendPoints;
					std::vector<const std::vector<Imath::V3f>*> blendNormals;
					std::vector<double> blendWeights;
                    for (size_t blendId=0; blendId < numBlendShapes; ++blendId)
					{
						auto blendWeightDataIt = metadataMap.find(std::string(branchPath[1].c_str()) + "_" + std::string(branchPath.back().c_str()) +"_" + std::to_string(blendId));
						if (blendWeightDataIt == metadataMap.cend())
							continue;

						auto blendWeightData = runTimeCast<const DoubleData>(blendWeightDataIt->second);
						if (!blendWeightData)
							continue;

						double weight = blendWeightData->readable();

						if (weight < 0.00001)
						    continue;

						auto blendPointsData = runTimeCast<V3fVectorData>(result->variables["blendShape_" + std::to_string(blendId) + "_P"].data);
						if (!blendPointsData)
							continue;

						auto blendNormalsData = runTimeCast<V3fVectorData>(result->variables["blendShape_" + std::to_string(blendId) + "_N"].data);
						if (!blendNormalsData)
							continue;

						auto& blendPointsVec = blendPointsData->readable();

						if (blendPointsVec.size() != points.size())
							continue;


						auto& blendNormalsVec = blendNormalsData->readable();

						if (blendNormalsVec.size() != normals.size())
							continue;

                        blendPoints.push_back(&blendPointsVec);
						blendNormals.push_back(&blendNormalsVec);
						blendWeights.push_back(weight);
					}

					for (size_t ptId=0; ptId < points.size(); ++ptId)
                    {
					    Imath::V3f currentPoint;
					    auto& meshPoint = points[ptId];
                        currentPoint.x = meshPoint.x;
                        currentPoint.y = meshPoint.y;
                        currentPoint.z = meshPoint.z;


                        for (size_t blendId = 0; blendId < blendPoints.size(); blendId++)
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

					for (size_t ptId=0; ptId < points.size(); ++ptId) {
						Imath::V3f currentNormal;
						auto &meshNormal = normals[ptId];
						currentNormal.x = meshNormal.x;
						currentNormal.y = meshNormal.y;
						currentNormal.z = meshNormal.z;

						Imath::V3f pNormals(currentNormal.x, currentNormal.y, currentNormal.z);
						Imath::V3f  blendNormal(0.0, 0.0, 0.0);
						size_t counterN = 0;

						for (size_t blendId = 0; blendId < blendNormals.size(); blendId++)
						{
							auto &blendN = *blendNormals[blendId];
							double weight = blendWeights[blendId];
							blendNormal += pNormals * (1.0f - weight) +
										   blendN[ptId] * weight;
							counterN++;
						}
						if (counterN > 0) {
							blendNormal = blendNormal / static_cast<float>(counterN);
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


                auto jointIndexCountData = meshAttributes->member<const IntVectorData>("jointIndexCount");
                auto jointIndicesData = meshAttributes->member<const IntVectorData>("jointIndices");
                auto jointWeightsData = meshAttributes->member<const FloatVectorData>("jointWeights");
                auto vertexIdsData = meshPrim->vertexIds();
                if (jointIndexCountData && jointIndicesData && jointWeightsData && vertexIdsData) {

                    auto& vertexIds = vertexIdsData->readable();

                    auto &jointIndexCountVec = jointIndexCountData->readable();

                    auto &jointIndicesVec = jointIndicesData->readable();

                    auto &jointWeightsVec = jointWeightsData->readable();
                    //AtomsUtils::Logger::info() << "attributes " <<jointIndexCountData <<  " " << jointIndicesData << " " << jointWeightsData;
                    //AtomsUtils::Logger::info() << jointIndexCountVec.size() << " "<< points.size();
                    std::vector<size_t> offsetData(jointIndexCountVec.size());
                    size_t counter = 0;
                    auto pData = runTimeCast<V3fVectorData>(result->variables["P"].data);
                    if (pData) {
                        auto &points = pData->writable();
                        for (size_t vId = 0; vId < jointIndexCountVec.size(); ++vId) {
                            offsetData[vId] = counter;
                            Imath::V4d newP, currentPoint;
                            newP.x = 0.0;
                            newP.y = 0.0;
                            newP.z = 0.0;
                            newP.w = 1.0;

                            currentPoint.x = points[vId].x;
                            currentPoint.y = points[vId].y;
                            currentPoint.z = points[vId].z;
                            currentPoint.w = 1.0;

                            for (size_t jId = 0; jId < jointIndexCountVec[vId]; ++jId, ++counter) {
                                int jointId = jointIndicesVec[counter];
                                newP += currentPoint * jointWeightsVec[counter] * worldMatrices[jointId];
                            }

                            points[vId].x = newP.x;
                            points[vId].y = newP.y;
                            points[vId].z = newP.z;
                        }
                    }
                    else
                    {
                        AtomsUtils::Logger::warning() << "No points found";
                    }


                    auto nData = runTimeCast<V3fVectorData>(result->variables["N"].data);
                    if (nData) {
                        auto &normals = nData->writable();
                        for (size_t vtxId = 0; vtxId < vertexIds.size(); ++vtxId) {
                            int pointId = vertexIds[vtxId];
                            size_t offset = offsetData[pointId];

                            Imath::V4d newP, currentPoint;
                            newP.x = 0.0;
                            newP.y = 0.0;
                            newP.z = 0.0;
                            newP.w = 0.0;

                            currentPoint.x = normals[vtxId].x;
                            currentPoint.y = normals[vtxId].y;
                            currentPoint.z = normals[vtxId].z;
                            currentPoint.w = 0.0;

                            for (size_t jId = 0; jId < jointIndexCountVec[pointId]; ++jId) {
                                int jointId = jointIndicesVec[offset];
                                newP += currentPoint * jointWeightsVec[offset] * worldMatrices[jointId];
                            }

                            normals[vtxId].x = newP.x;
                            normals[vtxId].y = newP.y;
                            normals[vtxId].z = newP.z;
                            normals[vtxId].normalize();
                        }
                    }
                    else
                    {
                        AtomsUtils::Logger::warning() << "No normals found";
                    }
                }

				// add metadata to the mesh prim
                auto metaDataPtr = agentData->member<const CompoundData>("metadata");
                if (metaDataPtr)
                {
                    auto metaData = metaDataPtr->readable();
                    for (auto metaIt = metaData.cbegin(); metaIt != metaData.cend(); ++metaIt)
                    {
                        if (metaIt->first == "id")
                            continue;
                        if (metaIt->second->typeId() == V3dData::staticTypeId())
                        {
                            V3fDataPtr vec3fData = new V3fData();
                            auto vec3dData = runTimeCast<const V3dData>(metaIt->second);
                            vec3fData->writable() = vec3dData->readable();
                            continue;
                        }
                        if (metaIt->second->typeId() == DoubleData::staticTypeId())
                        {
                            FloatDataPtr fData = new FloatData();
                            auto dData = runTimeCast<const DoubleData>(metaIt->second);
                            fData->writable() = dData->readable();
                            continue;
                        }

                        result->variables[metaIt->first] = PrimitiveVariable( PrimitiveVariable::Constant, metaIt->second);
                    }
                }
                return result;
            }
        }





		return agentsPlug()->objectPlug()->getValue();
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
		//h = agentsPlug()->childNamesHash( ScenePath() );
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
		h = agentsPlug()->childNamesPlug()->hash();
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
		//return agentsPlug()->childNames( ScenePath() );
        IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
        auto& varData = children->readable();
        InternedStringVectorDataPtr result = new InternedStringVectorData();
        auto& names = result->writable();
        for (auto it = varData.cbegin(); it != varData.cend(); ++it)
        {

            names.push_back(it->first);
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
        if (variationsData)
        {
            auto& varData = variationsData->readable();
            for (auto it = varData.cbegin(); it != varData.cend(); ++it)
            {
                names.push_back(it->first);
            }
        }
        return result;
		//return children->member<InternedStringVectorData>( branchPath.back() );
	}
    else if( branchPath.size() == 3 )
    {
        IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
        auto variationsData = children->member<CompoundData>(branchPath[1]);
        InternedStringVectorDataPtr result = new InternedStringVectorData();
        if (variationsData)
        {
            return variationsData->member<InternedStringVectorData>(branchPath.back());

        }
        return result;
    }

	else
	{
		// "/agents/<agentType>/<variation>/<id>/..."
		AgentScope scope( context, branchPath );
		return agentsPlug()->childNamesPlug()->getValue();
	}

}

void AtomsCrowdGenerator::hashBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	h = agentsPlug()->setNamesPlug()->hash();
    //BranchCreator::hashBranchSetNames( parentPath, context, h );
}
ConstInternedStringVectorDataPtr AtomsCrowdGenerator::computeBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context ) const
{
	return agentsPlug()->setNamesPlug()->getValue();
	//return BranchCreator::computeBranchSetNames( parentPath, context );
}

void AtomsCrowdGenerator::hashBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context, MurmurHash &h ) const
{
	BranchCreator::hashBranchSet( parentPath, setName, context, h );

	h.append( agentsPlug()->childNamesHash( ScenePath() ) );
	agentChildNamesHash( parentPath, context, h );
	agentsPlug()->setPlug()->hash( h );
	namePlug()->hash( h );
}

std::string printPath(const std::vector<InternedString>& path)
{
	std::string ss;
	for(auto& p: path)
	{
		ss += std::string("/") + p.string();
	}
	return ss;
}

ConstPathMatcherDataPtr AtomsCrowdGenerator::computeBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context ) const
{
	ConstInternedStringVectorDataPtr agentNames = agentsPlug()->childNames( ScenePath() );
	IECore::ConstCompoundDataPtr instanceChildNames = agentChildNames( parentPath, context );
	ConstPathMatcherDataPtr inputSet = agentsPlug()->setPlug()->getValue();

	PathMatcherDataPtr outputSetData = new PathMatcherData;
	PathMatcher &outputSet = outputSetData->writable();

	std::vector<InternedString> branchPath( { namePlug()->getValue() } );
	std::vector<InternedString> agentPath( 1 );



	for( const auto &agentName : agentNames->readable() )
	{
		agentPath.back() = agentName;

        std::vector<std::string> paths;
        inputSet->readable().paths(paths);

		PathMatcher instanceSet = inputSet->readable().subTree( agentPath );
		auto variationNamesData = instanceChildNames->member<CompoundData>( agentName );
		if (!variationNamesData)
			continue;

		auto variationNames = variationNamesData->readable();

		for(auto it = variationNames.cbegin(); it != variationNames.cend(); ++it)
		{
			branchPath.resize( 2 );
			branchPath.back() = agentName;
            PathMatcher variationInstanceSet = instanceSet.subTree( it->first );


			auto childNamesData = runTimeCast<InternedStringVectorData>(it->second);
			if (!childNamesData)
				continue;

			const std::vector<InternedString> &childNames = childNamesData->readable();
			branchPath.emplace_back(InternedString());
			branchPath.back() = it->first;
			branchPath.emplace_back(InternedString());
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
