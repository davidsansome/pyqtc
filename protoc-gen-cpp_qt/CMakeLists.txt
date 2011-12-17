set(SOURCES
  cpp_enum.cc
  cpp_enum_field.cc
  cpp_extension.cc
  cpp_field.cc
  cpp_file.cc
  cpp_generator.cc
  cpp_helpers.cc
  cpp_message.cc
  cpp_message_field.cc
  cpp_primitive_field.cc
  cpp_service.cc
  cpp_string_field.cc
  main.cc
)

add_executable(protoc-gen-cpp_qt
  ${SOURCES}
)

target_link_libraries(protoc-gen-cpp_qt
  ${PROTOBUF_LIBRARY}
  ${PROTOBUF_PROTOC_LIBRARY}
  ${QT_LIBRARIES}
)
