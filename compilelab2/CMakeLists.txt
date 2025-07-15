cmake_minimum_required(VERSION 3.16)
project(CompilerLab CXX C)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-pedantic -Wall)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR})

set(BISON_OUTPUT_SRC "${CMAKE_CURRENT_BINARY_DIR}/parser.cpp")
set(BISON_OUTPUT_HDR "${CMAKE_CURRENT_BINARY_DIR}/parser.h")

add_custom_command(
    OUTPUT ${BISON_OUTPUT_SRC} ${BISON_OUTPUT_HDR}
    COMMAND ${BISON_EXECUTABLE} --defines=${BISON_OUTPUT_HDR} -o ${BISON_OUTPUT_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/grammar.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/grammar.y
    COMMENT "Running Bison on grammar.y"
)

set(FLEX_OUTPUT_SRC "${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp")

add_custom_command(
    OUTPUT ${FLEX_OUTPUT_SRC}
    COMMAND ${FLEX_EXECUTABLE} -o ${FLEX_OUTPUT_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l ${BISON_OUTPUT_HDR}
    COMMENT "Running Flex on lexer.l"
)

set(GENERATED_SOURCES
    ${BISON_OUTPUT_SRC}
    ${FLEX_OUTPUT_SRC}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Compilerlab2
    main.cpp
    codegen.cpp
    ${GENERATED_SOURCES}
)