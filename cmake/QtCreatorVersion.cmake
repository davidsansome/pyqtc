macro(find_qt_creator_version output_var source_dir)
  file(READ ${source_dir}/qtcreator.pri pri_contents)
  string(REGEX MATCH "QTCREATOR_VERSION *= *([0-9.]+)" match "${pri_contents}")

  if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "Qt Creator version not found in source directory '${source_dir}'")
  endif()

  set(${output_var} ${CMAKE_MATCH_1})
endmacro()

macro(find_qt_creator_library output_var name)
  find_library(${output_var}
    NAMES ${name}
    PATHS ${QTC_BINARY}/lib/qtcreator
          ${QTC_BINARY}/lib/qtcreator/plugins/Nokia
  )
  find_file(${output_var}
    NAMES lib${name}.so
          lib${name}.so.1
          lib${name}.so.1.0.0
    PATHS ${QTC_BINARY}/lib/qtcreator
          ${QTC_BINARY}/lib/qtcreator/plugins/Nokia
  )
endmacro()
