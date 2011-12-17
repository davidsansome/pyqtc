cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0011 NEW)

find_package(Qt4 4.5.0 REQUIRED QtCore QtGui QtNetwork)
find_package(Threads)
include(${QT_USE_FILE})

include(cmake/ParseArguments.cmake)
include(cmake/Protobuf.cmake)
include(cmake/Pylint.cmake)

set(WORKER_SOURCE_PATH ${CMAKE_BINARY_DIR}/parser/main.py)

add_subdirectory(parser)
add_subdirectory(plugin)
