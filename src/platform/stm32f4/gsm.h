/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */
#ifndef _GSM_H
#define _GSM_H

#include "lua.h"
#include "lualib.h"

enum Power_mode { POWER_OFF=0, POWER_ON };
enum Reply { AT_OK=0, AT_FAIL, AT_ERROR, AT_TIMEOUT };

/* C-API */
int gsm_cmd(const char *cmd); 	/* Send AT command to modem. Returns AT_OK, AT_FAIL or AT_ERROR */
int gsm_wait(const char *pattern, int timeout, char *line); /* wait for pattern to appear */
void gsm_uart_write(const char *line);
void gsm_set_power_state(enum Power_mode mode);
int gsm_is_gps_ready();         /* Check if GPS flag is set in GSM */
int gsm_read_line(char *buf, int max_len);
int gsm_read_raw(char *buf, int max_len);

/* Internals */
int luaopen_gsm( lua_State *L );
void gsm_line_received();
void gsm_setup_io();
void gsm_toggle_power_pin();
void gsm_enable_voltage();
void gsm_disable_voltage();
void gsm_set_raw_mode();
void gsm_disable_raw_mode();
int gsm_is_raw_mode();
int gsm_int_dummy_set_status( elua_int_resnum resnum, int state );
int gsm_int_dummy_get_status( elua_int_resnum resnum );
int gsm_int_call_get_flag  ( elua_int_resnum resnum, int clear );
int gsm_int_sms_get_flag  ( elua_int_resnum resnum, int clear );

#endif	/* GSM_H */
