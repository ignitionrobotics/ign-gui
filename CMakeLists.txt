cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)

#============================================================================
# Initialize the project
#============================================================================
project(ignition-gui5 VERSION 5.1.0)

#============================================================================
# Find ignition-cmake
#============================================================================
find_package(ignition-cmake2 2.8.0 REQUIRED)

#============================================================================
# Configure the project
#============================================================================
ign_configure_project(VERSION_SUFFIX)

#============================================================================
# Set project-specific options
#============================================================================


#============================================================================
# Search for project-specific dependencies
#============================================================================
message(STATUS "\n\n-- ====== Finding Dependencies ======")

#--------------------------------------
# Find Protobuf
set(REQ_PROTOBUF_VER 3)
ign_find_package(IgnProtobuf
                 VERSION ${REQ_PROTOBUF_VER}
                 REQUIRED
                 PRETTY Protobuf)

#--------------------------------------
# Find Tinyxml2
ign_find_package(TINYXML2 REQUIRED PRIVATE PRETTY tinyxml2)

#--------------------------------------
# Find ignition-math
ign_find_package(ignition-math6 REQUIRED VERSION 6.5)
set(IGN_MATH_VER ${ignition-math6_VERSION_MAJOR})

#--------------------------------------
# Find ignition-common
ign_find_package(ignition-common4 REQUIRED VERSION 4.1)
set(IGN_COMMON_VER ${ignition-common4_VERSION_MAJOR})

#--------------------------------------
# Find ignition-plugin
ign_find_package(ignition-plugin1 REQUIRED COMPONENTS loader register)
set(IGN_PLUGIN_VER ${ignition-plugin1_VERSION_MAJOR})

#--------------------------------------
# Find ignition-transport
ign_find_package(ignition-transport10 REQUIRED)
set(IGN_TRANSPORT_VER ${ignition-transport10_VERSION_MAJOR})

#--------------------------------------
# Find ignition-rendering
ign_find_package(ignition-rendering5 REQUIRED VERSION 5.1 COMPONENTS ogre)
set(IGN_RENDERING_VER ${ignition-rendering5_VERSION_MAJOR})

#--------------------------------------
# Find ignition-msgs
ign_find_package(ignition-msgs7 REQUIRED)
set(IGN_MSGS_VER ${ignition-msgs7_VERSION_MAJOR})

#--------------------------------------
# Find ignition-tools
ign_find_package(ignition-tools QUIET)
if (ignition-tools_FOUND)
  set (HAVE_IGN_TOOLS TRUE)
endif()

#--------------------------------------
# Find ignition-utils
ign_find_package(ignition-utils1 REQUIRED COMPONENTS cli)
set(IGN_UTILS_VER ${ignition-utils1_VERSION_MAJOR})

#--------------------------------------
# Find QT
ign_find_package (Qt5
  COMPONENTS
    Core
    Quick
    QuickControls2
    Widgets
    Qml
  REQUIRED
  PKGCONFIG "Qt5Core Qt5Quick Qt5Qml Qt5QuickControls2 Qt5Widgets"
)

set(IGNITION_GUI_PLUGIN_INSTALL_DIR
  ${CMAKE_INSTALL_PREFIX}/${IGN_LIB_INSTALL_DIR}/ign-${IGN_DESIGNATION}-${PROJECT_VERSION_MAJOR}/plugins
)

#============================================================================
# Configure the build
#============================================================================
ign_configure_build(QUIT_IF_BUILD_ERRORS)

#============================================================================
# Create package information
#============================================================================
ign_create_packages()

configure_file(${CMAKE_SOURCE_DIR}/api.md.in ${CMAKE_BINARY_DIR}/api.md)
configure_file(${CMAKE_SOURCE_DIR}/tutorials.md.in ${CMAKE_BINARY_DIR}/tutorials.md)

ign_create_docs(
  API_MAINPAGE_MD "${CMAKE_BINARY_DIR}/api.md"
  TUTORIALS_MAINPAGE_MD "${CMAKE_BINARY_DIR}/tutorials.md"
  ADDITIONAL_INPUT_DIRS "${CMAKE_SOURCE_DIR}/src/plugins"
  TAGFILES
  "\"${CMAKE_SOURCE_DIR}/doc/qt.tag.xml=http://doc.qt.io/qt-5/\" \
   \"${IGNITION-MATH_DOXYGEN_TAGFILE} = ${IGNITION-MATH_API_URL}\" \
   \"${IGNITION-MSGS_DOXYGEN_TAGFILE} = ${IGNITION-MSGS_API_URL}\" \
   \"${IGNITION-TRANSPORT_DOXYGEN_TAGFILE} = ${IGNITION-TRANSPORT_API_URL}\" \
   \"${IGNITION-COMMON_DOXYGEN_TAGFILE} = ${IGNITION-COMMON_API_URL}\""
  )

file(COPY ${CMAKE_SOURCE_DIR}/tutorials/images/ DESTINATION ${CMAKE_BINARY_DIR}/doxygen/html/images/)
