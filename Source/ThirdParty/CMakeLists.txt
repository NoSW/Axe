
set(THIRD_PARTY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

##############################################################################
#                                 mimalloc
##############################################################################
set(MI_OVERRIDE OFF CACHE BOOL "")
set(MI_OSX_INTERPOSE OFF CACHE BOOL "")
set(MI_OSX_ZONE OFF CACHE BOOL "")
set(MI_BUILD_SHARED OFF CACHE BOOL "")
set(MI_BUILD_OBJECT OFF CACHE BOOL "")
set(MI_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory(mimalloc)
set_target_properties(mimalloc-static PROPERTIES FOLDER "Source/ThirdParty")

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
set_target_properties(SDL2-static PROPERTIES FOLDER "Source/ThirdParty")
set_target_properties(sdl_headers_copy PROPERTIES FOLDER "Source/ThirdParty")

##############################################################################
#                                 spdlog (header-only)
##############################################################################
add_subdirectory(spdlog)
set_target_properties(spdlog PROPERTIES FOLDER "Source/ThirdParty")

##############################################################################
#                                 magic_enum (header-only)
##############################################################################
add_subdirectory(magic_enum)
set_target_properties(magic_enum PROPERTIES FOLDER "Source/ThirdParty")

##############################################################################
#                                 glm (header-only)
##############################################################################
add_subdirectory(glm)
set_target_properties(glm PROPERTIES FOLDER "Source/ThirdParty")

##############################################################################
#                                 tiny_imageformat
##############################################################################
add_subdirectory(tiny_imageformat)
set_target_properties(tiny_imageformat PROPERTIES FOLDER "Source/ThirdParty")
