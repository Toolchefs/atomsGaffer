#ifndef ATOMSGAFFER_ATOMSCROWDREADER_H
#define ATOMSGAFFER_ATOMSCROWDREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/ObjectSource.h"

#include "Gaffer/StringPlug.h"
#include "Gaffer/NumericPlug.h"

namespace AtomsGaffer
{

// \todo: We may replace this node with an AtomsSceneInterface
// and load via the GafferScene::SceneReader. We're not doing
// that yet because we haven't decided what direction to go with
// regards to the various modes.
class AtomsCrowdReader : public GafferScene::ObjectSource
{

	public:

		AtomsCrowdReader( const std::string &name = defaultName<AtomsCrowdReader>() );
		~AtomsCrowdReader() = default;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsCrowdReader, TypeId::AtomsCrowdReaderTypeId, GafferScene::ObjectSource );

		Gaffer::StringPlug *atomsSimFilePlug();
		const Gaffer::StringPlug *atomsSimFilePlug() const;

        Gaffer::StringPlug *agentIdsPlug();
        const Gaffer::StringPlug *agentIdsPlug() const;

		Gaffer::FloatPlug *timeOffsetPlug();
		const Gaffer::FloatPlug *timeOffsetPlug() const;

		Gaffer::IntPlug *refreshCountPlug();
		const Gaffer::IntPlug *refreshCountPlug() const;

		Gaffer::ObjectPlug *enginePlug();
		const Gaffer::ObjectPlug *enginePlug() const;

		void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

	protected:

		void hashSource( const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstObjectPtr computeSource( const Gaffer::Context *context ) const override;

		void hashAttributes( const SceneNode::ScenePath &path, const Gaffer::Context *context,
		        const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		IECore::ConstCompoundObjectPtr computeAttributes( const SceneNode::ScenePath &path,
		        const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

		void hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		void compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const override;

	private:

		IE_CORE_FORWARDDECLARE( EngineData );

		static size_t g_firstPlugIndex;

};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSCROWDREADER_H
