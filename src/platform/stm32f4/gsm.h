/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */
#ifndef _GSM_H
#define _GSM_H

#include "lua.h"
#include "lualib.h"

/* LUA Application interface */
int gsm_send_cmd(lua_State *L);
int gsm_set_power_state(lua_State *L);
int gsm_is_pin_required(lua_State *L);
int gsm_send_pin(lua_State *L);
int gsm_is_ready(lua_State *L);
int gsm_is_gprs_enabled(lua_State *L);
int gsm_gprs_enable(lua_State *L);
int gsm_send_sms(lua_State *L);
int gsm_state(lua_State *L);

/* Internals */
int gsm_cmd(const char *cmd);
int luaopen_gsm( lua_State *L );
void gsm_uart_received(u8 c);
void gsm_uart_write(const char *line);
void gsm_line_received();
void gsm_setup_io();
void gsm_toggle_power_pin();
void gsm_enable_voltage();
void gsm_disable_voltage();

#endif	/* GSM_H */
