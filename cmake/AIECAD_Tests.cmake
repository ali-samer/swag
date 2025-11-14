option(USE_CMAKE_GOOGLE_TEST_INTEGRATION "If enabled, use the google test integration included in CMake." ON)

find_package(GMock MODULE REQUIRED)
if (USE_CMAKE_GOOGLE_TEST_INTEGRATION)
    include(GoogleTest OPTIONAL RESULT_VARIABLE HAVE_CMAKE_GTEST)
    enable_testing()
else ()
    set(HAVE_CMAKE_GTEST OFF)
endif ()

add_library(aiecad_test_support
    ${AIECAD_DIR}/test/common/TestMain.cpp
)

target_compile_definitions(aiecad_test_support
    PUBLIC
        ${LIBGMOCK_DEFINES}
)

# Extract gtest include directory from LIBGMOCK_INCLUDE_DIR (which has generator expression)
if(LIBGMOCK_INCLUDE_DIR)
    string(REGEX REPLACE "\\$<\\$<CONFIG:[^>]+>:" "" _gtest_path "${LIBGMOCK_INCLUDE_DIR}")
    string(REGEX REPLACE ">" "" _gtest_path "${_gtest_path}")
    if(_gtest_path AND EXISTS "${_gtest_path}")
        target_include_directories(aiecad_test_support
            SYSTEM
            PUBLIC
                "${_gtest_path}"
        )
        message(STATUS "Added gtest include to test_support: ${_gtest_path}")
    else()
        # Fallback to using the variable directly (might work in some cases)
        target_include_directories(aiecad_test_support
            SYSTEM
            PUBLIC
                ${LIBGMOCK_INCLUDE_DIR}
                ${GTEST_INCLUDE_DIRS}
        )
    endif()
else()
    target_include_directories(aiecad_test_support
        SYSTEM
        PUBLIC
            ${GTEST_INCLUDE_DIRS}
    )
endif()

# Link gtest targets
if(TARGET gtest::gtest)
    target_link_libraries(aiecad_test_support PUBLIC gtest::gtest)
endif()

target_link_libraries(aiecad_test_support
    PUBLIC
        aiecad_lib
        aiecad_deps
        ${LIBGMOCK_LIBRARIES}
)

# Add SPDLOG_FMT_EXTERNAL and /utf-8 flag
target_compile_definitions(aiecad_test_support PUBLIC SPDLOG_FMT_EXTERNAL)
target_compile_options(aiecad_test_support PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

include(cmake/utils/Functions.cmake)

aiecad_define_tests(
    DIRECTORY test/core/
        TEST StressTestLogger SOURCES LoggerTest.cpp
)