set(TARGET_NAME 02Rhi)

file(GLOB COMMON_SRCLIST ./*.*)
file(GLOB_RECURSE VULKAN_SRCLIST ./Vulkan/*)
source_group("" FILES ${COMMON_SRCLIST})
source_group("Vulkan" FILES ${VULKAN_SRCLIST})
if(WIN32)
file(GLOB_RECURSE D3D12_SRCLIST ./D3D12/*)
source_group("D3D12" FILES ${D3D12_SRCLIST})
add_library(${TARGET_NAME} STATIC ${COMMON_SRCLIST} ${VULKAN_SRCLIST} ${D3D12_SRCLIST})
else()
add_library(${TARGET_NAME} STATIC ${COMMON_SRCLIST} ${VULKAN_SRCLIST})
endif()
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Source/Runtime")
target_link_libraries(${TARGET_NAME} PUBLIC 00Core)

############################################################################################
##                                         Vulkan
############################################################################################

# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON)

if(AUTO_LOCATE_VULKAN)
	message(STATUS "Attempting auto locate Vulkan using CMake......")
	
	# Find Vulkan Path using CMake's Vulkan Module
	# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
	# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
	find_package(Vulkan REQUIRED)
	
	# Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
	if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
		set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
		STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
	endif()
		 
	if(NOT Vulkan_FOUND)
		# CMake may fail to locate the libraries but could be able to 
		# provide some path in Vulkan SDK include directory variable
		# 'Vulkan_INCLUDE_DIRS', try to extract path from this.
		message(STATUS "Failed to locate Vulkan SDK, retrying again...")
		if(EXISTS "${VULKAN_PATH}")
			message(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
		else()
			message("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
			message("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
			return()
		endif()
	endif()
else()
	message(STATUS "Attempting to locate Vulkan SDK using manual path......")
	set(VULKAN_SDK "C:/VulkanSDK")
	set(VULKAN_VERSION "1.3.216.0")
	set(VULKAN_PATH "${VULKAN_SDK}/${VULKAN_VERSION}")
	message(STATUS "Using manual specified path: ${VULKAN_PATH}")

	# Check if manual set path exists
	if(NOT EXISTS "${VULKAN_PATH}")
		message("Error: Unable to locate this Vulkan SDK path VULKAN_PATH: ${VULKAN_PATH}, please specify correct path.
		For more information on correct installation process, please refer to subsection 'Getting started with Lunar-G SDK'
		and 'Setting up first project with CMake' in Chapter 3, 'Shaking hands with the device' in this book 'Learning Vulkan', ISBN - 9781786469809.")
	   return()
	endif()
endif()

if(WIN32)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${Vulkan_LIB_LIST})

############################################################################################
##                                         D3D12
############################################################################################
if(WIN32)
target_link_libraries(${TARGET_NAME} PRIVATE d3d12.lib dxgi.lib)
endif()