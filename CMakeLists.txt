cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(Axe VERSION 0.1.0 LANGUAGES CXX)

include(CMakePrintHelpers) # cmake_print_variables(..) used for printing

# Add property to allow making project folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Lanuages
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message("C++ compiler flags: ${CMAKE_CXX_FLAGS}")

# a general funtion+macro used for generating filter of vs2022
function(GROUP_DIR input_path)
  file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${input_path} ${CMAKE_CURRENT_SOURCE_DIR}/${input_path}/*)
  set(cursrclist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${input_path}/${child})
	      GROUP_DIR(${input_path}/${child})
		else()
		list(APPEND cursrclist ${CMAKE_CURRENT_SOURCE_DIR}/${input_path}/${child})
    endif()
  endforeach()
  
  if ("${input_path} " STREQUAL " ")
    source_group("" FILES ${cursrclist})
  else()
  	source_group(${input_path} FILES ${cursrclist})
  endif()
endfunction()

macro(ALLSRCLIST result)
GROUP_DIR("")
file(GLOB_RECURSE all_items ${CMAKE_CURRENT_SOURCE_DIR}/*)
set(SRCLIST "")
foreach(src_file ${all_items})
	if(NOT IS_DIRECTORY src_file)
		list(APPEND SRCLIST  ${src_file})
	endif()
endforeach()
endmacro()

add_subdirectory(Source/ThirdParty)
# add_subdirectory(Source/AssetSystem)
add_subdirectory(Source/Runtime)
# add_subdirectory(Source/Tests)
add_subdirectory(Examples)
