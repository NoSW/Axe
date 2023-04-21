set(TARGET_NAME HelloTriangle)

ALLSRCLIST(SRCLIST)
add_executable(${TARGET_NAME} ${SRCLIST})
target_link_libraries(${TARGET_NAME} PRIVATE Runtime)
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Examples")

if (AXE_USE_DYNAMIC_MIMALLOC)
    add_custom_command (TARGET HelloTriangle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mimalloc> $<TARGET_FILE_DIR:HelloTriangle>
        COMMENT "Copy $<TARGET_FILE:mimalloc> to ${CMAKE_CURRENT_BINARY_DIR}\n"
    )
    if(WIN32)
        add_custom_command(TARGET HelloTriangle POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy "${THIRD_PARTY_INCLUDE_DIR}/mimalloc/bin/mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.dll" $<TARGET_FILE_DIR:HelloTriangle>
          COMMENT "Copy mimalloc-redirect${MIMALLOC_REDIRECT_SUFFIX}.dll to ${CMAKE_CURRENT_BINARY_DIR}\n"
        )
    endif()
endif()
