cmake_minimum_required(VERSION 3.0)
project(AtomsGaffer)

option( USE_GAFFER_DEPENDENCIES "Turn this off you're building against your own dependencies (eg boost, python, tbb, exr, cortex) rather than the versions included with Gaffer" ON )
option( USE_ATOMS_PROCEDURAL "Turn this off you plan to load AtomsGaffer in maya or another environment that requires a full atoms license" ON )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(ATOMS_MAYA_ROOT)
    set(ATOMS_INCLUDE_PATH ${ATOMS_MAYA_ROOT}/include)
    set(ATOMS_LIB_PATH ${ATOMS_MAYA_ROOT}/lib/2018)
    set(ATOMS_ICONS_PATH ${ATOMS_MAYA_ROOT}/icons)
endif()

if(ATOMS_ROOT)
    set(ATOMS_INCLUDE_PATH ${ATOMS_ROOT}/include)
    set(ATOMS_LIB_PATH ${ATOMS_ROOT}/lib)
    set(ATOMS_ICONS_PATH ${ATOMS_ROOT}/icons)
endif()

if(USE_GAFFER_DEPENDENCIES)
    set(BOOST_INCLUDE_PATH ${GAFFER_ROOT}/include)
    set(PYTHON_ROOT ${GAFFER_ROOT})
    set(TBB_ROOT ${GAFFER_ROOT})
    set(OPENEXR_ROOT ${GAFFER_ROOT})
    set(CORTEX_ROOT ${GAFFER_ROOT})
endif()

set(DEPENDENCY_INCLUDE_PATHS
    ${BOOST_INCLUDE_PATH}
    ${TBB_ROOT}/include
    ${OPENEXR_ROOT}/include
    ${OPENEXR_ROOT}/include/OpenEXR
    ${CORTEX_ROOT}/include
    ${GAFFER_ROOT}/include
    ${ATOMS_INCLUDE_PATH}
)

# build the library
file( GLOB AtomsGafferSrc src/AtomsGaffer/*.cpp )
link_directories( AtomsGaffer ${GAFFER_ROOT}/lib ${ATOMS_LIB_PATH} )
add_library( AtomsGaffer SHARED ${AtomsGafferSrc} )
target_compile_definitions( AtomsGaffer PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1 )
target_include_directories( AtomsGaffer PRIVATE include ${DEPENDENCY_INCLUDE_PATHS} )
if(USE_ATOMS_PROCEDURAL)
	target_link_libraries( AtomsGaffer Gaffer GafferScene AtomsProcedural )
else()
	target_link_libraries( AtomsGaffer Gaffer GafferScene AtomsCore AtomsGraph Atoms AtomsUtils )
endif()
install( TARGETS AtomsGaffer DESTINATION lib )

# build the python bindings
file( GLOB AtomsGafferModuleSrc src/AtomsGafferModule/*.cpp )
add_library( AtomsGafferModule SHARED ${AtomsGafferModuleSrc} )
set_target_properties( AtomsGafferModule PROPERTIES PREFIX "" OUTPUT_NAME "_AtomsGaffer" )
target_compile_definitions( AtomsGafferModule PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1 )
target_link_libraries( AtomsGafferModule AtomsGaffer )
target_include_directories( AtomsGafferModule PRIVATE include ${DEPENDENCY_INCLUDE_PATHS} ${PYTHON_ROOT}/include/python2.7 )
target_link_libraries( AtomsGafferModule GafferBindings )
install( TARGETS AtomsGafferModule DESTINATION python/AtomsGaffer )

# build the python modules
install( DIRECTORY python DESTINATION . FILES_MATCHING PATTERN "*.py" )

# build the startup configs
install( DIRECTORY startup DESTINATION . FILES_MATCHING PATTERN "*.py" )

# build the graphics
install( FILES ${ATOMS_ICONS_PATH}/logo_new.png DESTINATION graphics RENAME atoms_logo.png )

# build the examples
file( GLOB AtomsGafferExampleScripts examples/scripts/*.gfr )
install( FILES ${AtomsGafferExampleScripts} DESTINATION examples/scripts )
install( DIRECTORY examples/assets DESTINATION examples )

