ign_gui_add_plugin(MarkerManager
  SOURCES
    MarkerManager.cc
  QT_HEADERS
    MarkerManager.hh
  PUBLIC_LINK_LIBS
   ignition-rendering${IGN_RENDERING_VER}::ignition-rendering${IGN_RENDERING_VER}
)
