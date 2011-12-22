protobuf_generate_python(PROTOBUF_SOURCE
  ${CMAKE_SOURCE_DIR}/common/codemodel.proto
  ${CMAKE_SOURCE_DIR}/common/rpc.proto
)

check_python(
  OUTPUT PYTHON
  DEPENDS ${PROTOBUF_SOURCE}
  SOURCE main.py
         parse.py
         typedes.py
         worker.py
)

add_custom_target(parser ALL
  DEPENDS ${PYTHON}
          ${PROTOBUF_SOURCE}
)