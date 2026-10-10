add_executable(uiutils_sample main.cpp)
target_link_libraries(uiutils_sample PRIVATE uiutils::uiutils)
add_custom_command(TARGET uiutils_sample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:uiutils_sample>/assets)
add_executable(uiutils_controls_gallery controls_gallery.cpp)
target_link_libraries(uiutils_controls_gallery PRIVATE uiutils::uiutils)

if(UIUTILS_ENABLE_CODES)
    add_executable(uiutils_codes_sample codes.cpp)
    target_link_libraries(uiutils_codes_sample PRIVATE uiutils::uiutils)
endif()
if(UIUTILS_ENABLE_CHARTS)
    add_executable(uiutils_charts_sample charts.cpp)
    target_link_libraries(uiutils_charts_sample PRIVATE uiutils::uiutils)
endif()
if(UIUTILS_ENABLE_PDF)
    add_executable(uiutils_pdf_sample pdf_viewer.cpp)
    target_link_libraries(uiutils_pdf_sample PRIVATE uiutils::uiutils)
    add_custom_command(TARGET uiutils_pdf_sample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:uiutils_pdf_sample>/assets)
endif()
