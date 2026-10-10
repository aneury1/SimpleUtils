cmake_minimum_required(VERSION 3.20)
project(uiutils VERSION 0.6.0 LANGUAGES CXX)
option(UIUTILS_BUILD_LIBRARY "Build the wxWidgets backend" ON)
option(UIUTILS_BUILD_SAMPLE "Build the samples" ON)
option(UIUTILS_BUILD_TESTS "Build GoogleTest tests" OFF)
option(UIUTILS_BUILD_GUI_TESTS "Build integration tests requiring a display" OFF)
option(UIUTILS_FETCH_GTEST "Download GoogleTest if it is not installed" OFF)
option(UIUTILS_ENABLE_CODES "Enable QR and barcode images (ZXing-C++ 2.x)" OFF)
option(UIUTILS_ENABLE_CHARTS "Enable internal bar/line chart rendering" ON)
option(UIUTILS_ENABLE_PDF "Enable PDF rendering (Poppler C++)" OFF)
option(UIUTILS_FETCH_ZXING "Fetch pinned ZXing-C++ instead of using a system installation" OFF)
if(UIUTILS_BUILD_LIBRARY)
    find_package(wxWidgets REQUIRED COMPONENTS core base)
    add_library(uiutils STATIC src/layout.cpp src/UiApplication.cpp src/activity.cpp src/Elements.cpp src/Style.cpp
        src/RenderSource.cpp src/RenderCodes.cpp src/RenderCharts.cpp src/RenderPdf.cpp)
    add_library(uiutils::uiutils ALIAS uiutils)
    target_compile_features(uiutils PUBLIC cxx_std_17)
    target_compile_definitions(uiutils PRIVATE
        UIUTILS_ENABLE_CODES=$<BOOL:${UIUTILS_ENABLE_CODES}>
        UIUTILS_ENABLE_CHARTS=$<BOOL:${UIUTILS_ENABLE_CHARTS}>
        UIUTILS_ENABLE_PDF=$<BOOL:${UIUTILS_ENABLE_PDF}>)
    if(UIUTILS_ENABLE_CODES)
        if(UIUTILS_FETCH_ZXING)
            include(FetchContent)
            set(ZXING_WRITERS ON CACHE BOOL "" FORCE)
            set(ZXING_READERS OFF CACHE BOOL "" FORCE)
            set(ZXING_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(ZXING_UNIT_TESTS OFF CACHE BOOL "" FORCE)
            set(ZXING_BLACKBOX_TESTS OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(zxing
                GIT_REPOSITORY https://github.com/zxing-cpp/zxing-cpp.git
                GIT_TAG v2.3.0)
            FetchContent_MakeAvailable(zxing)
        else()
            find_package(ZXing 2.2 REQUIRED CONFIG)
            if(DEFINED ZXing_VERSION AND ZXing_VERSION VERSION_GREATER_EQUAL 3)
                message(FATAL_ERROR "This backend uses ZXing-C++ 2.x. Enable UIUTILS_FETCH_ZXING to use the pinned 2.3.0 source instead.")
            endif()
            include(CheckCXXSourceCompiles)
            set(CMAKE_REQUIRED_LIBRARIES ZXing::ZXing)
            check_cxx_source_compiles("
                #if __has_include(<ZXing/MultiFormatWriter.h>)
                #include <ZXing/MultiFormatWriter.h>
                #include <ZXing/BitMatrix.h>
                #else
                #include <MultiFormatWriter.h>
                #include <BitMatrix.h>
                #endif
                int main() {
                    auto code = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode).encode(\"test\", 100, 100);
                    return code.width() ? 0 : 1;
                }" UIUTILS_ZXING_HAS_WRITER)
            unset(CMAKE_REQUIRED_LIBRARIES)
            if(NOT UIUTILS_ZXING_HAS_WRITER)
                message(FATAL_ERROR "Installed ZXing lacks the required writer API. Enable UIUTILS_FETCH_ZXING or install ZXing 2.x with writers.")
            endif()
        endif()
        target_link_libraries(uiutils PRIVATE ZXing::ZXing)
    endif()
    if(UIUTILS_ENABLE_PDF)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
        target_link_libraries(uiutils PRIVATE PkgConfig::POPPLER_CPP)
    endif()
    target_include_directories(uiutils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    # Backend-only wxWidgets dependency: public headers use standard C++ types.
    target_include_directories(uiutils PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(uiutils PRIVATE ${wxWidgets_DEFINITIONS})
    foreach(definition IN LISTS wxWidgets_DEFINITIONS_DEBUG)
        target_compile_definitions(uiutils PRIVATE $<$<CONFIG:Debug>:${definition}>)
    endforeach()
    if(wxWidgets_CXX_FLAGS)
        separate_arguments(wx_flags NATIVE_COMMAND "${wxWidgets_CXX_FLAGS}")
        target_compile_options(uiutils PRIVATE ${wx_flags})
    endif()
    if(wxWidgets_LIBRARY_DIRS)
        target_link_directories(uiutils PUBLIC ${wxWidgets_LIBRARY_DIRS})
    endif()
    target_link_libraries(uiutils PUBLIC ${wxWidgets_LIBRARIES})
    set_target_properties(uiutils PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
endif()
if(UIUTILS_BUILD_SAMPLE)
    if(NOT UIUTILS_BUILD_LIBRARY)
        message(FATAL_ERROR "Samples require UIUTILS_BUILD_LIBRARY=ON")
    endif()
    add_subdirectory(sample)
endif()
if(UIUTILS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
elseif(UIUTILS_BUILD_GUI_TESTS)
    message(FATAL_ERROR "GUI tests require UIUTILS_BUILD_TESTS=ON")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

target_include_directories(uiutils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Install the static library and public headers.
install(TARGETS uiutils
    EXPORT uiutilsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/uiutils
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export the uiutils::uiutils target.
install(EXPORT uiutilsTargets
    FILE uiutilsTargets.cmake
    NAMESPACE uiutils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uiutils
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uiutilsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uiutilsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uiutils
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/uiutilsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/uiutilsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/uiutilsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uiutils
)
