add_subdirectory(Analysis)
add_subdirectory(Support)
add_subdirectory(Target)

add_custom_target(check-hirct-unit
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS FSMAnalysisTest GenFuncModelTest
  COMMENT "Running HIRCT unit tests")
