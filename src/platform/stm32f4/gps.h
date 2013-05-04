/*
 *  Simcom 908 GPS driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski
 */
#ifndef _GPS_H_
#define _GPS_H_

#include "lua.h"
#include "lualib.h"


/* C-API */

/* LUA Application interface */
void gps_set_power_state(lua_State *L);
int gps_has_fix(lua_State *L);
int gps_get_location(lua_State *L);

/* Internals */
int luaopen_gps( lua_State *L );
void gps_setup_io();
void gps_line_received();

#endif

