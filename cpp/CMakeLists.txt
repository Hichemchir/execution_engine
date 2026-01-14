cmake_minimum_required(VERSION 3.15)
project(execution_engine_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(pybind11 REQUIRED)

include(FetchContent)

# ============================================================================
# FETCH DEPENDENCIES
# ============================================================================

# IXWebSocket
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.4
)
set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_ZLIB ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ixwebsocket)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

message(STATUS "✅ All dependencies fetched successfully")

# ============================================================================
# SOURCES
# ============================================================================

set(SOURCES
    src/execution_engine.cpp
    src/feed_handler.cpp
)

# ============================================================================
# PYTHON MODULE
# ============================================================================

pybind11_add_module(_execution_cpp 
    ${SOURCES}
    bindings/bindings.cpp
)

target_link_libraries(_execution_cpp PRIVATE
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
)

target_compile_definitions(_execution_cpp PRIVATE HAVE_FEED_HANDLER)

# Install to Python package
install(TARGETS _execution_cpp
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../src/execution_engine
)

# ============================================================================
# TEST EXECUTABLES
# ============================================================================

enable_testing()

# Test 1: TWAP tests
add_executable(test_twap
    test/test_twap.cpp  
    ${SOURCES}
)

target_link_libraries(test_twap  
    GTest::gtest_main    
    GTest::gmock_main
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
)

include(GoogleTest)
gtest_discover_tests(test_twap)

# Test 2: Feed handler tests
add_executable(test_feed_handler
    test/test_feed_handler.cpp
    ${SOURCES}
)

target_link_libraries(test_feed_handler
    GTest::gtest_main
    GTest::gmock_main
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
)

# Add test utilities include path
target_include_directories(test_feed_handler PRIVATE test)

gtest_discover_tests(test_feed_handler)

message(STATUS "test_feed_handler configured")