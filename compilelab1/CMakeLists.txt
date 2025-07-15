cmake_minimum_required(VERSION 3.16)
project(lab01)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-pedantic)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
add_executable(Compilerlab1
    compilelab1.cpp
)
target_compile_features(Compilerlab1 PRIVATE cxx_std_14)