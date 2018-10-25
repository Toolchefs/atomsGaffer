#ifndef ATOMSGAFFER_ATOMSAGENTREADER_H
#define ATOMSGAFFER_ATOMSAGENTREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneNode.h"

#include "Gaffer/StringPlug.h"

namespace AtomsGaffer
{

// \todo: We may replace this node with an AtomsSceneInterface
// and load via the GafferScene::SceneReader. We're not doing
// that yet because I'm not familiar enough with the various
// file formats that Atoms provides.
class AtomsAgentReader : public GafferScene::SceneNode
{

	public:

		AtomsAgentReader( const std::string &name = defaultName<AtomsAgentReader>() );
		~AtomsAgentReader() = default;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsAgentReader, TypeId::AtomsAgentReaderTypeId, GafferScene::SceneNode );

		Gaffer::StringPlug *atomsAgentFilePlug();
		const Gaffer::StringPlug *atomsAgentFilePlug() const;

		Gaffer::IntPlug *refreshCountPlug();
		const Gaffer::IntPlug *refreshCountPlug() const;

		void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

	protected:

		void hashBound( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashTransform( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashAttributes( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashChildNames( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashGlobals( const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashSetNames( const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
		void hashSet( const IECore::InternedString &setName, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;

		Imath::Box3f computeBound( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		Imath::M44f computeTransform( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstCompoundObjectPtr computeAttributes( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstInternedStringVectorDataPtr computeChildNames( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstCompoundObjectPtr computeGlobals( const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstInternedStringVectorDataPtr computeSetNames( const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstPathMatcherDataPtr computeSet( const IECore::InternedString &setName, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

	private :

		static size_t g_firstPlugIndex;

};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSAGENTREADER_H
