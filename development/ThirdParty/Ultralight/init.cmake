set(WD_ULTRALIGHT_SUPPORT OFF CACHE BOOL "Enable support for Ultralight, a HTML/CSS/JS UI Engine.")

macro(wd_requires_ultralight)
    wd_requires_windows()
    wd_requires(WD_ULTRALIGHT_SUPPORT)
endmacro()

function(wd_link_target_ultralight TARGET_NAME)
    # Require that Ultralight Support is ticked.
    wd_requires_ultralight()

    # Link target to libraries.
    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/lib/AppCore.lib)
    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/lib/Ultralight.lib)
    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/lib/UltralightCore.lib)
    target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/lib/WebCore.lib)
    
    # Make target include ultralight headers.
    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include)

    # Make dlls export to target output.
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/AppCore.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/gio-2.0-0.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/glib-2.0-0.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/gmodule-2.0-0.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/gstreamer-full-1.0.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/gthread-2.0-0.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/Ultralight.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/UltralightCore.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin/WebCore.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )

 

    # Export needed resources in-order for Ultralight to work.
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/resources/icudt67l.dat  ${CMAKE_CURRENT_LIST_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/resources/cacert.pem  ${CMAKE_CURRENT_LIST_DIR}/resources
    WORKING_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )

endfunction(wd_link_target_ultralight)
