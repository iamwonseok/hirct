add_subdirectory(Analysis)

add_custom_target(check-hirct-unit
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS ModuleAnalyzerTest
  COMMENT "Running HIRCT unit tests")
