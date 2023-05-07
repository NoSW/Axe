cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(Axe VERSION 0.1.0 LANGUAGES CXX)

include(CMakePrintHelpers) # cmake_print_variables(..) used for printing

#############################################################################################
# Options
#############################################################################################
set(AXE_USE_DYNAMIC_MIMALLOC OFF CACHE BOOL "useful when we need to detect memory usage except new/delete in third-party libraries, e.g., malloc/free")

#############################################################################################
# property
#############################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Add property to allow making project folders in IDEs
define_property(GLOBAL PROPERTY ALL_SAHRED_BINARY BRIEF_DOCS "all shared binary" FULL_DOCS "all shared binary")

#############################################################################################
# Lanuages
#############################################################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message("C++ compiler flags: ${CMAKE_CXX_FLAGS}")

#############################################################################################
# helpr functions
#############################################################################################

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

function(copy_all_shared_to_target target)
  get_property(ALL_SAHRED_BINARY GLOBAL PROPERTY ALL_SAHRED_BINARY)
  foreach(filepath ${ALL_SAHRED_BINARY})
      get_filename_component(filext "${filepath}" EXT)
      if ((WIN32 AND filext STREQUAL ".dll") OR (__linux__ AND filext STREQUAL ".so"))
          add_custom_command(TARGET ${target} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${filepath} $<TARGET_FILE_DIR:${target}>
          COMMENT "Copying ${filepath} for ${target}")
      else()
          message(FATAL_ERROR "Unsupported filext: ${filext} on current platform")
      endif()
  endforeach()
endfunction()

#############################################################################################
# compilers
#############################################################################################
if(WIN32)
  set(PLATFORM_NAME "Windows")
  set(PYTHON_PATH python)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PLATFORM_NAME "Linux")
  set(PYTHON_PATH python3)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

if (MSVC)
    add_compile_options(/MP)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #    add_compile_options(-fsanitize-address-use-after-return=runtime)
    #    add_compile_options(-fsanitize=undefined)
    add_compile_options(-Wno-nullability-completeness)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_compile_options(-fsanitize=thread)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

#############################################################################################
# clean
#############################################################################################
add_custom_target(clear_generated COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/Source/Generated")

#############################################################################################
# subdirectories
#############################################################################################
add_subdirectory(Source/ThirdParty)
# add_subdirectory(Source/AssetSystem)
add_subdirectory(Source/Shaders)
add_subdirectory(Source/Runtime)
# add_subdirectory(Source/Tests)
add_subdirectory(Examples)
