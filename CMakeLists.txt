cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0011 NEW)

# Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Find dependencies
find_package(Qt4 4.5.0 REQUIRED QtCore QtGui QtNetwork)
find_package(Threads)
find_program(ZIP_EXECUTABLE zip)
find_program(PYLINT_EXECUTABLE pylint)
find_program(PYTHON_EXECUTABLE python)
include(${QT_USE_FILE})

# Include support files
include(cmake/ParseArguments.cmake)
include(cmake/Protobuf.cmake)
include(cmake/Pylint.cmake)
include(cmake/QtCreatorVersion.cmake)
include(cmake/TestPython.cmake)

# Figure out some important directories
set(QTC_SOURCE "/home/david/src/qt-creator" CACHE STRING "Qt Creator source directory")
set(QTC_BINARY "/home/david/src/qt-creator" CACHE STRING "Qt Creator binary directory")

set(CMAKE_INSTALL_PREFIX ${QTC_BINARY} CACHE PATH "Install dir" FORCE)

set(PYQTC_LIB_DIR   "lib/qtcreator/plugins/davidsansome")
set(PYQTC_SHARE_DIR "share/qtcreator/pyqtc")

# Get the Qt Creator version
find_qt_creator_version(QTC_VERSION ${QTC_SOURCE})

if(QTC_VERSION VERSION_GREATER 2.4.1)
  set(QTC_HAS_CORE_ID ON)
endif()

# Find Qt Creator libraries
find_qt_creator_library(QTC_AGGREGATION_LIB      Aggregation)
find_qt_creator_library(QTC_BOTAN_LIB            Botan)
find_qt_creator_library(QTC_CPLUSPLUS_LIB        CPlusPlus)
find_qt_creator_library(QTC_CORE_LIB             Core)
find_qt_creator_library(QTC_EXTENSION_SYSTEM_LIB ExtensionSystem)
find_qt_creator_library(QTC_LOCATOR_LIB          Locator)
find_qt_creator_library(QTC_PROJECT_EXPLORER_LIB ProjectExplorer)
find_qt_creator_library(QTC_UTILS_LIB            Utils)

# Subdirectories
add_subdirectory(protoc-gen-cpp_qt)
add_subdirectory(parser)
add_subdirectory(plugin)
