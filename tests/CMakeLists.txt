enable_testing()

add_executable(test_verifier test_verifier.cpp)
target_link_libraries(test_verifier PRIVATE bmapcore Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME verifier COMMAND test_verifier)

add_executable(test_bmapparser test_bmapparser.cpp)
target_link_libraries(test_bmapparser PRIVATE bmapcore Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME bmapparser COMMAND test_bmapparser)

add_executable(test_imagesource test_imagesource.cpp)
target_link_libraries(test_imagesource PRIVATE bmapcore Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME imagesource COMMAND test_imagesource)
