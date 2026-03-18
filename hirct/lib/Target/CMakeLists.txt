add_library(hirctTarget EmitExpr.cpp GenFuncModel.cpp GenModel.cpp GenMakefile.cpp GenVerify.cpp GenDPIC.cpp GenWrapper.cpp GenFormat.cpp GenRAL.cpp GenCocotb.cpp GenTB.cpp GenDoc.cpp)
target_link_libraries(hirctTarget PUBLIC
    hirctAnalysis
    hirctSupport
    CIRCTTransforms
    CIRCTSeqTransforms
)
