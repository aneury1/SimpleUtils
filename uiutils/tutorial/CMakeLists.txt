cmake_minimum_required(VERSION 3.20)
project(uiutils_todo LANGUAGES CXX)

find_package(uiutils CONFIG REQUIRED)

add_executable(uiutils_todo_sample todo.cpp)
target_compile_features(uiutils_todo_sample PRIVATE cxx_std_17)
target_link_libraries(uiutils_todo_sample PRIVATE uiutils::uiutils)