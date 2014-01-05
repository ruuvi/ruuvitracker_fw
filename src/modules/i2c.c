// Module for interfacing with the I2C interface

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ruuvi_errors.h"

// Lua: speed = i2c.setup( id, speed )
static int i2c_setup( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );
	s32 speed = ( s32 )luaL_checkinteger( L, 2 );

	MOD_CHECK_ID( i2c, id );
	if (speed <= 0)
		return luaL_error( L, "frequency must be > 0" );
	lua_pushinteger( L, platform_i2c_setup( id, (u32)speed ) );
	return 1;
}

// Lua: i2c.start( id )
static int i2c_start( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );

	MOD_CHECK_ID( i2c, id );
	rt_error status = platform_i2c_send_start( id );
	switch(status)
	{
		case RT_ERR_OK:
			break;
		case RT_ERR_TIMEOUT:
			return luaL_error( L, "Timeout when sending start" );
			break;
		default:
		case RT_ERR_ERROR:
			return luaL_error( L, "Unknown error when sending start" );
			break;
	}
	return 0;
}

// Lua: i2c.stop( id )
static int i2c_stop( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );

	MOD_CHECK_ID( i2c, id );
	rt_error status = platform_i2c_send_stop( id );
	switch(status)
	{
		case RT_ERR_OK:
			break;
		case RT_ERR_TIMEOUT:
			return luaL_error( L, "Timeout when sending stop" );
			break;
		default:
		case RT_ERR_ERROR:
			return luaL_error( L, "Unknown error when sending stop" );
			break;
	}
	return 0;
}

// Lua: i2c.address( id, address, direction )
static int i2c_address( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );
	int address = luaL_checkinteger( L, 2 );
	int direction = luaL_checkinteger( L, 3 );

	MOD_CHECK_ID( i2c, id );
	if ( address < 0 || address > 127 )
		return luaL_error( L, "slave address must be from 0 to 127" );
	rt_error status = platform_i2c_send_address( id, (u16)address, direction );
	switch(status)
	{
		case RT_ERR_OK:
			lua_pushboolean( L, 1);
			return 1;
			break;
		case RT_ERR_FAIL:
			lua_pushboolean( L, 0);
			return 1;
			break;
		case RT_ERR_TIMEOUT:
			return luaL_error( L, "Timeout when sending address" );
			break;
		default:
		case RT_ERR_ERROR:
			return luaL_error( L, "Unknown error when sending address" );
			break;
	}
}

// Lua: wrote = i2c.write( id, data1, [data2], ..., [datan] )
// data can be either a string, a table or an 8-bit number
static int i2c_write( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );
	const char *pdata;
	size_t datalen, i;
	int numdata;
	u32 wrote = 0;
	unsigned argn;
	rt_error status;

	MOD_CHECK_ID( i2c, id );
	if( lua_gettop( L ) < 2 )
		return luaL_error( L, "invalid number of arguments" );
	for( argn = 2; argn <= lua_gettop( L ); argn ++ )
	{
		// lua_isnumber() would silently convert a string of digits to an integer
		// whereas here strings are handled separately.
		if( lua_type( L, argn ) == LUA_TNUMBER )
		{
			numdata = ( int )luaL_checkinteger( L, argn );
			if( numdata < 0 || numdata > 255 )
				return luaL_error( L, "numeric data must be from 0 to 255" );
			status = platform_i2c_send_byte( id, numdata );
			switch(status)
			{
				case RT_ERR_OK:
					break;
				case RT_ERR_TIMEOUT:
					return luaL_error( L, "Timeout when sending data" );
					break;
				default:
				case RT_ERR_ERROR:
					return luaL_error( L, "Unknown error when sending data" );
					break;
			}
			wrote ++;
		}
		else if( lua_istable( L, argn ) )
		{
			datalen = lua_objlen( L, argn );
			for( i = 0; i < datalen; i ++ )
			{
				lua_rawgeti( L, argn, i + 1 );
				numdata = ( int )luaL_checkinteger( L, -1 );
				lua_pop( L, 1 );
				if( numdata < 0 || numdata > 255 )
					return luaL_error( L, "numeric data must be from 0 to 255" );
				status = platform_i2c_send_byte( id, numdata );
				switch(status)
				{
					case RT_ERR_OK:
						break;
					case RT_ERR_TIMEOUT:
						return luaL_error( L, "Timeout when sending data" );
						break;
					default:
					case RT_ERR_ERROR:
						return luaL_error( L, "Unknown error when sending data" );
						break;
				}
			}
			wrote += i;
			if( i < datalen )
				break;
		}
		else
		{
			pdata = luaL_checklstring( L, argn, &datalen );
			for( i = 0; i < datalen; i ++ )
			{
				status = platform_i2c_send_byte( id, pdata[ i ] );
				switch(status)
				{
					case RT_ERR_OK:
						break;
					case RT_ERR_TIMEOUT:
						return luaL_error( L, "Timeout when sending data" );
						break;
					default:
					case RT_ERR_ERROR:
						return luaL_error( L, "Unknown error when sending data" );
						break;
				}
			}
			wrote += i;
			if( i < datalen )
				break;
		}
	}
	lua_pushinteger( L, wrote );
	return 1;
}

// Lua: read = i2c.read( id, size )
static int i2c_read( lua_State *L )
{
	unsigned id = luaL_checkinteger( L, 1 );
	u32 size = ( u32 )luaL_checkinteger( L, 2 ), i;
	luaL_Buffer b;
	u8 data;
	rt_error status;

	MOD_CHECK_ID( i2c, id );
	if( size == 0 )
		return 0;
	luaL_buffinit( L, &b );
	for( i = 0; i < size; i ++ )
	{
		status = platform_i2c_recv_byte( id, i < size - 1, &data );
		switch(status)
		{
			case RT_ERR_OK:
				break;
			case RT_ERR_TIMEOUT:
				return luaL_error( L, "Timeout when receiving data" );
				break;
			default:
			case RT_ERR_ERROR:
				return luaL_error( L, "Unknown error when receiving data" );
				break;
		}
		luaL_addchar( &b, ( char )data );
	}
	luaL_pushresult( &b );
	return 1;
}


static int _i2c_read_8_16(char width, lua_State *L )
{
	int rc;
	rt_error status;
	unsigned id = luaL_checkinteger(L, 1);
	int dev     = luaL_checkinteger(L, 2);
	int addr    = luaL_checkinteger(L, 3);
	int size   = luaL_checkinteger(L, 4);
	u8 *data;

	MOD_CHECK_ID( i2c, id );
	if( size == 0 )
		return 0;
	data = malloc(size);
	if (8 == width)
		status = platform_i2c_read8(id, dev, addr, data, size, &rc);
	else
		status = platform_i2c_read16(id, dev, addr, data, size, &rc);
	switch(status)
	{
		case RT_ERR_OK:
			break;
		case RT_ERR_TIMEOUT:
			return luaL_error( L, "Timeout from platform_i2c_read8/16 (actual subfunction unknown)" );
			break;
		default:
		case RT_ERR_ERROR:
			return luaL_error( L, "Unknown error from platform_i2c_read8/16 (actual subfunction unknown)" );
			break;
	}
	if (0 == rc)
		return 0;
	if (1 == size)
		lua_pushinteger(L, data[0]);
	else
		lua_pushlstring(L, (void*)data, size);
	free(data);
	return 1;
}

static int i2c_read_8(lua_State *L)
{
	return _i2c_read_8_16(8, L);
}
static int i2c_read_16(lua_State *L)
{
	return _i2c_read_8_16(16, L);
}

static int _i2c_write_8_16(char width, lua_State *L )
{
	int ret;
	rt_error status;
	unsigned id = luaL_checkinteger(L, 1);
	int dev     = luaL_checkinteger(L, 2);
	int addr    = luaL_checkinteger(L, 3);
	u8 *buff = 0;
	int argn,i,index=0,len=0;
	unsigned int datalen, numdata;
	const char *pdata;

	MOD_CHECK_ID( i2c, id );
	if( lua_gettop( L ) < 4 )
		return luaL_error( L, "invalid number of arguments" );
	for( argn = 4; argn <= lua_gettop( L ); argn ++ ) {
		// lua_isnumber() would silently convert a string of digits to an integer
		// whereas here strings are handled separately.
		if( lua_type( L, argn ) == LUA_TNUMBER ) {
			numdata = ( int )luaL_checkinteger( L, argn );
			if( numdata < 0 || numdata > 255 )
				return luaL_error( L, "numeric data must be from 0 to 255" );
			len++;
			buff = realloc(buff, len);
			buff[index++] = (u8)numdata;
		} else if( lua_istable( L, argn ) ) {
			datalen = lua_objlen( L, argn );
			for( i = 0; i < datalen; i ++ ) {
				lua_rawgeti( L, argn, i + 1 );
				numdata = ( int )luaL_checkinteger( L, -1 );
				lua_pop( L, 1 );
				if( numdata < 0 || numdata > 255 )
					return luaL_error( L, "numeric data must be from 0 to 255" );
				len++;
				buff = realloc(buff, len);
				buff[index++] = (u8)numdata;
			}
		} else {
			pdata = luaL_checklstring( L, argn, &datalen );
			len += datalen;
			buff = realloc(buff, len);
			memcpy(&buff[index], pdata, datalen);
			index+=datalen;
		}
	}
	if (8 == width)
		status = platform_i2c_write8(id, dev, addr, buff, len, &ret);
	else
		status = platform_i2c_write16(id, dev, addr, buff, len, &ret);
	free(buff);
	switch(status)
	{
		case RT_ERR_OK:
			break;
		case RT_ERR_TIMEOUT:
			return luaL_error( L, "Timeout from platform_i2c_write8/16 (actual subfunction unknown)" );
			break;
		default:
		case RT_ERR_ERROR:
			return luaL_error( L, "Unknown error from platform_i2c_write8/16 (actual subfunction unknown)" );
			break;
	}
	lua_pushinteger(L, ret);
	return 1;
}

static int i2c_write_8(lua_State *L)
{
	return _i2c_write_8_16(8, L);
}
static int i2c_write_16(lua_State *L)
{
	return _i2c_write_8_16(16, L);
}

// Module function map
#define MIN_OPT_LEVEL   2
#include "lrodefs.h"
const LUA_REG_TYPE i2c_map[] = {
	{ LSTRKEY( "setup" ),  LFUNCVAL( i2c_setup ) },
	{ LSTRKEY( "start" ), LFUNCVAL( i2c_start ) },
	{ LSTRKEY( "stop" ), LFUNCVAL( i2c_stop ) },
	{ LSTRKEY( "address" ), LFUNCVAL( i2c_address ) },
	{ LSTRKEY( "write" ), LFUNCVAL( i2c_write ) },
	{ LSTRKEY( "read" ), LFUNCVAL( i2c_read ) },
	{ LSTRKEY( "read8" ), LFUNCVAL( i2c_read_8 ) },
	{ LSTRKEY( "read16" ), LFUNCVAL( i2c_read_16 ) },
	{ LSTRKEY( "write8" ), LFUNCVAL( i2c_write_8 ) },
	{ LSTRKEY( "write16" ), LFUNCVAL( i2c_write_16 ) },
#if LUA_OPTIMIZE_MEMORY > 0
	{ LSTRKEY( "FAST" ), LNUMVAL( PLATFORM_I2C_SPEED_FAST ) },
	{ LSTRKEY( "SLOW" ), LNUMVAL( PLATFORM_I2C_SPEED_SLOW ) },
	{ LSTRKEY( "TRANSMITTER" ), LNUMVAL( PLATFORM_I2C_DIRECTION_TRANSMITTER ) },
	{ LSTRKEY( "RECEIVER" ), LNUMVAL( PLATFORM_I2C_DIRECTION_RECEIVER ) },
#endif
	{ LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_i2c( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
	return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
	luaL_register( L, AUXLIB_I2C, i2c_map );

	// Add the stop bits and parity constants (for uart.setup)
	MOD_REG_NUMBER( L, "FAST", PLATFORM_I2C_SPEED_FAST );
	MOD_REG_NUMBER( L, "SLOW", PLATFORM_I2C_SPEED_SLOW );
	MOD_REG_NUMBER( L, "TRANSMITTER", PLATFORM_I2C_DIRECTION_TRANSMITTER );
	MOD_REG_NUMBER( L, "RECEIVER", PLATFORM_I2C_DIRECTION_RECEIVER );

	return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

