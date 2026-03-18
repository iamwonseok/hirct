add_library(hirctAnalysis IRAnalysis.cpp FSMAnalysis.cpp)
target_link_libraries(hirctAnalysis PUBLIC
    hirctSupport
    CIRCTHW
    CIRCTComb
    CIRCTSeq
    CIRCTLLHD
    CIRCTSim
    CIRCTSV
)
