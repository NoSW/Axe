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
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Runtime")
target_link_libraries(${TARGET_NAME} PRIVATE spirv-cross-core spirv-cross-glsl spirv-cross-cpp spirv-cross-reflect spirv-cross-msl spirv-cross-hlsl spirv-cross-util spirv-cross-c)
target_link_libraries(${TARGET_NAME} PUBLIC 00Core)
target_include_directories(${TARGET_NAME} PUBLIC ${TNIY_IMAGEFORMAT_INCLUDE_DIR})

############################################################################################
##                                         Vulkan
############################################################################################
AXE_LOG("Attempting auto locate Vulkan using CMake......")

# Find Vulkan Path using CMake's Vulkan Module
# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
find_package(Vulkan)
	 
if(NOT Vulkan_FOUND AND WIN32)
    if ("${VK_SDK_PATH}" STREQUAL "")
        set(VK_SDK_PATH "C:/VulkanSDK/1.3.243.0")
        set(${Vulkan_INCLUDE_DIRS} ${VK_SDK_PATH}/Include)
        set(${Vulkan_LIBRARY} ${VK_SDK_PATH}/Lib/vulkan-1.lib)
        AXE_LOG("VK_SDK_PATH not specified, attempting to locate the Vulkan SDK using default path: ${VK_SDK_PATH}")
	endif()

    if(NOT EXISTS "${VK_SDK_PATH}")
       AXE_LOG("Error: Unable to locate this Vulkan SDK path VK_SDK_PATH: ${VK_SDK_PATH}, please specify correct path.")
       return()
    else()
        AXE_LOG("Successfully located the Vulkan SDK: ${VK_SDK_PATH}")
    endif()
endif()

if(WIN32)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/volk) # for diff dir-tree between windows and linux
target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vma) # ditto

############################################################################################
##                                         D3D12
############################################################################################
if(WIN32)
	target_link_libraries(${TARGET_NAME} PRIVATE d3d12.lib dxgi.lib)
endif()