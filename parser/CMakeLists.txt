protobuf_generate_python(PROTOBUF_SOURCE
  ${CMAKE_SOURCE_DIR}/common/rpc.proto
)

check_python(
  OUTPUT PYTHON
  DEPENDS ${PROTOBUF_SOURCE}
  SOURCE __main__.py
         messagehandler.py
         symbolindex.py
)

set(ZIP_PATH ${CMAKE_CURRENT_BINARY_DIR}/worker.zip)

add_custom_command(
  OUTPUT ${ZIP_PATH}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/rope
    ${CMAKE_CURRENT_BINARY_DIR}/rope
  COMMAND ${CMAKE_COMMAND} -E remove ${ZIP_PATH}
  COMMAND ${ZIP_EXECUTABLE} --recurse-paths --must-match --quiet ${ZIP_PATH}
    __main__.py
    messagehandler.py
    rope/
    rpc_pb2.py
    symbolindex.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${PYTHON} ${PROTOBUF_SOURCE}
)

add_custom_target(parser ALL
  DEPENDS ${ZIP_PATH}
)

install(FILES ${ZIP_PATH} DESTINATION ${PYQTC_SHARE_DIR})
