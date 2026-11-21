cmake_minimum_required(VERSION 3.18)
project(simple_orm VERSION 2.0.0 LANGUAGES C CXX)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
option(SIMPLE_ORM_BUILD_TESTS "Build tests" ON)
option(SIMPLE_ORM_BUILD_EXAMPLES "Build examples" ON)
option(SIMPLE_ORM_REQUIRE_PREUPDATE "Require SQLite preupdate hook for column Live Events" ON)
set(SIMPLE_ORM_SQLITE_PROVIDER "SYSTEM" CACHE STRING "SYSTEM (SQLite >= 3.37) or FETCH_3540100")
set_property(CACHE SIMPLE_ORM_SQLITE_PROVIDER PROPERTY STRINGS SYSTEM FETCH_3540100)
if(SIMPLE_ORM_SQLITE_PROVIDER STREQUAL "FETCH_3540100")
    include(FetchContent)
    # SQLite public-domain amalgamation. Network required only at configure time.
    FetchContent_Declare(simple_orm_sqlite URL "https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip"
        URL_HASH SHA3_256=92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(simple_orm_sqlite)
    add_library(simple_orm_sqlite3 STATIC "${simple_orm_sqlite_SOURCE_DIR}/sqlite3.c")
    target_include_directories(simple_orm_sqlite3 PUBLIC
        $<BUILD_INTERFACE:${simple_orm_sqlite_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/simple_orm/vendor>)
    target_compile_definitions(simple_orm_sqlite3 PRIVATE SQLITE_ENABLE_PREUPDATE_HOOK=1 SQLITE_THREADSAFE=1)
    if(UNIX AND NOT APPLE)
        target_link_libraries(simple_orm_sqlite3 PUBLIC dl m)
    endif()
    add_library(SQLite::SQLite3 ALIAS simple_orm_sqlite3)
else()
    find_package(SQLite3 3.37 REQUIRED)
endif()

add_library(simple_orm src/database.cpp)
add_library(simple_orm::simple_orm ALIAS simple_orm)
target_compile_features(simple_orm PUBLIC cxx_std_17)
set_target_properties(simple_orm PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_include_directories(simple_orm PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(simple_orm PUBLIC SQLite::SQLite3)

if(SIMPLE_ORM_SQLITE_PROVIDER STREQUAL "FETCH_3540100")
    target_compile_definitions(simple_orm PUBLIC SIMPLE_ORM_HAS_PREUPDATE=1 PRIVATE SQLITE_ENABLE_PREUPDATE_HOOK=1)
else()
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES SQLite::SQLite3)
    check_cxx_source_compiles("#define SQLITE_ENABLE_PREUPDATE_HOOK 1\n#include <sqlite3.h>\nint main(){ auto f = &sqlite3_preupdate_hook; return f==nullptr; }" SIMPLE_ORM_SQLITE_LINK_PREUPDATE)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(SIMPLE_ORM_SQLITE_LINK_PREUPDATE)
        target_compile_definitions(simple_orm PUBLIC SIMPLE_ORM_HAS_PREUPDATE=1 PRIVATE SQLITE_ENABLE_PREUPDATE_HOOK=1)
    elseif(SIMPLE_ORM_REQUIRE_PREUPDATE)
        message(FATAL_ERROR "SQLite has no preupdate hook; use FETCH_3540100 or -DSIMPLE_ORM_REQUIRE_PREUPDATE=OFF (row-level events only)")
    endif()
endif()

if(SIMPLE_ORM_BUILD_EXAMPLES)
    add_executable(simple_orm_example examples/contacts.cpp)
    target_link_libraries(simple_orm_example PRIVATE simple_orm::simple_orm)
endif()
if(SIMPLE_ORM_BUILD_TESTS)
    enable_testing()
    add_executable(simple_orm_tests tests/test_main.cpp)
    target_link_libraries(simple_orm_tests PRIVATE simple_orm::simple_orm)
    add_test(NAME simple_orm_tests COMMAND simple_orm_tests)
endif()
install(TARGETS simple_orm EXPORT simple_ormTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/simple_orm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT simple_ormTargets FILE simple_ormTargets.cmake NAMESPACE simple_orm:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simple_orm)

# Relocatable CMake package for consumers of an installed library.
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/simple_ormConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/simple_ormConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/simple_orm")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/simple_ormConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/simple_ormConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/simple_ormConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/simple_orm")
if(SIMPLE_ORM_SQLITE_PROVIDER STREQUAL "FETCH_3540100")
    install(TARGETS simple_orm_sqlite3 EXPORT simple_ormTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES "${simple_orm_sqlite_SOURCE_DIR}/sqlite3.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/simple_orm/vendor")
endif()
