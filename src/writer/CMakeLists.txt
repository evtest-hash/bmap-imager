add_executable(bmapwriter main.cpp)
set_target_properties(bmapwriter PROPERTIES OUTPUT_NAME "bmap-writer")
target_link_libraries(bmapwriter PRIVATE bmapplatform)
