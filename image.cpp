#include "al2o3_platform/platform.h"
#include "utils_simple_logmanager/logmanager.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "gfx_imageio/io.h"
#include "lua-5.3.5/src/lua.hpp"

static char const MetaName[] = "Al230.Image";

// create the null image user data return on the lua state
static Image_ImageHeader** imageud_create(lua_State *L) {
	// allocate a pointer and push it onto the stack
	Image_ImageHeader** ud = (Image_ImageHeader**)lua_newuserdata(L, sizeof(Image_ImageHeader*));
	if(ud == nullptr) return nullptr;

	*ud = nullptr;
	luaL_getmetatable(L, MetaName);
	lua_setmetatable(L, -2);
	return ud;
}

static int imageud_gc (lua_State *L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	if (image) Image_Destroy(image);

	return 0;
}

static int width(lua_State * L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->width);
	return 1;
}

static int height(lua_State * L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->height);
	return 1;
}
static int depth(lua_State * L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->depth);
	return 1;
}
static int slices(lua_State * L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->slices);
	return 1;
}

static int load(lua_State * L) {
	char const* filename = luaL_checkstring(L, 1);

	auto ud = imageud_create(L);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_ReadBinary);
	if(!file) {
		return 1;
	}

	*ud = Image_Load(file);

	return 1;
}

static int saveDDS(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SaveDDS(image, file);

	return 0;
}

static int saveTGA(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SaveTGA(image, file);

	return 0;
}
static int saveBMP(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SaveBMP(image, file);

	return 0;
}
static int savePNG(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SavePNG(image, file);

	return 0;
}

static int saveJPG(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SaveJPG(image, file);

	return 0;
}


static int saveHDR(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	Image_ImageHeader* image = *(Image_ImageHeader**)ud;
	Image_SaveHDR(image, file);

	return 0;
}



int luaopen_Image(lua_State* L) {
	static const struct luaL_Reg imageObj [] = {
			{"width", &width},
			{"height", &height},
			{"depth", &depth},
			{"slices", &slices},
			{"saveAsTGA", &saveTGA},
			{"saveAsBMP", &saveBMP},
			{"saveAsPNG", &savePNG},
			{"saveAsJPG", &saveJPG},
			{"saveAsHDR", &saveHDR},
			// currently broken			{"saveAsDDS", &saveDDS},
			{"__gc", &imageud_gc },
			{nullptr, nullptr}  /* sentinel */
	};

	static const struct luaL_Reg imageLib [] = {
			{"load", &load},
			{nullptr, nullptr}  /* sentinel */
	};

	luaL_newmetatable(L, MetaName);
	/* metatable.__index = metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	/* register methods */
	luaL_setfuncs(L, imageObj, 0);

	luaL_newlib(L, imageLib);
	return 1;
}