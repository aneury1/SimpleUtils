cmake_minimum_required(VERSION 3.20)
project(uiutils_todo LANGUAGES CXX)

add_executable(uiutils_todo_sample todo.cpp)

if(UIUTILS_FETCH_WXWIDGET)
    target_include_directories(uiutils_todo_sample PRIVATE ${UIUTILS_INCLUDE_DIR})
    target_link_directories(uiutils_todo_sample PRIVATE ${UIUTILS_LIBS_DIR})
else()
    find_package(uiutils CONFIG REQUIRED)
endif()

target_compile_features(uiutils_todo_sample PRIVATE cxx_std_17)
target_link_libraries(uiutils_todo_sample PRIVATE uiutils::uiutils)