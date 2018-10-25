#include "AtomsGaffer/AtomsCrowdReader.h"

#include "IECoreScene/PointsPrimitive.h"

IE_CORE_DEFINERUNTIMETYPED( AtomsGaffer::AtomsCrowdReader );

using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace AtomsGaffer;

size_t AtomsCrowdReader::g_firstPlugIndex = 0;

AtomsCrowdReader::AtomsCrowdReader( const std::string &name )
	:	ObjectSource( name, "crowd" )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "atomsSimFile" ) );
	addChild( new IntPlug( "refreshCount" ) );
}

StringPlug* AtomsCrowdReader::atomsSimFilePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const StringPlug* AtomsCrowdReader::atomsSimFilePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

IntPlug* AtomsCrowdReader::refreshCountPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const IntPlug* AtomsCrowdReader::refreshCountPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

void AtomsCrowdReader::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	ObjectSource::affects( input, outputs );

	if( input == atomsSimFilePlug() || input == refreshCountPlug() )
	{
		outputs.push_back( sourcePlug() );
	}
}

void AtomsCrowdReader::hashSource( const Gaffer::Context *context, MurmurHash &h ) const
{
	atomsSimFilePlug()->hash( h );
	refreshCountPlug()->hash( h );
}

ConstObjectPtr AtomsCrowdReader::computeSource( const Gaffer::Context *context ) const
{
	std::string atomsSimFile = atomsSimFilePlug()->getValue();

	// \todo: actually read the file and extract the necessary data

	size_t numAgents = 10;
	size_t numAgentTypes = 3;

	V3fVectorDataPtr positionData = new V3fVectorData;
	positionData->setInterpretation( IECore::GeometricData::Interpretation::Point );
	auto &positions = positionData->writable();
	positions.reserve( numAgents );

	StringVectorDataPtr agentTypeData = new StringVectorData;
	auto &agentTypes = agentTypeData->writable();
	agentTypes.reserve( numAgentTypes );
	for( size_t i = 0; i < numAgentTypes; ++i )
	{
		agentTypes.push_back( ( boost::format( "type %d" ) % i ).str() );
	}

	IntVectorDataPtr agentTypeIndexData = new IntVectorData;
	auto &agentTypeIndices = agentTypeIndexData->writable();
	agentTypeIndices.reserve( numAgents );

	for( size_t i = 0; i < numAgents; ++i )
	{
		positions.emplace_back( i, 0, 0 );
		agentTypeIndices.push_back( i % 3 );
	}

	PointsPrimitivePtr points = new PointsPrimitive( positionData );
	points->variables["agentType"] = PrimitiveVariable( PrimitiveVariable::Vertex, agentTypeData, agentTypeIndexData );

	return points;
}
