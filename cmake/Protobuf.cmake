find_package(Protobuf 2.4.1 REQUIRED)

function(PROTOBUF_GENERATE_PYTHON OUTPUT)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_PYTHON() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${OUTPUT})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(PATH ${FIL} PATH)

    list(APPEND ${OUTPUT} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}_pb2.py")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}_pb2.py"
      COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --python_out ${CMAKE_CURRENT_BINARY_DIR}
           --proto_path ${PATH}
           ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running Python protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${OUTPUT} ${${OUTPUT}} PARENT_SCOPE)
endfunction()


function(PROTOBUF_GENERATE_CPP_QT OUTPUT)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP_QT() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${OUTPUT})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(PATH ${FIL} PATH)

    list(APPEND ${OUTPUT} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")

    set(protoc_plugin ${CMAKE_BINARY_DIR}/protoc-gen-cpp_qt/protoc-gen-cpp_qt)

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
      COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --plugin=protoc-gen-cpp_qt=${protoc_plugin}
           --cpp_qt_out ${CMAKE_CURRENT_BINARY_DIR}
           --proto_path ${PATH}
           ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      DEPENDS ${protoc_plugin}
      COMMENT "Running C++/Qt protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${OUTPUT} ${${OUTPUT}} PARENT_SCOPE)
endfunction()
