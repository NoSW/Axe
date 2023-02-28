set(TARGET_NAME HelloTriangle)

ALLSRCLIST(SRCLIST)
add_executable(${TARGET_NAME} ${SRCLIST})
target_link_libraries(${TARGET_NAME} PRIVATE Runtime)
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Examples")