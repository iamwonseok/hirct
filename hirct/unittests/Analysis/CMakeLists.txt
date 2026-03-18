add_executable(FSMAnalysisTest FSMAnalysisTest.cpp)
target_link_libraries(FSMAnalysisTest PRIVATE
    hirctAnalysis
    CIRCTComb CIRCTSeq CIRCTHW
    MLIRParser
    GTest::gtest_main)
add_test(NAME FSMAnalysisTest COMMAND FSMAnalysisTest)
