add_executable(SemanticModelTest SemanticModelTest.cpp)
target_link_libraries(SemanticModelTest PRIVATE
    hirctSemanticModel
    CIRCTArc
    CIRCTComb
    CIRCTHW
    CIRCTSeq
    MLIRFuncDialect
    MLIRParser
    GTest::gtest_main)
add_test(NAME SemanticModelTest COMMAND SemanticModelTest)
