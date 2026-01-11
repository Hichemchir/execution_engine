cmake_minimum_required(VERSION 3.15)
project(execution_engine_cpp)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

find_package(pybind11 REQUIRED)

set(SOURCES
    src/execution_engine.cpp
)

# ============================================================================
# PYTHON MODULE
# ============================================================================

pybind11_add_module(_execution_cpp 
    ${SOURCES}
    bindings/bindings.cpp
)

# Install to Python package
install(TARGETS _execution_cpp
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../src/execution_engine
)

# ============================================================================
# GOOGLE TEST SETUP
# ============================================================================

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

# Don't install GoogleTest system-wide
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)  # FIX: Disable gtest install
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ============================================================================
# TEST EXECUTABLE
# ============================================================================

add_executable(test_twap
    test/test_twap.cpp  
    ${SOURCES}  
)

target_link_libraries(test_twap  
    GTest::gtest_main                
)

include(GoogleTest)
gtest_discover_tests(test_twap)