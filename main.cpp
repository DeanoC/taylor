#include "al2o3_platform/platform.h"
#include "utils_simple_logmanager/logmanager.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "lua_base5.3/lua.hpp"
#include "lua_base5.3/utils.h"
#include "lua_image/image.h"
#include "lua_al2o3_os/os.h"


void Usage() {
	LOGINFO("taylor script - process script to convert assets");
};

int main(int argc, char const *argv[]) {
	auto logger = SimpleLogManager_Alloc();


	lua_State* L = LuaBase_Create();

	LuaBase_RegisterLib(L, "image", &LuaImage_Open);
	LuaBase_RegisterLib(L, "al2o3.os", &LuaOs_Open);

	int ret = 0;
	if(argc != 2) { ret = 1; }
	else {

		tinystl::string scriptFileName = argv[1];

		VFile::ScopedFile scriptFile = VFile::File::FromFile(scriptFileName, Os_FileMode::Os_FM_Read);

		if (!scriptFile) {
			LOGINFO("%s can't be opened", scriptFileName.c_str());
		} else {
			try {
				LuaBase_ExecuteScript(L, scriptFile);
			} catch( ... ) {
				LOGINFO("%s has crashed", scriptFileName.c_str());
				ret = 10;
			}
		}
	}

	if (ret != 0) {
		Usage();
	}

	LuaBase_Destroy(L);

	SimpleLogManager_Free(logger);
	return ret;
}