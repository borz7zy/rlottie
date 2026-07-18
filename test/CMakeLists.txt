project(rlottie_tests CXX)
find_package(GTest REQUIRED)

add_definitions(-DDEMO_DIR="${CMAKE_SOURCE_DIR}/example/resource/")
link_libraries(GTest::GTest GTest::Main)

add_executable(vectorTestSuite testsuite.cpp test_vrect.cpp test_vpath.cpp
    ${CMAKE_SOURCE_DIR}/src/vector/vbezier.cpp
    ${CMAKE_SOURCE_DIR}/src/vector/vdebug.cpp
    ${CMAKE_SOURCE_DIR}/src/vector/vmatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/vector/vpath.cpp)
target_include_directories(vectorTestSuite PRIVATE ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/vector ${CMAKE_SOURCE_DIR}/src/vector/pixman)
gtest_add_tests(vectorTestSuite "" AUTO)

add_executable(animationTestSuite testsuite.cpp
    test_lottieanimation.cpp test_lottieanimation_capi.cpp)
target_include_directories(animationTestSuite PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(animationTestSuite PRIVATE rlottie)
gtest_add_tests(animationTestSuite "" AUTO)

# Unit tests for the internal model types. These need the private headers but
# no resource files, so they run anywhere.
add_executable(modelTestSuite testsuite.cpp
    test_lottiemodel.cpp test_lottiefiltermodel.cpp)
target_include_directories(modelTestSuite PRIVATE ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/inc ${CMAKE_SOURCE_DIR}/src/lottie
    ${CMAKE_SOURCE_DIR}/src/vector)
# lottiefiltermodel.h uses std::variant; rlottie's own -std=c++17 is PRIVATE,
# so state the requirement here rather than relying on the compiler default.
target_compile_features(modelTestSuite PRIVATE cxx_std_17)
target_link_libraries(modelTestSuite PRIVATE rlottie)
gtest_add_tests(modelTestSuite "" AUTO)
