AXE_LOG("build unit tests")
add_subdirectory(ThirdParty/Catch2)
set_target_properties(Catch2 PROPERTIES FOLDER "Tests/Catch2")
set_target_properties(Catch2WithMain PROPERTIES FOLDER "Tests/Catch2")


file(GLOB_RECURSE TEST_THREAD_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Runtime/00Core/Thread/*.cpp")
add_executable(Test_Thread ${TEST_THREAD_CPP_FILES})
target_link_libraries(Test_Thread 00Core)
target_include_directories(Test_Thread PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Source/Runtime")

# Use the standard separate compilation
set( SOURCES_IDIOMATIC_EXAMPLES
    Test_Example.cpp
)

string(REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES "${SOURCES_IDIOMATIC_EXAMPLES}" )
set(TARGETS_IDIOMATIC_EXAMPLES ${BASENAMES_IDIOMATIC_EXAMPLES} )


foreach(name ${TARGETS_IDIOMATIC_EXAMPLES} )
    add_executable(${name} ${name}.cpp )
endforeach()

set(ALL_EXAMPLE_TARGETS
    Test_Thread
    ${TARGETS_IDIOMATIC_EXAMPLES}
)

foreach( name ${ALL_EXAMPLE_TARGETS} )
    target_link_libraries(${name} Catch2 Catch2WithMain )
    set_target_properties(${name} PROPERTIES FOLDER "Tests")
endforeach()

list(APPEND CATCH_WARNING_TARGETS ${ALL_EXAMPLE_TARGETS})
set(CATCH_WARNING_TARGETS ${CATCH_WARNING_TARGETS} PARENT_SCOPE)
