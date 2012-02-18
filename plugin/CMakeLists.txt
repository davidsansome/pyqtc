include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QTC_SOURCE}/src
  ${QTC_SOURCE}/src/libs
  ${QTC_SOURCE}/src/libs/3rdparty/cplusplus
  ${QTC_SOURCE}/src/plugins
  ${QTC_SOURCE}/tools
)

link_directories(
  ${QTC_BINARY}/lib/qtcreator
  ${QTC_BINARY}/lib/qtcreator/plugins/Nokia
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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

set(SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
  closure.cpp
  completionassist.cpp
  constants.cpp
  hoverhandler.cpp
  messagehandler.cpp
  plugin.cpp
  projects.cpp
  pythoneditor.cpp
  pythoneditorfactory.cpp
  pythonfilter.cpp
  pythonicons.cpp
  pythonindenter.cpp
  waitforsignal.cpp
  workerclient.cpp
  workerpool.cpp
)

set(HEADERS
  closure.h
  hoverhandler.h
  messagehandler.h
  plugin.h
  projects.h
  pythoneditor.h
  pythonfilter.h
  workerpool.h
)

set(UI)

set(RESOURCES
  pyqtc.qrc
)

qt4_wrap_cpp(MOC ${HEADERS})
qt4_wrap_ui(UIC ${UI})
qt4_add_resources(QRC ${RESOURCES})

protobuf_generate_cpp_qt(PROTO_SOURCES
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
  ${QTC_AGGREGATION_LIB}
  ${QTC_BOTAN_LIB}
  ${QTC_CPLUSPLUS_LIB}
  ${QTC_CORE_LIB}
  ${QTC_EXTENSION_SYSTEM_LIB}
  ${QTC_LOCATOR_LIB}
  ${QTC_PROJECT_EXPLORER_LIB}
  ${QTC_UTILS_LIB}
  ${QT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${PROTOBUF_LIBRARY}
)

install(TARGETS pyqtc LIBRARY DESTINATION ${PYQTC_LIB_DIR})
install(FILES pyqtc.pluginspec DESTINATION ${PYQTC_LIB_DIR})
install(FILES python-2.7.2.qch python-2.7.2.qhc DESTINATION ${PYQTC_SHARE_DIR})
