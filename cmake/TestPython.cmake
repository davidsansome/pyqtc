find_program(PYTHON_EXECUTABLE python)

macro(test_python)
  parse_arguments(TEST_PYTHON
    "OUTPUT;TESTS;DEPENDS"
    ""
    ${ARGN}
  )

  foreach(file ${TEST_PYTHON_TESTS})
    get_filename_component(absolute_file ${file} ABSOLUTE)
    get_filename_component(file_we ${file} NAME_WE)

    set(output ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.dummy)

    add_custom_command(
      OUTPUT ${output}
      COMMAND ${PYTHON_EXECUTABLE} ${absolute_file} --quiet
      COMMAND ${CMAKE_COMMAND} -E touch ${output}
      DEPENDS ${absolute_file} ${TEST_PYTHON_DEPENDS}
      COMMENT "Running ${file_we}"
    )

    list(APPEND ${TEST_PYTHON_OUTPUT} ${output})
  endforeach()
endmacro()