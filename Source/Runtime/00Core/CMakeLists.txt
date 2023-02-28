set(TARGET_NAME 00Core)

ALLSRCLIST(SRCLIST)
add_library(${TARGET_NAME} STATIC ${SRCLIST})

target_link_libraries(${TARGET_NAME} PRIVATE mimalloc-static)
target_link_libraries(${TARGET_NAME} PUBLIC SDL2-static)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog_header_only)
target_include_directories(${TARGET_NAME} PUBLIC ${RUNTIME_INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PUBLIC ${MAGIC_ENUM_INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIR})
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Source/Runtime")