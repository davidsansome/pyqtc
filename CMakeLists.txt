cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0011 NEW)

find_package(Qt4 4.5.0 REQUIRED QtCore QtGui QtNetwork)
find_package(Threads)
find_program(ZIP_EXECUTABLE zip)
include(${QT_USE_FILE})

include(cmake/ParseArguments.cmake)
include(cmake/Protobuf.cmake)
include(cmake/Pylint.cmake)
include(cmake/TestPython.cmake)

set(WORKER_ZIP_PATH ${CMAKE_BINARY_DIR}/parser/worker.zip)

add_subdirectory(protoc-gen-cpp_qt)
add_subdirectory(parser)
add_subdirectory(plugin)
