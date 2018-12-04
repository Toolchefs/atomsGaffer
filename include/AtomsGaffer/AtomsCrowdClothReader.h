#ifndef ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H
#define ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneProcessor.h"

#include "Gaffer/StringPlug.h"
#include "Gaffer/NumericPlug.h"

namespace AtomsGaffer
{

// \todo: We may replace this node with an AtomsSceneInterface
// and load via the GafferScene::SceneReader. We're not doing
// that yet because we haven't decided what direction to go with
// regards to the various modes.
    class AtomsCrowdClothReader : public GafferScene::SceneProcessor
    {

    public:

        AtomsCrowdClothReader( const std::string &name = defaultName<AtomsCrowdClothReader>() );
        ~AtomsCrowdClothReader() = default;

        IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsCrowdClothReader, TypeId::AtomsCrowdClothReaderTypeId, GafferScene::SceneProcessor );

        Gaffer::StringPlug *atomsClothFilePlug();
        const Gaffer::StringPlug *atomsClothFilePlug() const;

        Gaffer::FloatPlug *timeOffsetPlug();
        const Gaffer::FloatPlug *timeOffsetPlug() const;

        Gaffer::IntPlug *refreshCountPlug();
        const Gaffer::IntPlug *refreshCountPlug() const;

        void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const override;

    protected:

        void hashObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent, IECore::MurmurHash &h ) const override;
        IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;

    private:

        void getAtomsCacheName( const std::string& filePath, std::string& cachePath, std::string& cacheName, const std::string& extension ) const;

    private:

        static size_t g_firstPlugIndex;

    };

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSCROWDCLOTHREADER_H
