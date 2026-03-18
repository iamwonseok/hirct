add_executable(hirct-gen main.cpp)
target_link_libraries(hirct-gen PRIVATE hirctTarget hirctAnalysis hirctSupport)
