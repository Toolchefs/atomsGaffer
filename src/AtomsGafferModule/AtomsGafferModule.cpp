#include "boost/python.hpp"

#include "AtomsGaffer/AtomsCrowdReader.h"
#include "AtomsGaffer/AtomsVariationReader.h"
#include "AtomsGaffer/AtomsCrowdGenerator.h"
#include "AtomsGaffer/AtomsAttributes.h"

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

	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdReader>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsVariationReader>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsCrowdGenerator>();
	GafferBindings::DependencyNodeClass<AtomsGaffer::AtomsAttributes>();
}
