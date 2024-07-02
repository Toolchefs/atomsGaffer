cmake_minimum_required(VERSION 3.10)
project(AtomsGaffer)

option( USE_GAFFER_DEPENDENCIES "Turn this off you're building against your own dependencies (eg boost, python, tbb, exr, cortex) rather than the versions included with Gaffer" ON )

set(GAFFER_ROOT "/opt/gaffer" CACHE PATH "Gaffer root folder")
set(ATOMS_MAYA_ROOT "/opt/Toolchefs/AtomsMaya" CACHE PATH "Gaffer root folder")
set(PYTHON_VERSION 3.10 CACHE STRING "Python version")
set(MAYA_MAJOR_VERSION 2023 CACHE STRING "Maya version")

set(CMAKE_VERBOSE_MAKEFILE ON)

if(ATOMS_MAYA_ROOT)
    set(ATOMS_INCLUDE_PATH ${ATOMS_MAYA_ROOT}/include)
    set(ATOMS_LIB_PATH ${ATOMS_MAYA_ROOT}/lib/${MAYA_MAJOR_VERSION})
    set(ATOMS_ICONS_PATH ${ATOMS_MAYA_ROOT}/icons)
else ()
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
    set(FMT_ROOT ${GAFFER_ROOT})
endif()

set(DEPENDENCY_INCLUDE_PATHS
    ${BOOST_INCLUDE_PATH}
    ${TBB_ROOT}/include
    ${OPENEXR_ROOT}/include
    ${OPENEXR_ROOT}/include/OpenEXR
    ${CORTEX_ROOT}/include
    ${GAFFER_ROOT}/include
    ${FMT_ROOT}/include
    ${ATOMS_INCLUDE_PATH}
)

# If no OpenEXR major version is passed as an argument
# we try to read the version from the OpenEXR cmake package config.
if(NOT OPENEXR_MAJOR_VERSION)
  # We assume OpenEXR being installed with their cmake package config files
  # (normally in `lib/cmake` within the installation directory). So we add
  # the root install directory to CMAKE_PREFIX_PATH for find_package
  # to work
  list(APPEND CMAKE_PREFIX_PATH ${OPENEXR_ROOT})
  find_package(OpenEXR PATHS ${OPENEXR_ROOT} CONFIG)
endif()

# For OpenEXR version 3 and up, we have to add the Imath includes explicitly.
# We are trying to find the Imath cmake package config and read it's
# interface include directories to append them to the include path.
# This works with gaffer dependencies as well with own builds of OpenEXR and Imath
# which have their cmake directory anywhere in cmakes package search paths
# As a fallback the root directory can be set via `IMATH_ROOT`.
if(${OpenEXR_VERSION_MAJOR} AND ${OpenEXR_VERSION_MAJOR} VERSION_GREATER "2")
  if(NOT IMATH_ROOT)
    find_package(Imath CONFIG)
    get_target_property(IMATH_INCLUDES Imath::ImathConfig INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND DEPENDENCY_INCLUDE_PATHS ${IMATH_INCLUDES})
  else()
    list(APPEND DEPENDENCY_INCLUDE_PATHS "${IMATH_ROOT}/include")
    list(APPEND DEPENDENCY_INCLUDE_PATHS "${IMATH_ROOT}/include/Imath")
  endif()
elseif(${OPENEXR_MAJOR_VERSION} AND ${OPENEXR_MAJOR_VERSION} VERSION_GREATER "2")
  if(NOT IMATH_ROOT)
    find_package(Imath CONFIG)
    get_target_property(IMATH_INCLUDES Imath::ImathConfig INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND DEPENDENCY_INCLUDE_PATHS ${IMATH_INCLUDES})
  else()
    list(APPEND DEPENDENCY_INCLUDE_PATHS "${IMATH_ROOT}/include")
    list(APPEND DEPENDENCY_INCLUDE_PATHS "${IMATH_ROOT}/include/Imath")
  endif()
endif()

# The Atoms version is defined in the AtomsUtils/Version.h file. We read
# and parse it here for the major version
if(NOT ATOMS_MAJOR_VERSION)
  find_file(ATOMS_VERSION_H "Version.h" ${ATOMS_INCLUDE_PATH}/AtomsUtils NO_CACHE)
  file(READ ${ATOMS_VERSION_H} ATOMS_VERSION_CONTENT)
  string(REGEX MATCH "ATOMS_MAJOR_VERSION ([0-9]*)" _ ${ATOMS_VERSION_CONTENT})
  set(ATOMS_MAJOR_VERSION ${CMAKE_MATCH_1})
endif()

# In Atoms 6 the half type definition was moved into the Atoms namespace.
# When for example including AtomsUtils/Logger.h it will pull the half.h shipped with
# Atoms into the file. Since we want to use the one coming with our Imath
# version, we instruct the preprocessor to include `half.h` as if it was
# the first instruction in whatever source file and it will include a `half.h`
# found in one of the include paths (which we asume to be OpenExr/Imath).
# We make this version dependent to keep the previous behavior for older (re)builds.
if(ATOMS_MAJOR_VERSION VERSION_GREATER_EQUAL "6")
  add_compile_options( -include "half.h" )
endif()

# build the library
file( GLOB AtomsGafferSrc src/AtomsGaffer/*.cpp )
link_directories( AtomsGaffer ${GAFFER_ROOT}/lib ${ATOMS_LIB_PATH} )
add_library( AtomsGaffer SHARED ${AtomsGafferSrc} )
set_property(TARGET AtomsGaffer PROPERTY CXX_STANDARD 17)
target_compile_definitions( AtomsGaffer PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1)
if (MAYA_MAJOR_VERSION STREQUAL "2024")
    target_compile_definitions( AtomsGaffer PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
else ()
    target_compile_definitions( AtomsGaffer PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
endif ()
target_include_directories( AtomsGaffer SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_PATHS} )
target_include_directories( AtomsGaffer PRIVATE include )
# Since Atoms version 6, symbols from AtomsCore, AtomsGraph and AtomsUtils have been
# moved to the Atoms library, so we only link against the former for Atoms versions
# less than 6.
set(ATOMS_ADDITIONAL_LINK_LIBS AtomsCore AtomsGraph AtomsUtils)
target_link_libraries( AtomsGaffer Gaffer GafferScene Atoms "$<$<VERSION_LESS:${ATOMS_MAJOR_VERSION},6>:${ATOMS_ADDITIONAL_LINK_LIBS}>")
install( TARGETS AtomsGaffer DESTINATION lib )

# build the python bindings
file( GLOB AtomsGafferModuleSrc src/AtomsGafferModule/*.cpp )
add_library( AtomsGafferModule SHARED ${AtomsGafferModuleSrc} )
set_property(TARGET AtomsGafferModule PROPERTY CXX_STANDARD 17)
set_target_properties( AtomsGafferModule PROPERTIES PREFIX "" OUTPUT_NAME "_AtomsGaffer" )
target_compile_definitions( AtomsGafferModule PRIVATE BOOST_SIGNALS_NO_DEPRECATION_WARNING=1 LINUX=1 _GLIBCXX_USE_CXX11_ABI=0)
target_link_libraries( AtomsGafferModule AtomsGaffer )
target_include_directories( AtomsGafferModule SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_PATHS} ${PYTHON_ROOT}/include/python${PYTHON_VERSION} )
target_include_directories( AtomsGafferModule PRIVATE include )
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

