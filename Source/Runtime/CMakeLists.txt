set(TARGET_NAME Runtime)
set(RUNTIME_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}  CACHE STRING "")

add_subdirectory(00Core)
add_subdirectory(02Rhi)
add_subdirectory(03Resource)
add_subdirectory(06Pipeline)
add_subdirectory(14App)

add_library(${TARGET_NAME} INTERFACE)
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${TARGET_NAME} INTERFACE 00Core)
target_link_libraries(${TARGET_NAME} INTERFACE 02Rhi)
target_link_libraries(${TARGET_NAME} INTERFACE 03Resource)
target_link_libraries(${TARGET_NAME} INTERFACE 06Pipeline)
target_link_libraries(${TARGET_NAME} INTERFACE 14App)


# FAQ: Difference of PRIVATE|PUBLIC|INTERFACE, see https://leimao.github.io/blog/CMake-Public-Private-Interface/
