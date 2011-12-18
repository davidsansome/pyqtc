set(QTC_SOURCE "/home/david/src/qt-creator" CACHE STRING "Qt Creator source directory")
set(QTC_BUILD  "/home/david/src/qt-creator" CACHE STRING "Qt Creator binary directory")

set(LIBRARY_OUTPUT_PATH ${QTC_BUILD}/lib/qtcreator/plugins/davidsansome)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QTC_BUILD}/src
  ${QTC_SOURCE}/src/libs
  ${QTC_SOURCE}/src/libs/3rdparty/cplusplus
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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
  @ONLY
)

set(SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
  closure.cpp
  codemodel.cpp
  completionassist.cpp
  messagehandler.cpp
  miniparser.cpp
  plugin.cpp
  pythonfilter.cpp
  workerpool.cpp
  workerreply.cpp
)

set(HEADERS
  closure.h
  codemodel.h
  messagehandler.h
  plugin.h
  pythonfilter.h
  workerpool.h
  workerreply.h
)

set(UI)

set(RESOURCES)

qt4_wrap_cpp(MOC ${HEADERS})
qt4_wrap_ui(UIC ${UI})
qt4_add_resources(QRC ${RESOURCES})

protobuf_generate_cpp_qt(PROTO_SOURCES
  ${CMAKE_SOURCE_DIR}/common/codemodel.proto
  ${CMAKE_SOURCE_DIR}/common/rpc.proto
)

add_library(pyqtc SHARED
  ${SOURCES}
  ${PROTO_SOURCES}
  ${MOC}
  ${UIC}
  ${QRC}
)

target_link_libraries(pyqtc
  Aggregation
  Botan
  CPlusPlus
  Core
  ExtensionSystem
  Locator
  ProjectExplorer
  Utils
  ${QT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${PROTOBUF_LIBRARY}
)

configure_file(pyqtc.pluginspec ${LIBRARY_OUTPUT_PATH} COPY_ONLY)
