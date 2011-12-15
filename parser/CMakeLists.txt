protobuf_generate_python(CODEMODEL_PB2
  ${CMAKE_SOURCE_DIR}/common/codemodel.proto
)

check_python(
  OUTPUT PYTHON
  DEPENDS ${CODEMODEL_PB2}
  SOURCE parse.py
)

add_custom_target(parser ALL
  DEPENDS ${PYTHON}
          ${CODEMODEL_PB2}
)