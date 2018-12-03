#ifndef ATOMSGAFFER_ATOMSCROWDGENERATOR_H
#define ATOMSGAFFER_ATOMSCROWDGENERATOR_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/BranchCreator.h"

#include "Gaffer/PlugType.h"
#include "Gaffer/StringPlug.h"

namespace AtomsGaffer
{

class AtomsCrowdGenerator : public GafferScene::BranchCreator
{

	public:

		AtomsCrowdGenerator( const std::string &name = defaultName<AtomsCrowdGenerator>() );
		~AtomsCrowdGenerator() = default;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsCrowdGenerator, TypeId::AtomsCrowdGeneratorTypeId, GafferScene::BranchCreator );

		Gaffer::StringPlug *namePlug();
		const Gaffer::StringPlug *namePlug() const;

		GafferScene::ScenePlug *variationsPlug();
		const GafferScene::ScenePlug *variationsPlug() const;

		Gaffer::BoolPlug *useInstancesPlug();
		const Gaffer::BoolPlug *useInstancesPlug() const;

        Gaffer::FloatPlug *boundingBoxPaddingPlug();
        const Gaffer::FloatPlug *boundingBoxPaddingPlug() const;

		void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

	protected:

		void hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		void compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const override;

		void hashBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		Imath::Box3f computeBranchBound( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const override;

		void hashBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		Imath::M44f computeBranchTransform( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const override;

		void hashBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstCompoundObjectPtr computeBranchAttributes( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const override;

		void hashBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstObjectPtr computeBranchObject( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const override;

		void hashBranchChildNames( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstInternedStringVectorDataPtr computeBranchChildNames( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context ) const override;

		void hashBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstInternedStringVectorDataPtr computeBranchSetNames( const ScenePath &parentPath, const Gaffer::Context *context ) const override;

		void hashBranchSet( const ScenePath &parentPath, const IECore::InternedString &setName, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstPathMatcherDataPtr computeBranchSet( const ScenePath &parentPath, const IECore::InternedString &setName, const Gaffer::Context *context ) const override;

	private :

		Gaffer::AtomicCompoundDataPlug *agentChildNamesPlug();
		const Gaffer::AtomicCompoundDataPlug *agentChildNamesPlug() const;

		IECore::ConstCompoundDataPtr agentChildNames( const ScenePath &parentPath, const Gaffer::Context *context ) const;
		void agentChildNamesHash( const ScenePath &parentPath, const Gaffer::Context *context, IECore::MurmurHash &h ) const;

		void atomsPoseHash( const ScenePath &parentPath, const ScenePath &branchPath, const Gaffer::Context *context, IECore::MurmurHash &h) const;

        IECore::ConstCompoundDataPtr agentCacheData(const ScenePath &branchPath) const;

		struct AgentScope : public Gaffer::Context::EditableScope
		{
			AgentScope( const Gaffer::Context *context, const ScenePath &branchPath );
		};

		static size_t g_firstPlugIndex;

};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSCROWDGENERATOR_H
