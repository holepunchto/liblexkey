set(tests
  buffers
  specific
  uint
  int
  string
  bool
  date
  mixed
  range
)

foreach(test IN LISTS tests)
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} PRIVATE lexkey)
  add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  set_tests_properties(${test} PROPERTIES TIMEOUT 30)
endforeach()
