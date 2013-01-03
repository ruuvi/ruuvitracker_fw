#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "auxmods.h"
#include "sha1.h"
#include <string.h>

#define SHA1_LEN	40

static int sha1_gen( lua_State *L )
{
	SHA1Context sha;
	
	const char *str = luaL_checkstring(L,1);

	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)str, strlen(str));

	if (!SHA1Result(&sha)) {
        return 0;
    } else {
    	char s[SHA1_LEN];
    	int i;
        for(i = 0; i < 5 ; i++)
        {
            sprintf(s+i*8,"%.8x", sha.Message_Digest[i]);
        }
        lua_pushlstring(L,s,SHA1_LEN);
        return 1;
    }

}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"  

// Module function map
const LUA_REG_TYPE sha1_map[] =
{ 
  { LSTRKEY("gen") , LFUNCVAL(sha1_gen) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sha1( lua_State *L )
{
  LREGISTER( L, "sha1", sha1_map );
}
