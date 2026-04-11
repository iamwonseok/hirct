add_executable(GenFuncModelTest GenFuncModelTest.cpp)
target_link_libraries(GenFuncModelTest PRIVATE
    hirctTarget hirctAnalysis
    CIRCTComb CIRCTSeq CIRCTHW
    MLIRParser
    GTest::gtest_main)
add_test(NAME GenFuncModelTest COMMAND GenFuncModelTest)

add_executable(CModelEmitterTest CModelEmitterTest.cpp)
target_link_libraries(CModelEmitterTest PRIVATE
    hirctCModelEmitter
    hirctTarget hirctAnalysis
    hirctSemanticModel
    CIRCTArc CIRCTComb CIRCTSeq CIRCTHW CIRCTLLHD
    MLIRFuncDialect MLIRControlFlowDialect
    MLIRParser
    GTest::gtest_main)
add_test(NAME CModelEmitterTest COMMAND CModelEmitterTest)

add_executable(SystemCWrapperEmitterTest SystemCWrapperEmitterTest.cpp)
target_link_libraries(SystemCWrapperEmitterTest PRIVATE
    hirctSystemCWrapperEmitter
    hirctCModelEmitter
    hirctTarget hirctAnalysis
    hirctSemanticModel
    CIRCTArc CIRCTComb CIRCTSeq CIRCTHW CIRCTLLHD
    MLIRFuncDialect MLIRControlFlowDialect
    MLIRParser
    GTest::gtest_main)
add_test(NAME SystemCWrapperEmitterTest COMMAND SystemCWrapperEmitterTest)
