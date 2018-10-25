#include "AtomsGaffer/AtomsAgentReader.h"

#include "IECoreScene/MeshPrimitive.h"

IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsAgentReader );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

size_t AtomsAgentReader::g_firstPlugIndex = 0;

AtomsAgentReader::AtomsAgentReader( const std::string &name )
	:	SceneNode( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsAgentFile" ) );
	addChild( new IntPlug( "refreshCount" ) );
}

StringPlug* AtomsAgentReader::atomsAgentFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsAgentReader::atomsAgentFilePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

IntPlug* AtomsAgentReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const IntPlug* AtomsAgentReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

void AtomsAgentReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneNode::affects( input, outputs );

	if( input == atomsAgentFilePlug() || input == refreshCountPlug() )
	{
		outputs.push_back( outPlug()->boundPlug() );
		outputs.push_back( outPlug()->transformPlug() );
		outputs.push_back( outPlug()->attributesPlug() );
		outputs.push_back( outPlug()->objectPlug() );
		outputs.push_back( outPlug()->childNamesPlug() );
		// deliberately not adding globalsPlug(), since we don't
		// load those from file.
		outputs.push_back( outPlug()->setNamesPlug() );
		outputs.push_back( outPlug()->setPlug() );
	}
}

void AtomsAgentReader::hashBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashBound( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

Imath::Box3f AtomsAgentReader::computeBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	if( path.size() < 3 )
	{
		return unionOfTransformedChildBounds( path, parent );
	}

	return { { -1, -1, -1 }, { 1, 1, 1 } };
}

void AtomsAgentReader::hashTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashTransform( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

Imath::M44f AtomsAgentReader::computeTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	return {};
}

void AtomsAgentReader::hashAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashAttributes( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

IECore::ConstCompoundObjectPtr AtomsAgentReader::computeAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	if( path.size() != 1 )
	{
		return parent->attributesPlug()->defaultValue();;
	}

	CompoundObjectPtr result = new CompoundObject;

	if( path[0] == "SphereBot" )
	{
		result->members()["atoms:agent:type"] = new StringData( "type 0" );
	}
	else
	{
		result->members()["atoms:agent:type"] = new StringData( "type 1" );
	}

	result->members()["atoms:variation:tint"] = new Color3fData( Imath::Color3f( (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX ) );

	return result;
}

void AtomsAgentReader::hashObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashObject( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

IECore::ConstObjectPtr AtomsAgentReader::computeObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	if( path.size() < 3 )
	{
		return parent->objectPlug()->defaultValue();
	}

	if( path[0] == "SphereBot" )
	{
		return MeshPrimitive::createSphere( /* radius */ 1 );
	}
	else
	{
		return MeshPrimitive::createBox( Imath::Box3f( { -1, -1, -1 }, { 1, 1, 1 } ) );
	}
}

void AtomsAgentReader::hashChildNames( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashChildNames( path, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );

	h.append( &path.front(), path.size() );
}

IECore::ConstInternedStringVectorDataPtr AtomsAgentReader::computeChildNames( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.

	InternedStringVectorDataPtr resultData = new InternedStringVectorData;
	std::vector<InternedString> &result = resultData->writable();

	if( path.empty() )
	{
		result.emplace_back( "SphereBot" );
		result.emplace_back( "CubeBot" );
	}
	else if( path.size() == 1 )
	{
		result.emplace_back( "middle" );
	}
	else if( path.size() == 2 )
	{
		result.emplace_back( "leaf" );
	}

	return resultData;
}

void AtomsAgentReader::hashGlobals( const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	h = outPlug()->globalsPlug()->defaultValue()->Object::hash();
}

IECore::ConstCompoundObjectPtr AtomsAgentReader::computeGlobals( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return outPlug()->globalsPlug()->defaultValue();
}

void AtomsAgentReader::hashSetNames( const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashSetNames( context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
}

IECore::ConstInternedStringVectorDataPtr AtomsAgentReader::computeSetNames( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	InternedStringVectorDataPtr resultData = new InternedStringVectorData();
	auto &result = resultData->writable();

	result.emplace_back( "Sphere" );
	result.emplace_back( "Cube" );

	return resultData;
}

void AtomsAgentReader::hashSet( const IECore::InternedString &setName, const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	SceneNode::hashSet( setName, context, parent, h );

	atomsAgentFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
	h.append( setName );
}

IECore::ConstPathMatcherDataPtr AtomsAgentReader::computeSet( const IECore::InternedString &setName, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	// \todo: Implement using Atoms API. See GafferScene::SceneReader for hints.
	PathMatcherDataPtr resultData = new PathMatcherData;
	PathMatcher &result = resultData->writable();

	if( setName == "Sphere" )
	{
		result.addPath( { "SphereBot", "middle", "leaf" } );
	}
	else
	{
		result.addPath( { "CubeBot", "middle", "leaf" } );
	}

	return resultData;
}
