
set(THIRD_PARTY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

##############################################################################
#                                 mimalloc
##############################################################################
set(MI_OVERRIDE ON CACHE BOOL "")
set(MI_OSX_INTERPOSE OFF CACHE BOOL "")
set(MI_OSX_ZONE OFF CACHE BOOL "")
set(MI_BUILD_SHARED ON CACHE BOOL "")
set(MI_BUILD_STATIC ON CACHE BOOL "")
set(MI_BUILD_OBJECT OFF CACHE BOOL "")
set(MI_BUILD_TESTS OFF CACHE BOOL "")
if (CMAKE_C_COMPILER_ID STREQUAL  "clang")
    set(MI_DEBUG_TSAN ON CACHE BOOL "")
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL  "clang++")
    set(MI_DEBUG_UBSAN ON CACHE BOOL "")
endif()
set(MI_SKIP_COLLECT_ON_EXIT OFF CACHE BOOL "")

add_subdirectory(mimalloc)
set_target_properties(mimalloc-static PROPERTIES FOLDER "ThirdParty/mimalloc")
set_target_properties(mimalloc PROPERTIES FOLDER "ThirdParty/mimalloc")

##############################################################################
#                                 SDL
##############################################################################
set(SDL_WAYLAND_SHARED OFF CACHE BOOL "")
set(SDL_WAYLAND_LIBDECOR OFF CACHE BOOL "")
set(SDL_WAYLAND_LIBDECOR_SHARED OFF CACHE BOOL "")
set(SDL_WAYLAND_QT_TOUCH OFF CACHE BOOL "")
set(SDL_METAL OFF CACHE BOOL "")
set(SDL_KMSDRM OFF CACHE BOOL "")
set(SDL_KMSDRM_SHARED OFF CACHE BOOL "")
set(SDL_HIDAPI OFF CACHE BOOL "")
set(SDL_HIDAPI_JOYSTICK OFF CACHE BOOL "")
set(SDL_VIRTUAL_JOYSTICK OFF CACHE BOOL "")
set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_TEST OFF CACHE BOOL "")
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "")
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "")
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "")
add_subdirectory(SDL)
set_target_properties(SDL2-static PROPERTIES FOLDER "ThirdParty/SDL")
set_target_properties(sdl_headers_copy PROPERTIES FOLDER "ThirdParty/SDL")

##############################################################################
#                                 spdlog (header-only)
##############################################################################
add_subdirectory(spdlog)
set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")

##############################################################################
#                                 magic_enum (header-only)
##############################################################################
add_subdirectory(magic_enum)
set_target_properties(magic_enum PROPERTIES FOLDER "ThirdParty")

##############################################################################
#                                 magic_enum (header-only)
##############################################################################
add_subdirectory(stb)
set_target_properties(stb PROPERTIES FOLDER "ThirdParty")

##############################################################################
#                                 glm (header-only)
##############################################################################
add_subdirectory(glm)
set_target_properties(glm PROPERTIES FOLDER "ThirdParty")

##############################################################################
#                                 tiny_imageformat
##############################################################################
add_subdirectory(tiny_imageformat)
set_target_properties(tiny_imageformat PROPERTIES FOLDER "ThirdParty")
##############################################################################
#                                 SPIRV-Cross
##############################################################################
set(SPIRV_CROSS_CLI OFF CACHE BOOL "")
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "")
add_subdirectory(SPIRV-Cross)
set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")