#include "al2o3_platform/platform.h"
#include "utils_simple_logmanager/logmanager.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "ezoptionparser.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "lua-5.3.5/src/lua.hpp"


static void registerLib (lua_State *L, const char *name,
												 lua_CFunction f) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");  /* get 'package.preload' */
	lua_pushcfunction(L, f);
	lua_setfield(L, -2, name);  /* package.preload[name] = f */
	lua_pop(L, 2);  /* pop 'package' and 'preload' tables */
}

static void openStdLibs (lua_State *L) {
	luaL_requiref(L, "_G", luaopen_base, 1);
	luaL_requiref(L, "package", luaopen_package, 1);
	lua_pop(L, 2);  /* remove results from previous calls */
	registerLib(L, "coroutine", luaopen_coroutine);
	registerLib(L, "table", luaopen_table);
	registerLib(L, "io", luaopen_io);
	registerLib(L, "os", luaopen_os);
	registerLib(L, "string", luaopen_string);
	registerLib(L, "math", luaopen_math);
	registerLib(L, "utf8", luaopen_utf8);
	registerLib(L, "debug", luaopen_debug);
}

extern int luaopen_Image(lua_State *luaS);

void Usage(ez::OptionParser &opt) {
	tinystl::string usage;
	opt.getUsage(usage);
	LOGINFOF("%s", usage.c_str());
};

void *LuaAllocFunc(void *ud, void *ptr, size_t osize, size_t nsize) {
	if (ptr == nullptr) {
		// alloc
		if (nsize != 0) {
			return MEMORY_CALLOC(1, nsize);
		} else {
			return nullptr;
		}
	} else {
		// resize or free
		if (nsize == 0) {
			// free
			MEMORY_FREE(ptr);
			return nullptr;
		} else {
			// resize
			return MEMORY_REALLOC(ptr, nsize);
		}
	}
}

static void *LuaReadBuffer = nullptr;
static size_t LuaReaderBufferSize = 10 * 1024;

char const *LuaReaderFunc(lua_State *L, void *ud, size_t *sz) {

	VFile_Handle handle = (VFile_Handle) ud;
	if (LuaReadBuffer == nullptr) {
		LuaReadBuffer = MEMORY_MALLOC(LuaReaderBufferSize);
	}

	*sz = VFile_Read(handle, LuaReadBuffer, LuaReaderBufferSize);

	return (char const *) LuaReadBuffer;

}

int main(int argc, char const *argv[]) {
	auto logger = SimpleLogManager_Alloc();

	int ret = 0;
	ez::OptionParser opt;

	opt.overview = "Taylor. Takes a lua script and runs various operations like image conversion";
	opt.syntax = "taylor script [OPTIONS]";
	opt.example = "tayler script.lua\n\n";
	opt.footer = "taylor 0.0.1  Copyright (C) 2019 Deano Calver\nThis program is free and without warranty.\n";

	opt.parse(argc, argv);

	if (opt.isSet("-h")) {
		ret = 1;
	}

	if (ret == 0 && opt.lastArgs.size() < 1) {
		LOGINFO("ERROR: Expected at least 1 arguments.\n\n");
		ret = 1;
	}

	tinystl::vector<tinystl::string> badOptions;
	if (ret == 0 && !opt.gotRequired(badOptions)) {
		for (int i = 0; i < badOptions.size(); ++i) {
			LOGINFOF("ERROR: Missing required option %s. \n\n", badOptions[i].c_str());
		}
		ret = 1;
	}

	if (ret == 0 && !opt.gotExpected(badOptions)) {
		for (int i = 0; i < badOptions.size(); ++i) {
			LOGINFOF("ERROR: Got unexpected number of arguments for option  %s. \n\n", badOptions[i].c_str());
		}
		ret = 1;
	}

	// -- lets get our lua on

	lua_State *luaS = lua_newstate(&LuaAllocFunc, nullptr);
	openStdLibs(luaS); // standard libs
	registerLib(luaS, "image", &luaopen_Image);

	if (ret == 0 && opt.lastArgs.size() > 0) {
		tinystl::string scriptFileName = *opt.lastArgs[0];

		VFile::ScopedFile scriptFile = VFile::File::FromFile(scriptFileName, Os_FileMode::Os_FM_Read);
		if (!scriptFile) {
			LOGINFOF("%s can't be opened", scriptFileName.c_str());
		} else {
			if( lua_load(luaS, &LuaReaderFunc, scriptFile.owned, scriptFileName.c_str(), nullptr) ||
					lua_pcall(luaS, 0,0,0) ) {
				LOGERRORF("Lua script loading error %s", lua_tostring(luaS,-1));
			};

		}
	}

	if (ret != 0) {
		Usage(opt);
	}

	lua_close(luaS);
	MEMORY_FREE(LuaReadBuffer);

	SimpleLogManager_Free(logger);
	return ret;
}