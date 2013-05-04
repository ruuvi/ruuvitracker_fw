/*
 *  Simcom 908 GPS Driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski

AT+CGPSPWR=1 -> Power up GPS engine (and 2V8 antenna power)
AT+CGPSRST=0 -> Cold start (after this NMEA data starts to flow from USART2
AT+CGPSRST=1 -> Warm start (after this NMEA data starts to flow from USART2
AT+CGPSPWR=0 -> Power down GPS engine (and 2V8 antenna power)

// Memorizing GPS configurations' commands into Code Memory
char strGPS_PowerUp		[] PROGMEM = "AT+CGPSPWR=1\r\n";
char strGPS_Mode1		[] PROGMEM = "AT+CGPSRST=1\r\n";
char strGPS_BaudRate	[] PROGMEM = "AT+CGPSIPR=9600\r\n";
char strGPS_OutPut		[] PROGMEM = "AT+CGPSOUT=32\r\n";
char strGPS_PowerDown	[] PROGMEM = "AT+CGPSPWR=0\r\n";

$GPGGA,000325.002,,,,,0,0,,,M,,M,,*4E
$GPGLL,,,,,000325.002,V,N*7C
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPGSV,3,1,09,03,,,36,04,,,33,05,,,28,08,,,33*77
$GPGSV,3,2,09,16,,,19,20,,,36,21,,,32,26,,,34*78
$GPGSV,3,3,09,29,,,29*70
$GPRMC,000325.002,V,,,,,,,,,,N*4B
$GPVTG,,T,,M,,N,,K,N*2C

$GPGGA,093222.000,6500.722951,N,02530.995761,E,1,9,0.94,20.487,M,21.546,M,,*65
$GPGLL,6500.722951,N,02530.995761,E,093222.000,A,A*59
$GPGSA,A,3,13,10,23,07,16,02,04,29,08,,,,1.80,0.94,1.54*01
$GPGSV,3,1,12,13,70,121,29,10,61,250,42,23,44,104,29,07,40,194,41*76
$GPGSV,3,2,12,16,35,074,22,02,33,271,44,05,21,304,,04,20,230,39*73
$GPGSV,3,3,12,29,17,345,43,08,14,204,35,20,00,146,,33,,,33*4E
$GPRMC,093222.000,A,6500.722951,N,02530.995761,E,0.000,0.0,140413,,,A*6D
$GPVTG,0.0,T,,M,0.000,N,0.000,K,A*0D
$GPZDA,093222.000,14,04,2013,,*5F
*/

#include <string.h>
#include <stdlib.h>
#include <delay.h>
#include "platform.h"
#include "platform_conf.h"
#include "common.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "gps.h"

#ifdef BUILD_GPS

/* Lua API 2 */
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

#define BUFF_SIZE	256

enum State {
  STATE_UNKNOWN = 0,
  STATE_OFF = 1,
  STATE_ON = 2,
  STATE_HAS_FIX = 3,
  STATE_ERROR = 4,
};

/* GPS device status */
struct gps_device {
  enum State state;
} static gps = {	/* Initial status */
  .state = STATE_OFF
};



int gps_has_fix(lua_State *L)
{
	lua_pushboolean(L, gps.state == STATE_HAS_FIX);
	return 1;
}

void gps_set_power_state(lua_State *L)
{
	// TODO: move stuff from pc-project to here
}

int gps_get_location(lua_State *L)
{
	// TODO: move stuff from pc-project to here
 	//lua_pushlstring(L, RMCLatitude, sizeof(RMCLatitude));
  
  return 1;
}



/* ===================Internal functions===================== */

/* Setup IO ports. Called in platform_setup() from platform.c */
void gps_setup_io()
{
  // Setup serial port, GPS_UART_ID == 2 @ RUUVIB1
  platform_uart_setup( GPS_UART_ID, 115200, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1);
  platform_s_uart_set_flow_control( GPS_UART_ID, PLATFORM_UART_FLOW_CTS | PLATFORM_UART_FLOW_RTS);
}

/**
 * Receives lines from Simcom serial interfaces and parses them.
 */
void gps_line_received()
{

// TODO: platform_s_uart_recv(GSM_UART_ID,1) <- why the '1'?

  char buf[BUFF_SIZE];
  int c, i = 0;

  while('\n' != (c=platform_s_uart_recv(GPS_UART_ID, 0))) {
    if (-1 == c)
      break;
    if ('\r' == c)
      continue;
    buf[i++] = (char)c;
    if(i==BUFF_SIZE)
      break;
  }
  buf[i] = 0;
  printf("GPS: %s\n", buf);
}




const LUA_REG_TYPE gps_map[] =
{
#if LUA_OPTIMIZE_MEMORY > 0
  /* FUNCTIONS */
  { LSTRKEY("set_power_state") , LFUNCVAL(gps_set_power_state) },
  { LSTRKEY("has_fix") , LFUNCVAL(gps_has_fix) },
  { LSTRKEY("get_location") , LFUNCVAL(gps_get_location) },

  /* CONSTANTS */
#define MAP(a) { LSTRKEY(#a), LNUMVAL(a) }
  MAP( STATE_UNKNOWN),
  MAP( STATE_OFF),
  MAP( STATE_ON ),
  MAP( STATE_HAS_FIX ),
  MAP( STATE_ERROR ),
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_gps( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else
#error "Optimize memory=0 is not supported"
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

#endif	/* BUILD_GSM */

