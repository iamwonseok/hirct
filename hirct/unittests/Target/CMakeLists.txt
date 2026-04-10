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
    hirctSemanticModel
    CIRCTArc CIRCTComb CIRCTSeq CIRCTHW
    MLIRFuncDialect
    MLIRParser
    GTest::gtest_main)
add_test(NAME CModelEmitterTest COMMAND CModelEmitterTest)
