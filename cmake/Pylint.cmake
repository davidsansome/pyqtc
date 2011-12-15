find_program(PYLINT_EXECUTABLE pylint)

macro(check_python)
  parse_arguments(CHECK_PYTHON
    "OUTPUT;PYTHONPATH;DEPENDS;SOURCE"
    ""
    ${ARGN}
  )

  foreach(file ${CHECK_PYTHON_SOURCE})
    get_filename_component(absolute_file ${file} ABSOLUTE)
    get_filename_component(file_we ${file} NAME_WE)

    set(output ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.dummy)

    add_custom_command(
      OUTPUT ${output}
      COMMAND env
        ARGS "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_SOURCE_DIR}:${CHECK_PYTHON_PYTHONPATH}"
             ${PYLINT_EXECUTABLE}
             --rcfile=${CMAKE_SOURCE_DIR}/cmake/pylint.rc
             ${absolute_file}
      COMMAND ${CMAKE_COMMAND} ARGS -E touch ${output}
      DEPENDS ${absolute_file} ${CHECK_PYTHON_DEPENDS}
      COMMENT "Checking Python source ${file}"
    )

    list(APPEND ${CHECK_PYTHON_OUTPUT} ${output})
  endforeach()
endmacro()
