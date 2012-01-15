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

add_custom_command(
  OUTPUT ${WORKER_ZIP_PATH}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/rope
    ${CMAKE_CURRENT_BINARY_DIR}/rope
  COMMAND ${CMAKE_COMMAND} -E remove ${WORKER_ZIP_PATH}
  COMMAND ${ZIP_EXECUTABLE} --recurse-paths --must-match --quiet ${WORKER_ZIP_PATH}
    __main__.py
    messagehandler.py
    rope/
    rpc_pb2.py
    symbolindex.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${PYTHON} ${PROTOBUF_SOURCE}
)

add_custom_target(parser ALL
  DEPENDS ${WORKER_ZIP_PATH}
)