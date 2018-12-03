cmake_minimum_required(VERSION 3.0)
project(AtomsGaffer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE OFF)

if(ATOMS_MAYA_ROOT)
    set(ATOMS_INCLUDE_PATH ${ATOMS_MAYA_ROOT}/include)
    set(ATOMS_LIB_PATH ${ATOMS_MAYA_ROOT}/2018/lib)
    set(ATOMS_ICONS_PATH ${ATOMS_MAYA_ROOT}/icons)
endif()

if(ATOMS_ROOT)
    set(ATOMS_INCLUDE_PATH ${ATOMS_ROOT}/include)
    set(ATOMS_LIB_PATH ${ATOMS_ROOT}/lib)
    set(ATOMS_ICONS_PATH ${ATOMS_ROOT}/icons)
endif()

# build the library
file( GLOB AtomsGafferSrc src/AtomsGaffer/*.cpp )
link_directories( AtomsGaffer ${GAFFER_ROOT}/lib ${ATOMS_LIB_PATH} )
add_library( AtomsGaffer SHARED ${AtomsGafferSrc} include/AtomsGaffer/AtomsMetadataTranslator.h)
target_compile_definitions( AtomsGaffer PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1 )
target_include_directories( AtomsGaffer PRIVATE include ${GAFFER_ROOT}/include ${ATOMS_INCLUDE_PATH} ${GAFFER_ROOT}/include/OpenEXR)
target_link_libraries( AtomsGaffer Gaffer GafferScene AtomsProcedural)
install( TARGETS AtomsGaffer DESTINATION lib)

# build the python bindings
file( GLOB AtomsGafferModuleSrc src/AtomsGafferModule/*.cpp )
add_library( AtomsGafferModule SHARED ${AtomsGafferModuleSrc} )
set_target_properties( AtomsGafferModule PROPERTIES PREFIX "" OUTPUT_NAME "_AtomsGaffer" )
target_compile_definitions( AtomsGafferModule PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1 )
target_link_libraries( AtomsGafferModule AtomsGaffer )
target_include_directories( AtomsGafferModule PRIVATE include ${GAFFER_ROOT}/include ${GAFFER_ROOT}/include/python2.7 )
target_link_libraries( AtomsGafferModule GafferBindings )
install( TARGETS AtomsGafferModule DESTINATION python/AtomsGaffer )

# build the python modules
install( DIRECTORY python DESTINATION . FILES_MATCHING PATTERN "*.py" )

# build the startup configs
install( DIRECTORY startup DESTINATION . FILES_MATCHING PATTERN "*.py" )

# build the graphics
install( FILES ${ATOMS_ICONS_PATH}/logo_new.png DESTINATION graphics RENAME atoms_logo.png )

# build the examples
file( GLOB AtomsGafferExamples examples/scripts/*.gfr )
install( FILES ${AtomsGafferExamples} DESTINATION examples/scripts )
