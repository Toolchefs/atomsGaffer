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

#include "boost/python.hpp"

#include "IECorePython/RunTimeTypedBinding.h"

#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsVariationReader.h"
#include "AtomsGaffer/AtomsCrowdGenerator.h"
#include "AtomsGaffer/AtomsAttributes.h"
#include "AtomsGaffer/AtomsMetadata.h"
#include "AtomsGaffer/AtomsCrowdClothReader.h"

#include "GafferBindings/DependencyNodeBinding.h"
#include "IECore/MessageHandler.h"

#include "Atoms/Initialize.h"
#include "AtomsUtils/Logger.h"

using namespace boost::python;
using namespace GafferScene;

class GafferLogType : public AtomsUtils::LogType
{
public:
	GafferLogType() : AtomsUtils::LogType()
	{}

	~GafferLogType() {}

	void info(const std::string &message) override
	{
		IECore::msg( IECore::Msg::Info, "Atoms", message );
	}

	void warning(const std::string &message) override
	{
		IECore::msg( IECore::Msg::Warning, "Atoms", message );
	}

	void error(const std::string &message) override
	{
		IECore::msg( IECore::Msg::Error, "Atoms", message );
	}

	void cmd(const std::string &message)
	{

	}
};


BOOST_PYTHON_MODULE( _AtomsGaffer )
{
	// Initialize atoms lib
	AtomsPtr<AtomsUtils::LogType> mayaLogger(new GafferLogType());
	AtomsUtils::Logger::instance().setLogType(mayaLogger);
	Atoms::initAtoms();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsCrowdReader> AtomsCrowdReaderWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdReader, AtomsCrowdReaderWrapper>();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsVariationReader> AtomsVariationReaderWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsVariationReader, AtomsVariationReaderWrapper>();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsCrowdGenerator> AtomsCrowdGeneratorWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdGenerator, AtomsCrowdGeneratorWrapper>();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsAttributes> AtomsAttributesWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsAttributes, AtomsAttributesWrapper>();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsCrowdClothReader> AtomsCrowdClothReaderWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdClothReader, AtomsCrowdClothReaderWrapper>();

	typedef GafferBindings::DependencyNodeWrapper<AtomsGaffer::AtomsMetadata> AtomsMetadataWrapper;
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsMetadata, AtomsMetadataWrapper>();
}
