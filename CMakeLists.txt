cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

include(FetchContent)
FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
FetchContent_GetProperties(al2o3)
if(NOT al2o3_POPULATED)
	FetchContent_Populate(al2o3)
	add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
endif()

SET_MIN_VERSIONS()
set(ProjectName taylor)
project(${ProjectName})

set(Src
		main.cpp
		image.cpp
		lua-5.3.5/src/lapi.c
		lua-5.3.5/src/lapi.h
		lua-5.3.5/src/lauxlib.c
		lua-5.3.5/src/lauxlib.h
		lua-5.3.5/src/lbaselib.c
		lua-5.3.5/src/lbitlib.c
		lua-5.3.5/src/lcode.c
		lua-5.3.5/src/lcode.h
		lua-5.3.5/src/lcorolib.c
		lua-5.3.5/src/lctype.c
		lua-5.3.5/src/lctype.h
		lua-5.3.5/src/ldblib.c
		lua-5.3.5/src/ldebug.c
		lua-5.3.5/src/ldebug.h
		lua-5.3.5/src/ldo.c
		lua-5.3.5/src/ldo.h
		lua-5.3.5/src/ldump.c
		lua-5.3.5/src/lfunc.c
		lua-5.3.5/src/lfunc.h
		lua-5.3.5/src/lgc.c
		lua-5.3.5/src/lgc.h
		lua-5.3.5/src/linit.c
		lua-5.3.5/src/liolib.c
		lua-5.3.5/src/llex.c
		lua-5.3.5/src/llex.h
		lua-5.3.5/src/llimits.h
		lua-5.3.5/src/lmathlib.c
		lua-5.3.5/src/lmem.c
		lua-5.3.5/src/lmem.h
		lua-5.3.5/src/loadlib.c
		lua-5.3.5/src/lobject.c
		lua-5.3.5/src/lobject.h
		lua-5.3.5/src/lopcodes.c
		lua-5.3.5/src/lopcodes.h
		lua-5.3.5/src/loslib.c
		lua-5.3.5/src/lparser.c
		lua-5.3.5/src/lparser.h
		lua-5.3.5/src/lprefix.h
		lua-5.3.5/src/lstate.c
		lua-5.3.5/src/lstate.h
		lua-5.3.5/src/lstring.c
		lua-5.3.5/src/lstring.h
		lua-5.3.5/src/lstrlib.c
		lua-5.3.5/src/ltable.c
		lua-5.3.5/src/ltable.h
		lua-5.3.5/src/ltablib.c
		lua-5.3.5/src/ltm.c
		lua-5.3.5/src/ltm.h
		lua-5.3.5/src/lua.hpp
		lua-5.3.5/src/luaconf.h
		lua-5.3.5/src/lualib.h
		lua-5.3.5/src/lundump.c
		lua-5.3.5/src/lundump.h
		lua-5.3.5/src/lutf8lib.c
		lua-5.3.5/src/lvm.c
		lua-5.3.5/src/lvm.h
		lua-5.3.5/src/lzio.c
		lua-5.3.5/src/lzio.h
		)
set(Deps
		al2o3_platform
		al2o3_memory
		al2o3_os
		al2o3_cmath
		al2o3_tinystl
		gfx_image_interface
		gfx_image_impl_basic
		gfx_imageio
		al2o3_vfile
		data_binify
		data_bufferutils
		data_binnybundle
		data_binnybundlewriter
		utils_simple_logmanager
		utils_misccpp
		)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)

set(LIB_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/)

ADD_CONSOLE_APP(${ProjectName} "${Src}" "${Deps}")
target_include_directories(${ProjectName} PRIVATE lua-5.3.5/src/)