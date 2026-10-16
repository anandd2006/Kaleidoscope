cmake_minimum_required(VERSION 3.20)
# Match the static Windows runtime used by official LLVM development libraries.
cmake_policy(SET CMP0091 NEW)
project(ToyCompiler LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Prefer the compatible project-local package when no LLVM_DIR was supplied.
set(LOCAL_LLVM "${CMAKE_CURRENT_SOURCE_DIR}/tools/clang+llvm-20.1.8-x86_64-pc-windows-msvc")
if(NOT LLVM_DIR AND EXISTS "${LOCAL_LLVM}/lib/cmake/llvm/LLVMConfig.cmake")
    set(LLVM_DIR "${LOCAL_LLVM}/lib/cmake/llvm")
endif()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
add_executable(toycc src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp)
target_include_directories(toycc SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITION_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(toycc PRIVATE ${LLVM_DEFINITION_LIST})
llvm_map_components_to_libnames(LLVM_LIBS core support)
target_link_libraries(toycc PRIVATE ${LLVM_LIBS})

include(CTest)
if(BUILD_TESTING)
    add_test(NAME compiler_smoke
        COMMAND ${CMAKE_COMMAND}
        -DCOMPILER=$<TARGET_FILE:toycc>
        -DLLVM_TEST_TOOLS=${LLVM_TOOLS_BINARY_DIR}
        -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-output
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.cmake)
endif()
