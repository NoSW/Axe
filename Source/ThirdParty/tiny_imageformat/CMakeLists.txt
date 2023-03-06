cmake_minimum_required(VERSION 3.12.4)

set(LibName tiny_imageformat)
project(${LibName})

set(Interface
		./include/tiny_imageformat/tinyimageformat_base.h
		./include/tiny_imageformat/tinyimageformat_apis.h
		./include/tiny_imageformat/tinyimageformat_bits.h
		./include/tiny_imageformat/tinyimageformat_decode.h
		./include/tiny_imageformat/tinyimageformat_encode.h
		./include/tiny_imageformat/tinyimageformat_query.h
		./include/tiny_imageformat/tinyimageformat.h
)
set(TNIY_IMAGEFORMAT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")
add_library(${LibName} INTERFACE ${Interface})


