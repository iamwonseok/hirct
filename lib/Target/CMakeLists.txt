add_library(hirctTarget GenModel.cpp GenMakefile.cpp)
target_link_libraries(hirctTarget PUBLIC hirctAnalysis)
