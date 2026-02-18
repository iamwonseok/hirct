# -*- Python -*-
# ============================================================================
# HIRCT lit configuration — unit tests (test/)
#
# Phase 0 skeleton — CMake substitutions (config.hirct_obj_root,
# config.hirct_gen_path, etc.) will be added in Phase 1 Task 100 via
# a CMake-generated lit.site.cfg.py.
# ============================================================================

import os
import lit.formats

config.name = "hirct"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".mlir", ".test"]
config.test_source_root = os.path.dirname(__file__)

# Phase 0: test_exec_root defaults to source root.
# Phase 1+: overridden by lit.site.cfg.py from CMake.
config.test_exec_root = os.path.dirname(__file__)
