/*
 *  C-interfaces for Ruuvitracker Lua codes.
 *
 * @author: Seppo Takalo
 *          Goksel Goktas
 */


#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "auxmods.h"
#include "sha1.h"
#include <string.h>
#include "stm32f4xx.h"
#include <delay.h>
#include "dlmalloc.h"


/* C-interface for Ruuvitracker codes in Lua */

static int hello( lua_State *L )
{
	lua_getglobal(L, "print");
	lua_pushstring(L, "Hello World");
	lua_pcall(L, 1, 0, 0);
	return 0;
}

/* Start a loop that gets runned by Systick clock intevalls */
/* Parameter: function to run on loop */
/* this function never returns */
static int idleloop( lua_State *L)
{
	unsigned int tick;
	if (!lua_isfunction(L, -1)) {
		printf("Not a function\n");
		return 0;
	}
	for(;;) {
		lua_pushvalue(L,-1); //Get copy of a function to stack
		if (lua_pcall(L, 0, 0, 0)) { //Call the function
			return luaL_error(L, "error running idleloop: %s",
			                  lua_tostring(L, -1));
		}

		NVIC_SystemLPConfig(NVIC_LP_SLEEPONEXIT, ENABLE); //Enable SleepOnExit mode for interrupt
		tick = systick_get_raw();
		while(tick == systick_get_raw())
			__WFI(); //Go to sleep (WaitForInterrupt)
	}
	return 0;
}

/* SystemMemory: 0x1FFF0000 */

static int reset(lua_State *L)
{
	lua_getglobal(L, "print");
	lua_pushstring(L, "Invoking software reset...");
	lua_pcall(L, 1, 0, 0);

	NVIC_SystemReset();
	return 0;
}

/* Delay functions for Lua. (Uses Systick instead of tmr) */
static int l_delay_ms(lua_State *L)
{
	unsigned int d = luaL_checkinteger(L, -1);
	delay_ms(d);
	return 0;
}

extern char end[];
static int print_mem(lua_State *L)
{
	struct mallinfo mi = dlmallinfo();;
	lua_pushinteger(L, mi.fordblks);
	lua_pushinteger(L, mi.uordblks);
	lua_pushinteger(L, mi.usmblks);
	return 3;
}

static int standby(lua_State *L)
{
	PWR_EnterSTANDBYMode();
	return 0;
}

static int systick_get_raw_wrapper(lua_State *L)
{
	lua_pushnumber(L, systick_get_raw());
	return 1;
}


#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
extern const LUA_REG_TYPE sha1_map[];

const LUA_REG_TYPE ruuvi_map[] = {
#if LUA_OPTIMIZE_MEMORY > 0
	{ LSTRKEY("hello") ,         LFUNCVAL(hello) },
	{ LSTRKEY("reset") ,         LFUNCVAL(reset) },
	{ LSTRKEY("idleloop"),       LFUNCVAL(idleloop) },
	{ LSTRKEY("delay_ms"),       LFUNCVAL(l_delay_ms) },
	{ LSTRKEY( "malloc_stats" ), LFUNCVAL(print_mem) },
	{ LSTRKEY( "standby_mode" ), LFUNCVAL(standby) },
	/* SHA1 table ruuvi.sha1 */
	{ LSTRKEY( "sha1" ),         LROVAL( sha1_map ) },
	/* export systick_get_raw as Arduino style millis() */
	{ LSTRKEY( "millis" ),       LFUNCVAL( systick_get_raw_wrapper ) },
#endif
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_ruuvi( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
	return 0;
#else
#error "Optimize memory=0 is not supported"
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}


