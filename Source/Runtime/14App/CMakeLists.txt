set(TARGET_NAME 14App)

ALLSRCLIST(SRCLIST)
add_library(${TARGET_NAME} STATIC ${SRCLIST})

target_link_libraries(${TARGET_NAME} PUBLIC 06Pipeline)

set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")