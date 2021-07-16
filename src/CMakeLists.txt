include_directories(${PROJECT_BINARY_DIR})

set (sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Application.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/Conversions.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/Dialog.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/DragDropModel.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/Helpers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/PlottingInterface.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/Plugin.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/SearchModel.cc
  PARENT_SCOPE
)

set (gtest_sources
  Application_TEST
  Conversions_TEST
  DragDropModel_TEST
  Helpers_TEST
  GuiEvents_TEST
  MainWindow_TEST
  PlottingInterface_TEST
  Plugin_TEST
  SearchModel_TEST
)

if (MSVC)
  # Warning #4251 is the "dll-interface" warning that tells you when types used
  # by a class are not being exported. These generated source files have private
  # members that don't get exported, so they trigger this warning. However, the
  # warning is not important since those members do not need to be interfaced
  # with.
  set_source_files_properties(${sources} ${gtest_sources} COMPILE_FLAGS "/wd4251 /wd4146")
endif()

# this test currently fails on brew (issue #27)
# so disable it for now
if (NOT HAVE_IGN_TOOLS OR APPLE)
  list(REMOVE_ITEM gtest_sources ign_TEST.cc)
endif()

ign_build_tests(TYPE UNIT
                SOURCES
                  ${gtest_sources}
                LIB_DEPS
                  ${IGNITION-MATH_LIBRARIES}
                  TINYXML2::TINYXML2
)

add_subdirectory(cmd)
add_subdirectory(plugins)
