set(TARGET_NAME 06Pipeline)

ALLSRCLIST(SRCLIST)
add_library(${TARGET_NAME} STATIC ${SRCLIST})

target_link_libraries(${TARGET_NAME} PUBLIC 03Resource)

set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")