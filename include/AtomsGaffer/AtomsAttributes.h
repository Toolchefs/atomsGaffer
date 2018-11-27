#ifndef ATOMSGAFFER_ATOMSATTRIBUTES_H
#define ATOMSGAFFER_ATOMSATTRIBUTES_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneProcessor.h"

#include "GafferScene/ScenePath.h"

#include "Gaffer/StringPlug.h"

#include "Gaffer/CompoundDataPlug.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECore/CompoundObject.h"

namespace AtomsGaffer
{

    class AtomsAttributes : public GafferScene::SceneProcessor
    {

    public :

        AtomsAttributes( const std::string &name = defaultName<AtomsAttributes>() );
        ~AtomsAttributes() override;

        Gaffer::StringPlug *agentIdsPlug();
        const Gaffer::StringPlug *agentIdsPlug() const;

        Gaffer::BoolPlug *invertPlug();
        const Gaffer::BoolPlug *invertPlug() const;

        Gaffer::CompoundDataPlug *metadataPlug();
        const Gaffer::CompoundDataPlug *metadataPlug() const;

        IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsAttributes, TypeId::AtomsAttributesTypeId, GafferScene::SceneProcessor );
        void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

    protected :

        void hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
        IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

    private:

        void parseVisibleAgents(std::vector<int>& agentsFiltered,  std::vector<int>& agentIds, const std::string& filter, bool invert = false) const;

        template <typename T, typename V>
        void setMetadataOnPoints(
                IECoreScene::PointsPrimitivePtr& primitive,
                const std::string& metadataName,
                const std::vector<int>& agentIdVec,
                const std::vector<int>& agentsFiltered,
                std::map<int, int>& agentIdPointsMapper,
                const T& data,
                IECore::ConstCompoundObjectPtr& attributes
        ) const;

    private :

        static size_t g_firstPlugIndex;

    };


    template <>
    void AtomsAttributes::setMetadataOnPoints<Imath::M44f, IECore::M44dData>(
            IECoreScene::PointsPrimitivePtr& primitive,
            const std::string& metadataName,
            const std::vector<int>& agentIdVec,
            const std::vector<int>& agentsFiltered,
            std::map<int, int>& agentIdPointsMapper,
            const Imath::M44f& data,
            IECore::ConstCompoundObjectPtr& attributes
    ) const;
/*
class AtomsAttributes : public GafferScene::Attributes
{

	public:

		AtomsAttributes( const std::string &name = defaultName<AtomsAttributes>() );
		~AtomsAttributes() = default;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsAttributes, TypeId::AtomsAttributesTypeId, GafferScene::Attributes );

};
*/
} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSATTRIBUTES_H
