# Sentry Utilities

function(wd_sentry_grab_build TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sentry.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/crashpad_handler.exe $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/crashpad_wer.dll $<TARGET_FILE_DIR:${TARGET_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )
endfunction()

function(wd_sentry_target_build TARGET_NAME)
    wd_sentry_grab_build(${TARGET_NAME})

    target_link_libraries(${TARGET_NAME}
        PUBLIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sentry.lib
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/crashpad_handler_lib.lib
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/crashpad_wer.lib
    )

    target_include_directories(${TARGET_NAME}
        PUBLIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include
    )
    
endfunction()