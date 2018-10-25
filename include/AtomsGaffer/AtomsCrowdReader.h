#ifndef ATOMSGAFFER_ATOMSCROWDREADER_H
#define ATOMSGAFFER_ATOMSCROWDREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/ObjectSource.h"

#include "Gaffer/StringPlug.h"

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

		Gaffer::IntPlug *refreshCountPlug();
		const Gaffer::IntPlug *refreshCountPlug() const;

		void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

	protected:

		void hashSource( const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstObjectPtr computeSource( const Gaffer::Context *context ) const override;

	private :

		static size_t g_firstPlugIndex;

};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSCROWDREADER_H
