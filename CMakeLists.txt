cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0011 NEW)

# Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

find_package(Qt4 4.5.0 REQUIRED QtCore QtGui QtNetwork)
find_package(Threads)
include(${QT_USE_FILE})

find_program(ZIP_EXECUTABLE zip)
find_program(PYLINT_EXECUTABLE pylint)
find_program(PYTHON_EXECUTABLE python)

include(cmake/ParseArguments.cmake)
include(cmake/Protobuf.cmake)
include(cmake/Pylint.cmake)
include(cmake/TestPython.cmake)

set(QTC_SOURCE "/home/david/src/qt-creator" CACHE STRING "Qt Creator source directory")
set(QTC_BINARY "/home/david/src/qt-creator" CACHE STRING "Qt Creator binary directory")

set(CMAKE_INSTALL_PREFIX ${QTC_BINARY} CACHE PATH "Install dir" FORCE)

set(PYQTC_LIB_DIR   "lib/qtcreator/plugins/davidsansome")
set(PYQTC_SHARE_DIR "share/qtcreator/pyqtc")

add_subdirectory(protoc-gen-cpp_qt)
add_subdirectory(parser)
add_subdirectory(plugin)
