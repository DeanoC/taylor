#include "al2o3_platform/platform.h"
#include "utils_simple_logmanager/logmanager.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "gfx_imageio/io.h"
#include "gfx_image/create.h"
#include "gfx_image/utils.h"
#include "lua-5.3.5/src/lua.hpp"

static char const MetaName[] = "Al2o3.Image";

// create the null image user data return on the lua state
static Image_ImageHeader const** imageud_create(lua_State *L) {
	// allocate a pointer and push it onto the stack
	Image_ImageHeader const** ud = (Image_ImageHeader const**)lua_newuserdata(L, sizeof(Image_ImageHeader*));
	if(ud == nullptr) return nullptr;

	*ud = nullptr;
	luaL_getmetatable(L, MetaName);
	lua_setmetatable(L, -2);
	return ud;
}

static int imageud_gc (lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	if (image) Image_Destroy(image);

	return 0;
}

static int width(lua_State * L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->width);
	return 1;
}

static int height(lua_State * L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->height);
	return 1;
}
static int depth(lua_State * L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->depth);
	return 1;
}
static int slices(lua_State * L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, image->slices);
	return 1;
}

static int format(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushstring(L, ImageFormat_Name(image->format));
	return 1;
}

static int flags(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_createtable(L, 0, 2);
	lua_pushboolean(L, image->flags & Image_Flag_Cubemap);
	lua_setfield(L, -2, "Cubemap");
	lua_pushboolean(L, image->flags & Image_Flag_HeaderOnly);
	lua_setfield(L, -2, "HeaderOnly");
	return 1;
}

static int getPixelAt(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t index = luaL_checkinteger(L, 2);
	Image_PixelD pixel;
	Image_GetPixelAt(image, &pixel, index);

	lua_pushnumber(L, pixel.r);
	lua_pushnumber(L, pixel.g);
	lua_pushnumber(L, pixel.b);
	lua_pushnumber(L, pixel.a);

	return 4;
}

static int setPixelAt(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t index = luaL_checkinteger(L, 2);
	double r = luaL_checknumber(L, 3);
	double g = luaL_checknumber(L, 4);
	double b = luaL_checknumber(L, 5);
	double a = luaL_checknumber(L, 6);

	Image_PixelD pixel = { r, g, b, a };
	Image_SetPixelAt(image, &pixel, index);

	return 0;
}

static int is1D(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushboolean(L, Image_Is1D(image));
	return 1;
}

static int is2D(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushboolean(L, Image_Is2D(image));
	return 1;
}

static int is3D(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushboolean(L, Image_Is3D(image));
	return 1;
}

static int isArray(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushboolean(L, Image_IsArray(image));
	return 1;
}

static int isCubemap(lua_State *L) {
	Image_ImageHeader* image = *(Image_ImageHeader**)luaL_checkudata(L, 1, MetaName);
	lua_pushboolean(L, Image_IsCubemap(image));
	return 1;
}

static int pixelCount(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_PixelCountOf(image));
	return 1;
}
static int pixelCountPerSlice(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_PixelCountPerSliceOf(image));
	return 1;
}

static int pixelCountPerPage(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_PixelCountPerPageOf(image));
	return 1;
}

static int pixelCountPerRow(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_PixelCountPerRowOf(image));
	return 1;
}

static int byteCount(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_ByteCountOf(image));
	return 1;
}

static int byteCountPerSlice(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_ByteCountPerSliceOf(image));
	return 1;
}


static int byteCountPerPage(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_ByteCountPerPageOf(image));
	return 1;
}


static int byteCountPerRow(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_ByteCountPerRowOf(image));
	return 1;
}

static int byteCountOfImageChain(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_ByteCountOfImageChainOf(image));
	return 1;
}

static int bytesRequiredForMipMaps(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_BytesRequiredForMipMapsOf(image));
	return 1;
}

static int calculateIndex(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t x = luaL_checkinteger(L, 2);
	int64_t y = luaL_checkinteger(L, 3);
	int64_t z = luaL_checkinteger(L, 4);
	int64_t s = luaL_checkinteger(L, 5);

	lua_pushinteger(L, Image_CalculateIndex(image, (uint32_t)x, (uint32_t)y, (uint32_t)z, (uint32_t)s));
	return 1;
}

static int getChannelAt(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	tinystl::string chan(luaL_checkstring(L, 2));
	int64_t index = luaL_checkinteger(L, 3);
	chan = chan.to_lower();

	Image_Channel channel = Image_Channel::Image_Red;
	switch(Utils::CompileTimeHash(chan)) {
	case "red"_hash: channel = Image_Channel::Image_Red; break;
	case "green"_hash: channel = Image_Channel::Image_Green; break;
	case "blue"_hash: channel = Image_Channel::Image_Blue; break;
	case "alpha"_hash: channel = Image_Channel::Image_Alpha; break;
	default: lua_error(L);
	}

	lua_pushnumber(L, Image_GetChannelAt(image, channel, index));
	return 1;
}

static int setChannelAt(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	tinystl::string chan(luaL_checkstring(L, 2));
	int64_t index = luaL_checkinteger(L, 3);
	double v = luaL_checknumber(L, 4);

	chan = chan.to_lower();

	Image_Channel channel = Image_Channel::Image_Red;
	switch(Utils::CompileTimeHash(chan)) {
	case "red"_hash: channel = Image_Channel::Image_Red; break;
	case "green"_hash: channel = Image_Channel::Image_Green; break;
	case "blue"_hash: channel = Image_Channel::Image_Blue; break;
	case "alpha"_hash: channel = Image_Channel::Image_Alpha; break;
	default: lua_error(L);
	}
	Image_SetChannelAt(image, channel, index, v);


	return 0;
}
static int copy(lua_State *L) {
	auto src = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	auto dst = *(Image_ImageHeader const**)luaL_checkudata(L, 2, MetaName);

	Image_CopyImage(src, dst);
	return 0;
}

static int copySlice(lua_State *L) {
	auto src = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t sw = luaL_checkinteger(L, 2);
	auto dst = *(Image_ImageHeader const**)luaL_checkudata(L, 3, MetaName);
	int64_t dw = luaL_checkinteger(L, 4);

	Image_CopySlice(src, sw, dst, dw);
	return 0;
}

static int copyPage(lua_State *L) {
	auto src = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t sz = luaL_checkinteger(L, 2);
	int64_t sw = luaL_checkinteger(L, 3);
	auto dst = *(Image_ImageHeader const**)luaL_checkudata(L, 4, MetaName);
	int64_t dz = luaL_checkinteger(L, 5);
	int64_t dw = luaL_checkinteger(L, 6);

	Image_CopyPage(src, sz, sw, dst, dz, dw);
	return 0;
}

static int copyRow(lua_State *L) {
	auto src = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t sy = luaL_checkinteger(L, 2);
	int64_t sz = luaL_checkinteger(L, 3);
	int64_t sw = luaL_checkinteger(L, 4);
	auto dst = *(Image_ImageHeader const**)luaL_checkudata(L, 5, MetaName);
	int64_t dy = luaL_checkinteger(L, 6);
	int64_t dz = luaL_checkinteger(L, 7);
	int64_t dw = luaL_checkinteger(L, 8);

	Image_CopyRow(src, sy, sz, sw, dst, dy, dz, dw);
	return 0;
}

static int copyPixel(lua_State *L) {
	auto src = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t sx = luaL_checkinteger(L, 2);
	int64_t sy = luaL_checkinteger(L, 3);
	int64_t sz = luaL_checkinteger(L, 4);
	int64_t sw = luaL_checkinteger(L, 5);
	auto dst = *(Image_ImageHeader const**)luaL_checkudata(L, 6, MetaName);
	int64_t dx = luaL_checkinteger(L, 7);
	int64_t dy = luaL_checkinteger(L, 8);
	int64_t dz = luaL_checkinteger(L, 9);
	int64_t dw = luaL_checkinteger(L, 10);

	Image_CopyPixel(src, sx, sy, sz, sw, dst, dx, dy, dz, dw);
	return 0;
}

static int linkedImageCount(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	lua_pushinteger(L, Image_LinkedImageCountOf(image));
	return 1;
}

static int linkedImage(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	int64_t index = luaL_checkinteger(L, 2);
	auto ud = imageud_create(L);
	*ud = Image_LinkedImageOf(image, index);;

	return 1;
}

static int createMipMapChain(lua_State * L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	bool generateFromImage = lua_isnil(L, 2) ? true : (bool)lua_toboolean(L, 2);
	Image_CreateMipMapChain(image,generateFromImage);
	return 0;
}

static int clone(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	auto ud = imageud_create(L);
	*ud = Image_Clone(image);
	return 1;
}

static int cloneStructure(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	auto ud = imageud_create(L);
	*ud = Image_CloneStructure(image);
	return 1;

}

static int preciseConvert(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	auto ud = imageud_create(L);
	*ud = Image_PreciseConvert(image, ImageFormat_FromName(luaL_checkstring(L,2)) );
	return 1;
}

static int fastConvert(lua_State *L) {
	auto image = *(Image_ImageHeader const**)luaL_checkudata(L, 1, MetaName);
	bool allowInPlace = lua_isnil(L, 2) ? false : (bool)lua_toboolean(L, 3);
	auto ud = imageud_create(L);
	*ud = Image_FastConvert(image, ImageFormat_FromName(luaL_checkstring(L,2)), allowInPlace);

	return 1;
}

static int create(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	int64_t s = luaL_checkinteger(L, 4);
	char const* fmt = luaL_checkstring(L, 5);

	auto ud = imageud_create(L);
	*ud = Image_Create((uint32_t)w, (uint32_t)h, (uint32_t)d, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int createNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	int64_t s = luaL_checkinteger(L, 4);
	char const* fmt = luaL_checkstring(L, 5);

	auto ud = imageud_create(L);
	*ud = Image_CreateNoClear((uint32_t)w, (uint32_t)h, (uint32_t)d, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int create1D(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	char const* fmt = luaL_checkstring(L, 2);

	auto ud = imageud_create(L);
	*ud = Image_Create1D((uint32_t)w,ImageFormat_FromName(fmt));
	return 1;
}

static int create1DNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	char const* fmt = luaL_checkstring(L, 2);

	auto ud = imageud_create(L);
	*ud = Image_Create1DNoClear((uint32_t)w, ImageFormat_FromName(fmt));
	return 1;
}

static int create1DArray(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t s = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_Create1DArray((uint32_t)w, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int create1DArrayNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t s = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_Create1DArrayNoClear((uint32_t)w, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int create2D(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_Create2D((uint32_t)w, (uint32_t)h, ImageFormat_FromName(fmt));
	return 1;
}
static int create2DNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_Create2DNoClear((uint32_t)w, (uint32_t)h, ImageFormat_FromName(fmt));
	return 1;
}

static int create2DArray(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t s = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_Create2DArray((uint32_t)w, (uint32_t)h, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int create2DArrayNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t s = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_Create2DArrayNoClear((uint32_t)w, (uint32_t)h, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int create3D(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_Create3D((uint32_t)w, (uint32_t)h, (uint32_t)d, ImageFormat_FromName(fmt));
	return 1;
}

static int create3DNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_Create3DNoClear((uint32_t)w, (uint32_t)h, (uint32_t)d, ImageFormat_FromName(fmt));
	return 1;
}

static int create3DArray(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	int64_t s = luaL_checkinteger(L, 4);
	char const* fmt = luaL_checkstring(L, 5);

	auto ud = imageud_create(L);
	*ud = Image_Create3DArray((uint32_t)w, (uint32_t)h, (uint32_t)d, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}
static int create3DArrayNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t d = luaL_checkinteger(L, 3);
	int64_t s = luaL_checkinteger(L, 4);
	char const* fmt = luaL_checkstring(L, 5);

	auto ud = imageud_create(L);
	*ud = Image_Create3DArrayNoClear((uint32_t)w, (uint32_t)h, (uint32_t)d, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}
static int createCubemap(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_CreateCubemap((uint32_t)w, (uint32_t)h, ImageFormat_FromName(fmt));
	return 1;
}

static int createCubemapNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	char const* fmt = luaL_checkstring(L, 3);

	auto ud = imageud_create(L);
	*ud = Image_CreateCubemapNoClear((uint32_t)w, (uint32_t)h, ImageFormat_FromName(fmt));
	return 1;
}

static int createCubemapArray(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t s = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_CreateCubemapArray((uint32_t)w, (uint32_t)h, (uint32_t)s, ImageFormat_FromName(fmt));
	return 1;
}

static int createCubemapArrayNoClear(lua_State *L) {
	int64_t w = luaL_checkinteger(L, 1);
	int64_t h = luaL_checkinteger(L, 2);
	int64_t s = luaL_checkinteger(L, 3);
	char const* fmt = luaL_checkstring(L, 4);

	auto ud = imageud_create(L);
	*ud = Image_CreateCubemapArrayNoClear((uint32_t)w, (uint32_t)h, (uint32_t)s, ImageFormat_FromName(fmt));
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
	auto image = *(Image_ImageHeader const**)ud;
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
	auto image = *(Image_ImageHeader const**)ud;
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
	auto image = *(Image_ImageHeader const**)ud;
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
	auto image = *(Image_ImageHeader const**)ud;
	Image_SaveJPG(image, file);

	return 0;
}

static int saveKTX(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	auto image = *(Image_ImageHeader const**)ud;
	Image_SaveKTX(image, file);

	return 0;
}

static int saveHDR(lua_State * L) {
	void* ud = luaL_checkudata(L, 1, MetaName);
	char const* filename = luaL_checkstring(L, 2);
	VFile::ScopedFile file = VFile::File::FromFile(filename, Os_FM_WriteBinary);
	if(!file) {
		return 0;
	}
	auto image = *(Image_ImageHeader const**)ud;
	Image_SaveHDR(image, file);

	return 0;
}

int luaopen_Image(lua_State* L) {
	static const struct luaL_Reg imageObj [] = {
			{"width", &width},
			{"height", &height},
			{"depth", &depth},
			{"slices", &slices},
			{"format", &format},
			{"flags", &flags},

			{"getPixelAt", &getPixelAt},
			{"setPixelAt", &setPixelAt},
			{"getChannelAt", &getChannelAt},
			{"setChannelAt", &setChannelAt},

			{"copy", &copy},
			{"copySlice", &copySlice},
			{"copyPage", &copyPage},
			{"copyRow", &copyRow},
			{"copyPixel", &copyPixel},

			{"is1D", &is1D},
			{"is2D", &is2D},
			{"is3D", &is3D},
			{"isArray", &isArray},
			{"isCubemap", &isCubemap},

			{"calculateIndex", &calculateIndex },

			{"pixelCount", &pixelCount },
			{"pixelCountPerSlice", &pixelCountPerSlice },
			{"pixelCountPerPage", &pixelCountPerPage },
			{"pixelCountPerRow", &pixelCountPerRow },

			{"byteCount", &byteCount },
			{"byteCountPerSlice", &byteCountPerSlice },
			{"byteCountPerPage", &byteCountPerPage },
			{"byteCountPerRow", &byteCountPerRow },

			{"linkedImageCount", &linkedImageCount},
			{"linkedImage", &linkedImage},

			{"byteCountOfImageChain", &byteCountOfImageChain},
			{"bytesRequiredForMipMaps", &bytesRequiredForMipMaps},

			{"createMipMapChain", &createMipMapChain},
			{"clone", &clone},
			{"cloneStructure", &cloneStructure},
			{"preciseConvert", &preciseConvert},
			{"fastConvert", &fastConvert},

			{"saveAsTGA", &saveTGA},
			{"saveAsBMP", &saveBMP},
			{"saveAsPNG", &savePNG},
			{"saveAsJPG", &saveJPG},
			{"saveAsHDR", &saveHDR},
			{"saveAsKTX", &saveKTX},
			// currently broken			{"saveAsDDS", &saveDDS},
			{"__gc", &imageud_gc },
			{nullptr, nullptr}  /* sentinel */
	};

	static const struct luaL_Reg imageLib [] = {
			{"create", &create},
			{"createNoClear", &createNoClear},

			{"create1D", &create1D},
			{"create1DNoClear", &create1DNoClear},
			{"create1DArray", &create1DArray},
			{"create1DArrayNoClear", &create1DArrayNoClear},

			{"create2D", &create2D},
			{"create2DNoClear", &create2DNoClear},
			{"create2DArray", &create2DArray},
			{"create2DArrayNoClear", &create2DArrayNoClear},

			{"create3D", &create3D},
			{"create3DNoClear", &create3DNoClear},
			{"create3DArray", &create3DArray},
			{"create3DArrayNoClear", &create3DArrayNoClear},

			{"createCubemap", &createCubemap},
			{"createCubemapNoClear", &createCubemapNoClear},
			{"createCubemapArray", &createCubemapArray},
			{"createCubemapArrayNoClear", &createCubemapArrayNoClear},

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