###########################
## PIX Support (D3D12)
###########################
if(WD_CMAKE_PLATFORM_WINDOWS)
        set(WD_BUILD_PIX ON CACHE BOOL "Add support for WinPixEventRuntime.")
    else()
        set(WD_BUILD_PIX OFF CACHE BOOL "Add support for WinPixEventRuntime.")
endif()

macro(wd_requires_pix)
    wd_requires_windows()
    wd_requires(WD_BUILD_PIX)
endmacro()
# This function links to pix, and make sure all required dll's are copied over to the output.
function(wd_link_target_pix TARGET_NAME)

    wd_requires_pix()

    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dll/WinPixEventRuntime.lib)
    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dll/WinPixEventRuntime_UAP.lib)

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/dll/WinPixEventRuntime.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/dll/WinPixEventRuntime_UAP.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

endfunction()