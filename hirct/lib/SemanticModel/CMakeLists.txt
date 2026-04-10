add_library(hirctSemanticModel Builder.cpp Validation.cpp)
target_link_libraries(hirctSemanticModel PUBLIC
    hirctAnalysis
    CIRCTArc
    CIRCTHW
    CIRCTComb
    CIRCTSeq
    MLIRIR
    MLIRParser)
