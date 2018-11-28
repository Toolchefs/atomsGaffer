#include "AtomsGaffer/AtomsMetadataTranslator.h"

#include "IECore/TypedData.h"
#include "IECore/SimpleTypedData.h"
#include "IECore/VectorTypedData.h"
#include "IECore/CompoundData.h"
#include "IECore/MessageHandler.h"

#include "AtomsCore/Metadata/IntMetadata.h"
#include "AtomsCore/Metadata/BoolMetadata.h"
#include "AtomsCore/Metadata/DoubleMetadata.h"
#include "AtomsCore/Metadata/StringMetadata.h"
#include "AtomsCore/Metadata/QuaternionMetadata.h"
#include "AtomsCore/Metadata/Box3Metadata.h"
#include "AtomsCore/Metadata/Vector3Metadata.h"
#include "AtomsCore/Metadata/Vector2Metadata.h"
#include "AtomsCore/Metadata/MatrixMetadata.h"
#include "AtomsCore/Metadata/IntArrayMetadata.h"
#include "AtomsCore/Metadata/BoolArrayMetadata.h"
#include "AtomsCore/Metadata/DoubleArrayMetadata.h"
#include "AtomsCore/Metadata/StringArrayMetadata.h"
#include "AtomsCore/Metadata/QuaternionArrayMetadata.h"
#include "AtomsCore/Metadata/Vector3ArrayMetadata.h"
#include "AtomsCore/Metadata/Vector2ArrayMetadata.h"
#include "AtomsCore/Metadata/MatrixArrayMetadata.h"
#include "AtomsCore/Metadata/MapMetadata.h"
#include "AtomsCore/Metadata/ArrayMetadata.h"
#include "AtomsCore/Metadata/PoseMetadata.h"
#include "AtomsCore/Metadata/CurveMetadata.h"
#include "AtomsCore/Metadata/MeshMetadata.h"
#include "AtomsCore/Metadata/ImageMetadata.h"

using namespace IECore;
using namespace AtomsGaffer;

template <typename T>
IECore::DataPtr metadataSimpleTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto value = std::static_pointer_cast<AtomsCore::MetadataImpl<T>>( metadata )->get();
    typename TypedData<T>::Ptr ieData = new TypedData<T>();
    ieData->writable() = value;
    return boost::static_pointer_cast<Data>( ieData );
}

template <typename T>
IECore::DataPtr metadataGeometricTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto value = std::static_pointer_cast<AtomsCore::MetadataImpl<T>>( metadata )->get();
    typename GeometricTypedData<T>::Ptr ieData = new GeometricTypedData<T>();
    ieData->writable() = value;
    return boost::static_pointer_cast<Data>( ieData );
}


template <typename T>
IECore::DataPtr metadataArrayTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto &value = std::static_pointer_cast<AtomsCore::TypedArrayMetadata<T>>( metadata )->get();
    typename TypedData<std::vector<T>>::Ptr ieData = new TypedData<std::vector<T>>();
    ieData->writable() = value;
    return boost::static_pointer_cast<Data>( ieData );
}

IECore::DataPtr mapMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto &translator = AtomsMetadataTranslator::instance();
    CompoundDataPtr result = new CompoundData;
    auto mapData = std::static_pointer_cast<const AtomsCore::MapMetadata>( metadata );
    for( auto it = mapData->cbegin(); it != mapData->cend(); ++it )
    {
        if ( !it->second )
            continue;

        IECore::DataPtr object = translator.translate( it->second );
        if ( object )
            result->writable()[it->first] = object;
    }
    return result;
}

IECore::DataPtr arrayMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto &translator = AtomsMetadataTranslator::instance();
    CompoundDataPtr result = new CompoundData;
    auto arrayData = std::static_pointer_cast<AtomsCore::ArrayMetadata>( metadata );
    for( size_t i = 0; i < arrayData->size(); ++i )
    {
        IECore::DataPtr object = translator.translate( ( *arrayData )[i] );
        if ( object )
            result->writable()[std::to_string( i )] = object;
    }
    return result;
}

IECore::DataPtr poseMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    auto &translator = AtomsMetadataTranslator::instance();
    CompoundDataPtr result = new CompoundData;
    auto atomsData = std::static_pointer_cast<const AtomsCore::PoseMetadata>( metadata );
    const AtomsCore::Pose& pose = atomsData->get();

    //translate matrices
    M44dVectorDataPtr matrices = new M44dVectorData;
    auto& matrixVec = matrices->writable();
    matrixVec.resize( pose.numJoints() );
    for( unsigned short i = 0; i < pose.numJoints(); ++i )
    {
        matrixVec[i] = pose.jointPose( i ).matrix();
    }
    result->writable()["matrices"] = matrices;

    //translate joint metadata
    auto ids = pose.jointMetadataIds();
    CompoundDataPtr jointsMeta = new CompoundData;
    for( size_t i = 0; i < ids.size(); ++i )
    {
        const auto& jointData = pose.jointMetadata( ids[i] );
        CompoundDataPtr jointMeta = new CompoundData;

        for( auto it = jointData.cbegin(); it != jointData.cend(); ++it )
        {
            if ( !it->second )
                continue;

            IECore::DataPtr object = translator.translate( it->second );
            if ( object )
                jointMeta->writable()[it->first] = object;
        }

        jointsMeta->writable()[std::to_string( ids[i] )] = jointMeta;
    }

    result->writable()["jointMetadata"] = jointsMeta;

    // translate pose metadata
    CompoundDataPtr meta = new CompoundData;
    auto& mapData = pose.poseMetadata();

    for( auto it = mapData.cbegin(); it != mapData.cend(); ++it )
    {
        if ( !it->second )
            continue;

        IECore::DataPtr object = translator.translate( it->second );
        if ( object )
            meta->writable()[it->first] = object;
    }

    result->writable()["metadata"] = meta;

    return result;
}

IECore::DataPtr curveMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    // todo
    return IECore::DataPtr();
}

IECore::DataPtr meshMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    // todo
    return IECore::DataPtr();
}

IECore::DataPtr imageMetadataTranslator( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    // todo
    return IECore::DataPtr();
}

AtomsMetadataTranslator::AtomsMetadataTranslator()
{
    translatorMap[AtomsCore::BoolMetadata::staticTypeId()] = metadataSimpleTranslator<bool>;
    translatorMap[AtomsCore::IntMetadata::staticTypeId()] = metadataSimpleTranslator<int>;
    translatorMap[AtomsCore::DoubleMetadata::staticTypeId()] = metadataSimpleTranslator<double>;
    translatorMap[AtomsCore::StringMetadata::staticTypeId()] = metadataSimpleTranslator<std::string>;
    translatorMap[AtomsCore::Vector2Metadata::staticTypeId()] = metadataGeometricTranslator<AtomsCore::Vector2>;
    translatorMap[AtomsCore::Vector3Metadata::staticTypeId()] = metadataGeometricTranslator<AtomsCore::Vector3>;
    translatorMap[AtomsCore::Box3Metadata::staticTypeId()] = metadataSimpleTranslator<AtomsCore::Box3>;
    translatorMap[AtomsCore::QuaternionMetadata::staticTypeId()] = metadataSimpleTranslator<AtomsCore::Quaternion>;
    translatorMap[AtomsCore::MatrixMetadata::staticTypeId()] = metadataSimpleTranslator<AtomsCore::Matrix>;

    translatorMap[AtomsCore::BoolArrayMetadata::staticTypeId()] = metadataArrayTranslator<bool>;
    translatorMap[AtomsCore::IntArrayMetadata::staticTypeId()] = metadataArrayTranslator<int>;
    translatorMap[AtomsCore::DoubleArrayMetadata::staticTypeId()] = metadataArrayTranslator<double>;
    translatorMap[AtomsCore::StringArrayMetadata::staticTypeId()] = metadataArrayTranslator<std::string>;
    translatorMap[AtomsCore::Vector2ArrayMetadata::staticTypeId()] = metadataArrayTranslator<AtomsCore::Vector2>;
    translatorMap[AtomsCore::Vector3ArrayMetadata::staticTypeId()] = metadataArrayTranslator<AtomsCore::Vector3>;
    translatorMap[AtomsCore::QuaternionArrayMetadata::staticTypeId()] = metadataArrayTranslator<AtomsCore::Quaternion>;
    translatorMap[AtomsCore::MatrixArrayMetadata::staticTypeId()] = metadataArrayTranslator<AtomsCore::Matrix>;

    translatorMap[AtomsCore::MapMetadata::staticTypeId()] = mapMetadataTranslator;
    translatorMap[AtomsCore::ArrayMetadata::staticTypeId()] = arrayMetadataTranslator;
    translatorMap[AtomsCore::PoseMetadata::staticTypeId()] = poseMetadataTranslator;

    translatorMap[AtomsCore::CurveMetadata::staticTypeId()] = curveMetadataTranslator;
    translatorMap[AtomsCore::MeshMetadata::staticTypeId()] = meshMetadataTranslator;
    translatorMap[AtomsCore::ImageMetadata::staticTypeId()] = imageMetadataTranslator;
}

AtomsMetadataTranslator& AtomsMetadataTranslator::instance()
{
    static AtomsMetadataTranslator data;
    return data;
}

IECore::DataPtr AtomsMetadataTranslator::translate( const AtomsPtr<AtomsCore::Metadata>& metadata )
{
    if ( !metadata )
        return IECore::DataPtr();

    auto it = translatorMap.find( metadata->typeId() );
    if ( it != translatorMap.end() )
        return ( it->second )( metadata );

    IECore::msg( IECore::Msg::Warning, "AtomsMetadataTranslator", "Metadata converter doesn't exist for metadata type: " + metadata->typeStr() );
    return IECore::DataPtr();
}
