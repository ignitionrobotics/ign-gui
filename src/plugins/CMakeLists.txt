#################################################
# ign_gui_add_library (<library_name>
#              SOURCES <sources>
#              [PUBLIC_LINK_LIBS <libraries...>]
#              [PRIVATE_LINK_LIBS <libraries...>])
#
# Add a plugin library to Ignition GUI.
#
# <library_name> Required. Name of the library
#
# [SOURCES]: Specify the source files for the plugin.
#
# [QT_HEADERS]: Qt headers that need to be MOC'ed
#
# [PUBLIC_LINK_LIBS]: Specify a list of libraries to be publicly linked.
#
# [PRIVATE_LINK_LIBS]: Specify a list of libraries to be privately linked.
#
function(ign_gui_add_library library_name)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs SOURCES QT_HEADERS PUBLIC_LINK_LIBS PRIVATE_LINK_LIBS)

  cmake_parse_arguments(ign_gui_add_library "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  QT5_WRAP_CPP(${library_name}_headers_MOC ${ign_gui_add_library_QT_HEADERS})
  QT5_ADD_RESOURCES(${library_name}_RCC ${library_name}.qrc)

  add_library(${library_name} SHARED
    ${ign_gui_add_library_SOURCES}
    ${${library_name}_headers_MOC}
    ${${library_name}_RCC}
  )
  target_link_libraries(${library_name}
    PUBLIC
      ${PROJECT_LIBRARY_TARGET_NAME}
      TINYXML2::TINYXML2
      ${ign_gui_add_library_PUBLIC_LINK_LIBS}
    PRIVATE
      ${ign_gui_add_library_PRIVATE_LINK_LIBS}
  )
endfunction()

#################################################
# ign_gui_add_plugin(<plugin_name>
#              SOURCES <sources>
#              [PUBLIC_LINK_LIBS <libraries...>]
#              [PRIVATE_LINK_LIBS <libraries...>])
#
# Add a plugin to Ignition GUI.
#
# <plugin_name> Required. Name of the plugin.
#
# [SOURCES]: Specify the source files for the system.
#
# [QT_HEADERS]: Qt headers that need to be MOC'ed
#
# [TEST_SOURCES]: Source files for unit tests.
#
# [PUBLIC_LINK_LIBS]: Specify a list of libraries to be publicly linked.
#
# [PRIVATE_LINK_LIBS]: Specify a list of libraries to be privately linked.
#
function(ign_gui_add_plugin plugin_name)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs
    SOURCES
    QT_HEADERS
    TEST_SOURCES
    PUBLIC_LINK_LIBS
    PRIVATE_LINK_LIBS
  )

  cmake_parse_arguments(ign_gui_add_plugin "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  ign_gui_add_library(${plugin_name}
    SOURCES ${ign_gui_add_plugin_SOURCES}
    QT_HEADERS ${ign_gui_add_plugin_QT_HEADERS}
    PUBLIC_LINK_LIBS ${ign_gui_add_plugin_PUBLIC_LINK_LIBS}
    PRIVATE_LINK_LIBS ${ign_gui_add_plugin_PRIVATE_LINK_LIBS} ignition-plugin${IGN_PLUGIN_VER}::register
  )

  if(ign_gui_add_plugin_TEST_SOURCES)
    ign_build_tests(TYPE UNIT
      SOURCES
        ${ign_gui_add_plugin_TEST_SOURCES}
      LIB_DEPS
        ${IGNITION-GUI_LIBRARIES}
        TINYXML2::TINYXML2
        ${plugin_name}
      INCLUDE_DIRS
        # Used to make internal source file headers visible to the unit tests
        ${CMAKE_CURRENT_SOURCE_DIR}
        # Used to make test-directory headers visible to the unit tests
        ${PROJECT_SOURCE_DIR}
        # Used to make test_config.h visible to the unit tests
        ${PROJECT_BINARY_DIR})
  endif()

  if (MSVC)
    # Warning #4251 is the "dll-interface" warning that tells you when types used
    # by a class are not being exported. These generated source files have private
    # members that don't get exported, so they trigger this warning. However, the
    # warning is not important since those members do not need to be interfaced
    # with.
    set_source_files_properties(
        ${ign_gui_add_plugin_SOURCES}
        ${ign_gui_add_plugin_TEST_SOURCES}
        COMPILE_FLAGS "/wd4251")
  endif()

  install (TARGETS ${plugin_name} DESTINATION ${IGNITION_GUI_PLUGIN_INSTALL_DIR})
endfunction()

# Plugins
add_subdirectory(camera_tracking)
add_subdirectory(grid_3d)
add_subdirectory(grid_config)
add_subdirectory(image_display)
add_subdirectory(interactive_view_control)
add_subdirectory(key_publisher)
add_subdirectory(plotting)
add_subdirectory(publisher)
add_subdirectory(minimal_scene)
add_subdirectory(scene3d)
add_subdirectory(screenshot)
add_subdirectory(tape_measure)
add_subdirectory(topic_echo)
add_subdirectory(topic_viewer)
add_subdirectory(transport_scene_manager)
add_subdirectory(world_control)
add_subdirectory(world_stats)
