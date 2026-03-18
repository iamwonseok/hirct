configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY)

add_custom_target(check-hirct-integration
  COMMAND ${LIT_COMMAND} -v
    --xunit-xml-output ${CMAKE_BINARY_DIR}/lit-integration.xml
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS hirct-gen hirct-verify
  COMMENT "Running HIRCT integration tests")
