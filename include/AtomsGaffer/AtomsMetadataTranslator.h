#ifndef ATOMSGAFFER_ATOMSCONTERTERS_H
#define ATOMSGAFFER_ATOMSCONTERTERS_H

#include "IECore/Object.h"
#include "IECore/Data.h"

#include "AtomsCore/Metadata/Metadata.h"

#include <map>

namespace AtomsGaffer
{
    class AtomsMetadataTranslator
    {
    public:

        typedef IECore::DataPtr(*Translator)(const AtomsPtr<AtomsCore::Metadata>&);

        static AtomsMetadataTranslator& instance();

        IECore::DataPtr translate(const AtomsPtr<AtomsCore::Metadata>& metadata);

    private:

        AtomsMetadataTranslator();

        ~AtomsMetadataTranslator();

        AtomsMetadataTranslator(const AtomsMetadataTranslator&) = delete;

        AtomsMetadataTranslator& operator=(const AtomsMetadataTranslator&) = delete;

    private:

        std::map<unsigned int, Translator> translatorMap;

    };

}

#endif //ATOMSGAFFER_ATOMSCONTERTERS_H
