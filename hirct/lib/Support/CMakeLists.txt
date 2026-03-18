add_library(hirctSupport PathUtils.cpp VerilogLoader.cpp VerilatorPreprocessor.cpp)
target_link_libraries(hirctSupport PUBLIC
    hirctTransforms
    CIRCTImportVerilog
    CIRCTMooreToCore
    CIRCTMoore
    CIRCTLLHDTransforms
    CIRCTSupport
    MLIRIR
    MLIRParser
    MLIRPass
    MLIRSupport
    MLIRTransforms
    MLIRSCFToControlFlow
    LLVMOption
)
