# -*- Python -*-
# ============================================================================
# HIRCT lit configuration — unit tests (test/)
# ============================================================================

import os
import lit.formats

config.name = "hirct"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".mlir", ".test"]
config.test_source_root = os.path.dirname(__file__)
config.excludes = ["fixtures"]

# test_exec_root: overridden by lit.site.cfg.py from CMake, or defaults
# to source root for standalone runs.
if not hasattr(config, "hirct_obj_root"):
    config.test_exec_root = os.path.dirname(__file__)

# Tool substitutions (set by CMake-generated lit.site.cfg.py)
if hasattr(config, "hirct_gen_path"):
    config.substitutions.append(("%hirct-gen", config.hirct_gen_path))
if hasattr(config, "hirct_verify_path"):
    config.substitutions.append(("%hirct-verify", config.hirct_verify_path))
if hasattr(config, "filecheck_path"):
    config.substitutions.append(("%FileCheck", config.filecheck_path))
