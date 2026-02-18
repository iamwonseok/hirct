# -*- Python -*-
# ============================================================================
# HIRCT lit configuration — integration tests (integration_test/)
#
# Phase 0 skeleton — CMake substitutions and XFAIL integration
# (parse_known_limitations.py) will be added in Phase 2 Task 205.
# ============================================================================

import os
import lit.formats

config.name = "hirct-integration"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".v", ".f", ".test"]
config.test_source_root = os.path.dirname(__file__)

# Phase 0: test_exec_root defaults to source root.
# Phase 1+: overridden by lit.site.cfg.py from CMake.
config.test_exec_root = os.path.dirname(__file__)

# Phase 2+: XFAIL integration with known-limitations.md
# config.xfail_paths = load_xfail_paths(...)
