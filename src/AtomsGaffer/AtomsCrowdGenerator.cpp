#include <AtomsUtils/Logger.h>
#include "AtomsGaffer/AtomsCrowdGenerator.h"

#include "IECoreScene/PointsPrimitive.h"

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
		input == attributesPlug() ||
		input == agentsPlug()->transformPlug()
	)
	{
		outputs.push_back( outPlug()->transformPlug() );
	}

	if(
		input == agentsPlug()->attributesPlug() ||
		input == inPlug()->objectPlug() ||
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
    AtomsUtils::Logger::info() << "compute";
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
		ConstInternedStringVectorDataPtr agentNames = agentsPlug()->childNames( ScenePath() );
		std::vector<std::vector<InternedString> *> indexedAgentChildNames;

		CompoundDataPtr result = new CompoundData;
		for( const auto &instanceName : agentNames->readable() )
		{
			InternedStringVectorDataPtr instanceChildNames = new InternedStringVectorData;
			result->writable()[instanceName] = instanceChildNames;
			indexedAgentChildNames.push_back( &instanceChildNames->writable() );
			AtomsUtils::Logger::info() << "agent name " << instanceName;
		}

		ConstPointsPrimitivePtr crowd = runTimeCast<const PointsPrimitive>( inPlug()->objectPlug()->getValue() );
		if( !crowd )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a Compound Object." );
		}

		const auto agentType = crowd->variables.find( "agentType" );
		if( agentType == crowd->variables.end() )
		{
			throw InvalidArgumentException( "AtomsCrowdGenerator : Input crowd must be a PointsPrimitive containing an \"agentType\" vertex variable" );
		}

		auto agentTypesData = runTimeCast<const StringVectorData>(agentType->second.data);
		auto& agentTypeVec = agentTypesData->readable();
		for( size_t i = 0, e = crowd->getNumPoints(); i < e; ++i )
		{
			//size_t agentIndex = indices[i] % indexedAgentChildNames.size();
			//indexedAgentChildNames[agentIndex]->push_back( InternedString( i ) );
		}

		static_cast<AtomicCompoundDataPlug *>( output )->setValue( result );
		return;
	}

	BranchCreator::compute( output, context );
}

void AtomsCrowdGenerator::hashBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() < 2 )
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
	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentName>"
		BranchCreator::hashBranchBound( parentPath, branchPath, context, h );

		inPlug()->objectPlug()->hash( h );
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
		h = agentsPlug()->boundPlug()->hash();
	}
}
Imath::Box3f AtomsCrowdGenerator::computeBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
    AtomsUtils::Logger::info() << "computeBranchBound";
	if( branchPath.size() < 2 )
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
	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentName>"
		AgentScope scope( context, branchPath );
		// \todo: Implement using Atoms API instead if possible. Otherwise optimize as in GafferScene::Instancer.
		return unionOfTransformedChildBounds( branchPath, outPlug() );
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		return agentsPlug()->boundPlug()->getValue();
	}
}

void AtomsCrowdGenerator::hashBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		BranchCreator::hashBranchTransform( parentPath, branchPath, context, h );
	}
	else if( branchPath.size() == 3 )
	{
		// "/agents/<agentName>/<id>"
		BranchCreator::hashBranchTransform( parentPath, branchPath, context, h );
		{
			AgentScope instanceScope( context, branchPath );
			agentsPlug()->transformPlug()->hash( h );
		}
		inPlug()->objectPlug()->hash( h );
		attributesPlug()->hash( h );
		h.append( branchPath[2] );
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		h = agentsPlug()->transformPlug()->hash();
	}
}
Imath::M44f AtomsCrowdGenerator::computeBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
    AtomsUtils::Logger::info() << "computeBranchTransform";
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return Imath::M44f();
	}
	else if( branchPath.size() == 3 )
	{
		// "/agents/<agentName>/<id>"
		Imath::M44f result;
		{
			AgentScope scope( context, branchPath );
			result = agentsPlug()->transformPlug()->getValue();
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
}

void AtomsCrowdGenerator::hashBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		h = outPlug()->attributesPlug()->defaultValue()->Object::hash();
	}
	else if( branchPath.size() == 3 )
	{
		// "/agents/<agentName>/<id>"
		BranchCreator::hashBranchAttributes( parentPath, branchPath, context, h );
		{
			{
				AgentScope scope( context, branchPath );
				agentsPlug()->attributesPlug()->hash( h );
			}

			// \todo: Hash promoted attributes. See GafferScene::Instancer::EngineData for hints.
		}
	}
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope instanceScope( context, branchPath );
		h = agentsPlug()->attributesPlug()->hash();
	}
}
ConstCompoundObjectPtr AtomsCrowdGenerator::computeBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
    AtomsUtils::Logger::info() << "computeBranchAttributes";
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->attributesPlug()->defaultValue();
	}
	else if( branchPath.size() == 3 )
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
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		h = outPlug()->objectPlug()->defaultValue()->Object::hash();
	}
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope instanceScope( context, branchPath );
		h = agentsPlug()->objectPlug()->hash();
	}
}

ConstObjectPtr AtomsCrowdGenerator::computeBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const
{
    AtomsUtils::Logger::info() << "computeBranchObject";
	if( branchPath.size() <= 2 )
	{
		// "/" or "/agents" or "/agents/<agentName>"
		return outPlug()->objectPlug()->defaultValue();
	}
	else
	{
		// "/agents/<agentName>/<id>/...
		AgentScope scope( context, branchPath );
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
		h = agentsPlug()->childNamesHash( ScenePath() );
	}
	else if( branchPath.size() == 2 )
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
    AtomsUtils::Logger::info() << "computeBranchChildNames";
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
		return agentsPlug()->childNames( ScenePath() );
	}
	else if( branchPath.size() == 2 )
	{
		// "/agents/<agentName>"
		IECore::ConstCompoundDataPtr children = agentChildNames( parentPath, context );
		AtomsUtils::Logger::info() << "computeBranchChildNames Child names" << children << " " << branchPath.back();
		return children->member<InternedStringVectorData>( branchPath.back() );
	}
	else
	{
		// "/agents/<agentName>/<id>/..."
		AgentScope scope( context, branchPath );
		return agentsPlug()->childNamesPlug()->getValue();
	}
}

void AtomsCrowdGenerator::hashBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context, MurmurHash &h ) const
{
	h = agentsPlug()->setNamesPlug()->hash();
}
ConstInternedStringVectorDataPtr AtomsCrowdGenerator::computeBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context ) const
{
	return agentsPlug()->setNamesPlug()->getValue();
}

void AtomsCrowdGenerator::hashBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context, MurmurHash &h ) const
{
	BranchCreator::hashBranchSet( parentPath, setName, context, h );

	h.append( agentsPlug()->childNamesHash( ScenePath() ) );
	agentChildNamesHash( parentPath, context, h );
	agentsPlug()->setPlug()->hash( h );
	namePlug()->hash( h );
}
ConstPathMatcherDataPtr AtomsCrowdGenerator::computeBranchSet( const ScenePath &parentPath, const InternedString &setName, const Gaffer::Context *context ) const
{
    AtomsUtils::Logger::info() << "computeBranchSet";
	ConstInternedStringVectorDataPtr agentNames = agentsPlug()->childNames( ScenePath() );
	if (agentNames->readable().size() == 0)
	{
		AtomsUtils::Logger::info() << "Reading default agent type data";
	}

	IECore::ConstCompoundDataPtr instanceChildNames = agentChildNames( parentPath, context );
	ConstPathMatcherDataPtr inputSet = agentsPlug()->setPlug()->getValue();

	PathMatcherDataPtr outputSetData = new PathMatcherData;
	PathMatcher &outputSet = outputSetData->writable();

	std::vector<InternedString> branchPath( { namePlug()->getValue() } );
	std::vector<InternedString> agentPath( 1 );

	for( const auto &agentName : agentNames->readable() )
	{
		branchPath.resize( 2 );
		branchPath.back() = agentName;
		agentPath.back() = agentName;

		PathMatcher instanceSet = inputSet->readable().subTree( agentPath );

		const std::vector<InternedString> &childNames = instanceChildNames->member<InternedStringVectorData>( agentName )->readable();

		branchPath.emplace_back( InternedString() );
		for( const auto &instanceChildName : childNames )
		{
			branchPath.back() = instanceChildName;
			outputSet.addPaths( instanceSet, branchPath );
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
	assert( branchPath.size() >= 2 );
	ScenePath agentPath;
	agentPath.reserve( 1 + ( branchPath.size() > 3 ? branchPath.size() - 3 : 0 ) );
	agentPath.push_back( branchPath[1] );
	if( branchPath.size() > 3 )
	{
		agentPath.insert( agentPath.end(), branchPath.begin() + 3, branchPath.end() );
	}
	set( ScenePlug::scenePathContextName, agentPath );
}
