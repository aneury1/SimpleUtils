cmake_minimum_required(VERSION 3.16)
project(SimpleUtils VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SIMPLEUTILS_BUILD_TESTS "Build GoogleTest suite" OFF)
option(SIMPLEUTILS_FETCH_GTEST "Download GoogleTest if not installed" OFF)
option(SIMPLEUTILS_BUILD_EXAMPLES "Build examples" OFF)
option(SIMPLEUTILS_SHARED "Compile library as shared object" OFF)

set(SIMPLEUTILS_LIBRARY_TYPE STATIC)
if(SIMPLEUTILS_SHARED)
    set(SIMPLEUTILS_LIBRARY_TYPE SHARED)
endif()

set(BACKENDUTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
message("BACKENDUTILS_INCLUDE_DIR ${BACKENDUTILS_INCLUDE_DIR}")

find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)
find_package(Threads REQUIRED)

set(SIMPLEUTILS_SRC
      src/stringUtils.cpp
      src/encryptionUtils.cpp
      src/socketUtils.cpp
      src/httpUtils.cpp
      src/ipUtils.cpp
      src/sslHelper.cpp
      src/cmdUtils.cpp
)

function(setup_library library_name library_type)
  add_library(${library_name} ${library_type} ${SIMPLEUTILS_SRC})
  add_library(${library_name}::${library_name} ALIAS ${library_name})

  target_compile_features(${library_name} PUBLIC cxx_std_17)
  target_include_directories(${library_name} PUBLIC $<BUILD_INTERFACE:${BACKENDUTILS_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(${library_name} PUBLIC OpenSSL::Crypto Threads::Threads)
  if(WIN32)
    target_link_libraries(${library_name} PUBLIC ws2_32 iphlpapi)
  endif()
  set_target_properties(${library_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if(MSVC)
    target_compile_options(${library_name} PRIVATE /W4)
  else()
    target_compile_options(${library_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  install(TARGETS ${library_name} EXPORT ${library_name}Targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endfunction()

setup_library(simpleutils ${SIMPLEUTILS_LIBRARY_TYPE})

install(TARGETS simpleutils EXPORT SimpleUtilsTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/SimpleUtils)
configure_package_config_file(cmake/SimpleUtilsConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/SimpleUtilsConfig.cmake INSTALL_DESTINATION ${config_dir})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/SimpleUtilsConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(EXPORT SimpleUtilsTargets FILE SimpleUtilsTargets.cmake NAMESPACE simpleutils:: DESTINATION ${config_dir})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleUtilsConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/SimpleUtilsConfigVersion.cmake DESTINATION ${config_dir})
if(SIMPLEUTILS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
if(SIMPLEUTILS_BUILD_EXAMPLES)
  add_executable(simpleutils_example examples/main.cpp)
  target_link_libraries(simpleutils_example PRIVATE simpleutils::simpleutils)
endif()
