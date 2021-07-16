ign_gui_add_plugin(InteractiveViewControl
  SOURCES
    InteractiveViewControl.cc
  QT_HEADERS
    InteractiveViewControl.hh
  TEST_SOURCES
    # ViewControl_TEST.cc
  PUBLIC_LINK_LIBS
   ignition-rendering${IGN_RENDERING_VER}::ignition-rendering${IGN_RENDERING_VER}
)
