set(TARGET_NAME 01Resource)

ALLSRCLIST(SRCLIST)
add_library(${TARGET_NAME} STATIC ${SRCLIST})

target_link_libraries(${TARGET_NAME} PUBLIC 00Core)

target_link_libraries(${TARGET_NAME} PRIVATE Shaders)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Generated)

set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")