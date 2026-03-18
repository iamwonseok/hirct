# -*- Python -*-
# ============================================================================
# HIRCT lit configuration — integration tests (integration_test/)
# ============================================================================

import os
import shutil
import lit.formats

config.name = "hirct-integration"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".v", ".f", ".test"]
config.test_source_root = os.path.dirname(__file__)
config.excludes = ["Inputs"]

if not hasattr(config, "hirct_obj_root"):
    config.test_exec_root = os.path.dirname(__file__)

if hasattr(config, "hirct_gen_path"):
    config.substitutions.append(("%hirct-gen", config.hirct_gen_path))
if hasattr(config, "hirct_verify_path"):
    config.substitutions.append(("%hirct-verify", config.hirct_verify_path))
if hasattr(config, "filecheck_path"):
    config.substitutions.append(("%FileCheck", config.filecheck_path))
if not hasattr(config, "hirct_output_dir"):
    config.hirct_output_dir = os.path.join(os.path.dirname(__file__), "..", "..", "output")
config.substitutions.append(("%output-dir", os.path.realpath(config.hirct_output_dir)))

_not_path = shutil.which("not")
if _not_path is None:
    import glob as _glob

    _candidates = _glob.glob("/usr/lib/llvm-*/bin/not")
    if _candidates:
        _not_path = sorted(_candidates)[-1]
if _not_path:
    _llvm_bin = os.path.dirname(_not_path)
    config.environment["PATH"] = (
        _llvm_bin
        + os.pathsep
        + config.environment.get("PATH", os.environ.get("PATH", ""))
    )

# Feature detection for optional tools
if shutil.which("verilator"):
    config.available_features.add("verilator")

_output_dir = os.path.realpath(config.hirct_output_dir)
if os.path.isdir(os.path.join(_output_dir, "lib")):
    config.available_features.add("output-populated")
