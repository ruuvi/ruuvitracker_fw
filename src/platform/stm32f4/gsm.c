/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */

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
#include "ringbuff.h"

#ifdef BUILD_GSM

// For plaform_pio_op() portnums
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2
#define PORT_D 3
#define PORT_E 4

#define STATUS_PIN GPIO_Pin_12
#define STATUS_PORT PORT_E
#define DTR_PIN    GPIO_Pin_14
#define DTR_PORT   PORT_C
#define POWER_PIN  GPIO_Pin_2
#define POWER_PORT PORT_E
#define ENABLE_PIN  GPIO_Pin_15
#define ENABLE_PORT PORT_C

/* Ring buffer for line input */
#define BUFF_SIZE	256
struct rbuff *_buf;

/* Setup IO ports */
void gsm_setup_io()
{
  // Power pin (PE.2)
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_SET);
  // DTR pin (PC14)
  platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_CLEAR);
  // Status pin (PE12)
  platform_pio_op(STATUS_PORT, STATUS_PIN, PLATFORM_IO_PIN_DIR_INPUT);
  // Enable_voltage (PC15)
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);

  _buf = rbuff_new(BUFF_SIZE);

  // Serial port
  platform_uart_setup( GSM_UART_ID, 115200, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1);
  platform_s_uart_set_flow_control( GSM_UART_ID, PLATFORM_UART_FLOW_CTS | PLATFORM_UART_FLOW_RTS);
}

/**
 * GSM Uart handler.
 * Receives bytes from serial port interrupt handler.
 * Calls gsm_line_received() after each full line buffered.
 */
void gsm_uart_received(u8 c)
{
  if (('\n' == c) || ('\r' == c)) { //Line feed
    gsm_line_received();
  } else {
    rbuff_push(_buf, c);
  }
}

/**
 * GSM command and state machine handler.
 * Receives lines from Simcom serial interfaces and parses them.
 */
void gsm_line_received()
{
}

void gsm_write(char *line);
void gsm_setup_io();

/* Enable GSM module */
static int gsm_enable(lua_State *L)
{
  return 0;
}



/* Export Lua GSM library */

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

const LUA_REG_TYPE gsm_map[] =
{
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY("enable") , LFUNCVAL(gsm_enable) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_gsm( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else
#error "Optimize memory=0 is not supported"
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

#endif	/* BUILD_GSM */
