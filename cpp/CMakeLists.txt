cmake_minimum_required(VERSION 3.15)
project(execution_engine_cpp)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

find_package(pybind11 REQUIRED)

set(SOURCES
    src/execution_engine.cpp
)

# Python module
pybind11_add_module(_execution_cpp 
    ${SOURCES}
    bindings/bindings.cpp
)

# Install to Python package
install(TARGETS _execution_cpp
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../src/execution_engine
)