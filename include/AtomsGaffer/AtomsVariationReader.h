//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Toolchefs Ltd. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef ATOMSGAFFER_ATOMSAGENTREADER_H
#define ATOMSGAFFER_ATOMSAGENTREADER_H

#include "AtomsGaffer/TypeIds.h"

#include "GafferScene/SceneNode.h"

#include "Gaffer/StringPlug.h"

#include "IECoreScene/SceneInterface.h"

#include "AtomsCore/Metadata/ArrayMetadata.h"
#include "AtomsCore/Metadata/MeshMetadata.h"
#include "AtomsCore/Metadata/MapMetadata.h"

namespace AtomsGaffer
{

// \todo: We may replace this node with an AtomsSceneInterface
// and load via the GafferScene::SceneReader. We're not doing
// that yet because I'm not familiar enough with the various
// file formats that Atoms provides.
class AtomsVariationReader : public GafferScene::SceneNode
{

	public:

		AtomsVariationReader( const std::string &name = defaultName<AtomsVariationReader>() );
		~AtomsVariationReader() = default;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( AtomsGaffer::AtomsVariationReader, TypeId::AtomsVariationReaderTypeId, GafferScene::SceneNode );

		Gaffer::StringPlug *atomsVariationFilePlug();
		const Gaffer::StringPlug *atomsVariationFilePlug() const;

		Gaffer::IntPlug *refreshCountPlug();
		const Gaffer::IntPlug *refreshCountPlug() const;

		Gaffer::BoolPlug *generatePrefPlug();
		const Gaffer::BoolPlug *generatePrefPlug() const;

		Gaffer::BoolPlug *generateNrefPlug();
		const Gaffer::BoolPlug *generateNrefPlug() const;

        Gaffer::ObjectPlug *enginePlug();
        const Gaffer::ObjectPlug *enginePlug() const;

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
        void hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;

		Imath::Box3f computeBound( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		Imath::M44f computeTransform( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstCompoundObjectPtr computeAttributes( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstObjectPtr computeObject( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstInternedStringVectorDataPtr computeChildNames( const ScenePath &path, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstCompoundObjectPtr computeGlobals( const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstInternedStringVectorDataPtr computeSetNames( const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
		IECore::ConstPathMatcherDataPtr computeSet( const IECore::InternedString &setName, const Gaffer::Context *context, const GafferScene::ScenePlug *parent ) const override;
        void compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const;

	private:

        void mergeUvSets( AtomsUtils::Mesh& mesh, AtomsUtils::Mesh& inMesh, size_t startSize ) const;

		void mergeAtomsMesh(
				AtomsPtr<AtomsCore::MapMetadata>& geoMap,
				AtomsPtr<AtomsCore::MapMetadata>& outGeoMap,
				AtomsPtr<AtomsCore::MeshMetadata>& outMeshMeta,
				AtomsPtr<AtomsCore::ArrayMetadata>& outBlendMeta
		) const;

	private :

        IE_CORE_FORWARDDECLARE( EngineData );

		static size_t g_firstPlugIndex;

};

} // namespace AtomsGaffer

#endif // ATOMSGAFFER_ATOMSAGENTREADER_H
