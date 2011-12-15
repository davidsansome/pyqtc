set(QTC_SOURCE "/home/david/src/qt-creator" CACHE STRING "Qt Creator source directory")
set(QTC_BUILD  "/home/david/src/qt-creator" CACHE STRING "Qt Creator binary directory")

set(LIBRARY_OUTPUT_PATH ${QTC_BUILD}/lib/qtcreator/plugins/davidsansome)

include_directories(
  ${QTC_BUILD}/src
  ${QTC_SOURCE}/src/libs
  ${QTC_SOURCE}/src/plugins
  ${QTC_SOURCE}/tools
)

link_directories(
  ${QTC_BUILD}/lib/qtcreator
  ${QTC_BUILD}/lib/qtcreator/plugins/Nokia
  ${LIBRARY_OUTPUT_PATH}
)

add_definitions(
  -DPYQTC_LIBRARY
  ${QT_DEFINITIONS}
)

set(SOURCES
  plugin.cpp
  pythonfilter.cpp
)

set(HEADERS
  plugin.h
  pythonfilter.h
)

set(UI)

set(RESOURCES)

qt4_wrap_cpp(MOC ${HEADERS})
qt4_wrap_ui(UIC ${UI})
qt4_add_resources(QRC ${RESOURCES})


add_library(pyqtc SHARED
  ${SOURCES}
  ${MOC}
  ${UIC}
  ${QRC}
)

target_link_libraries(pyqtc
  Aggregation
  ExtensionSystem
  Botan
  Utils
  Core
  Locator
  ${QT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

configure_file(pyqtc.pluginspec ${LIBRARY_OUTPUT_PATH} COPY_ONLY)
