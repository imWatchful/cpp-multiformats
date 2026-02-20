add_library(varint INTERFACE)
target_include_directories(varint INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

option(VARINT_BUILD_TESTS "Build varint tests" OFF)

if (VARINT_BUILD_TESTS)
    find_package(GTest QUIET)

    if (NOT GTest_FOUND)
        message(STATUS "GTest not found, using submodule")

        add_subdirectory(
            ${CMAKE_SOURCE_DIR}/external/googletest
            ${CMAKE_BINARY_DIR}/external/googletest
        )
    else()
        message(STATUS "Using system-installed GTest")
    endif()

    add_executable(varint_tests
        tests/varint_test.cpp
    )

    target_link_libraries(varint_tests
        PRIVATE
        varint
        GTest::gtest
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(varint_tests)

endif()