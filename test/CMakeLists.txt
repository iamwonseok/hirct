configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY)

add_custom_target(check-hirct
  COMMAND ${LIT_COMMAND} -v
    --xunit-xml-output ${CMAKE_BINARY_DIR}/lit-check.xml
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS hirct-gen hirct-verify
  COMMENT "Running HIRCT lit tests")
