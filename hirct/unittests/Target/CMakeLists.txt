add_executable(GenFuncModelTest GenFuncModelTest.cpp)
target_link_libraries(GenFuncModelTest PRIVATE
    hirctTarget hirctAnalysis
    CIRCTComb CIRCTSeq CIRCTHW
    MLIRParser
    GTest::gtest_main)
add_test(NAME GenFuncModelTest COMMAND GenFuncModelTest)
