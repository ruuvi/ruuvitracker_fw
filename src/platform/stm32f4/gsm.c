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
#include "gsm.h"
#include <delay.h>
#include <string.h>
#include <stdlib.h>

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

#define BUFF_SIZE	64

#define GSM_CMD_LINE_END "\r\n"


enum State {
  STATE_UNKNOWN = 0,
  STATE_OFF = 1,
  STATE_BOOTING,
  STATE_ASK_PIN,
  STATE_WAIT_NETWORK,
  STATE_READY,
  STATE_ERROR,
};
enum CFUN {
  CFUN_0 = 0,
  CFUN_1 = 1,
};
enum GSM_FLAGS {
  HW_FLOW_ENABLED = 0x01,
  SIM_INSERTED    = 0x02,
  GPS_READY       = 0x04,
};

/* Modem Status */
struct gsm_modem {
  enum Power_mode power_mode;
  enum State state;
  volatile int waiting_reply;
  volatile int waiting_pattern;
  enum Reply reply;
  int flags;
  enum CFUN cfun;
} static gsm = {		/* Initial status */
  .power_mode=POWER_OFF,
  .state = STATE_OFF,
};


/* Handler functions for AT replies*/
static void handle_ok();
static void handle_fail();
static void handle_error();
static void set_hw_flow();
static void cfun_1();
static void gpsready();
static void sim_inserted();

typedef struct Message Message;
struct Message {
  char *msg;			/* Message string */
  enum State next_state;	/* Atomatic state transition */
  void (*func)();		/* Function to call on message */
};

/* Messages from modem */
static Message urc_messages[] = {
  /* Unsolicited Result Codes (URC messages) */
  { "RDY",                  .next_state=STATE_BOOTING },
  { "+CPIN: NOT INSERTED",  .next_state=STATE_ERROR },
  { "+CPIN: READY",         .next_state=STATE_WAIT_NETWORK, .func = sim_inserted },
  { "+CPIN: SIM PIN",       .next_state=STATE_ASK_PIN, .func = sim_inserted },
  { "+CFUN: 1",             .func = cfun_1 },
  { "Call Ready",           .next_state=STATE_READY },
  { "RING" },
  { "GPS Ready",            .func = gpsready },
  { "NORMAL POWER DOWN",     .next_state=STATE_OFF },
  /* Return codes */
  { "OK",   .func = handle_ok },
  { "FAIL", .func = handle_fail },
  { "ERROR",.func = handle_error },
  { NULL } /* Table must end with NULL */
};

static void sim_inserted()
{
  gsm.flags |= SIM_INSERTED;
}

static void gpsready()
{
  gsm.flags |= GPS_READY;
}

static void cfun_1()
{
  gsm.cfun = CFUN_1;
}
static void handle_ok()
{
  gsm.reply = AT_OK;
  gsm.waiting_reply = 0;
}

static void handle_fail()
{
  gsm.reply = AT_FAIL;
  gsm.waiting_reply = 0;
}

static void handle_error()
{
  gsm.reply = AT_ERROR;
  gsm.waiting_reply = 0;
}


/**
 * Send AT command to modem.
 * Waits for modem to reply with 'OK', 'FAIL' or 'ERROR'
 * return AT_OK, AT_FAIL or AT_ERROR
 */
int gsm_cmd(const char *cmd)
{
  gsm.waiting_reply = 1;

  gsm_uart_write(cmd);
  gsm_uart_write(GSM_CMD_LINE_END);

  while(gsm.waiting_reply)
    delay_ms(1);

  return gsm.reply;
}

/* Wait for specific pattern */
int gsm_wait(const char *pattern)
{
  const char *p = pattern;
  char c;

  gsm.waiting_pattern = 1;

  while(*p) {
    c = platform_s_uart_recv(GSM_UART_ID, 1);
    if (c == *p) { //Match
      p++;
    } else { //No match, go back to start
      p = pattern;
    }
  }

  gsm.waiting_pattern = 0;

  return gsm.reply;
}

/* Setup IO ports. Called in platform_setup() from platform.c */
void gsm_setup_io()
{
  /* Power pin (PE.2) */
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_SET);
  /* DTR pin (PC14) */
  platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_CLEAR);
  /* Status pin (PE12) */
  platform_pio_op(STATUS_PORT, STATUS_PIN, PLATFORM_IO_PIN_DIR_INPUT);
  /* Enable_voltage (PC15) */
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);

  // Serial port
  platform_uart_setup( GSM_UART_ID, 115200, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1);
  platform_s_uart_set_flow_control( GSM_UART_ID, PLATFORM_UART_FLOW_CTS | PLATFORM_UART_FLOW_RTS);
}

static void set_hw_flow()
{
  while(gsm.state < STATE_BOOTING)
    delay_ms(1);

  if (AT_OK == gsm_cmd("AT+IFC=2,2")) {
    gsm.flags |= HW_FLOW_ENABLED;
    gsm_cmd("ATE0"); 		/* Disable ECHO */
  }
}

void gsm_enable_voltage()
{
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);
  delay_ms(100);		/* Give 100ms for voltage to settle */
}

void gsm_disable_voltage()
{
  platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);
}

void gsm_toggle_power_pin()
{
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_CLEAR);
  delay_ms(2000);
  platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_SET);
}

/* Send *really* slow. Add inter character delays */
static void slow_send(const char *line) {
  while(*line) {
      platform_s_uart_send(GSM_UART_ID, *line);
    delay_ms(10);		/* inter-char delay */
     if ('\r' == *line)
       delay_ms(100);		/* Inter line delay */
     line++;
  }
}

void gsm_uart_write(const char *line)
{
  if (!gsm.flags&HW_FLOW_ENABLED)
    return slow_send(line);

  while(*line) {
    platform_s_uart_send(GSM_UART_ID, *line);
    line++;
  }
}

/* Find message matching current line */
static Message *lookup_urc_message(const char *line)
{
  int n;
  for(n=0; urc_messages[n].msg; n++) {
    if (0 == strncmp(line, urc_messages[n].msg, strlen(urc_messages[n].msg))) {
      return &urc_messages[n];
    }
  }
  return NULL;
}

/**
 * GSM command and state machine handler.
 * Receives lines from Simcom serial interfaces and parses them.
 */
void gsm_line_received()
{
  Message *m;
  char buf[BUFF_SIZE];
  int c,i=0;

  /* Dont mess with patterns */
  if (gsm.waiting_pattern)
    return;

  while('\n' != (c=platform_s_uart_recv(GSM_UART_ID,0))) {
    if (-1 == c)
      break;
    if ('\r' == c)
      continue;
    buf[i++] = (char)c;
    if(i==BUFF_SIZE)
      break;
  }
  buf[i] = 0;
  printf("GSM: %s\n", buf);

  m = lookup_urc_message(buf);
  if (m) {
    if (m->next_state) {
      gsm.state = m->next_state;
    }
    if (m->func) {
      m->func();
    }
  }
}




/* -------------------- L U A   I N T E R F A C E --------------------*/

/* LUA Application interface */

int gsm_send_cmd(lua_State *L) {
  const char *cmd;
  int ret;

  cmd = luaL_checklstring(L, -1, NULL);
  if (!cmd)
    return 0;

  ret = gsm_cmd(cmd);
  lua_pushinteger(L, ret);
  return 1;
}

int gsm_is_pin_required(lua_State *L)
{
  /* Wait for modem to boot */
  while( gsm.state < STATE_ASK_PIN )
    delay_ms(1);		/* Allow uC to sleep */

  lua_pushboolean(L, STATE_ASK_PIN == gsm.state);
  return 1;
}

int gsm_send_pin(lua_State *L)
{
  int ret;
  char cmd[20];
  const char *pin = luaL_checklstring(L, -1, NULL);
  if (!pin)
    return 0;
  snprintf(cmd,20,"AT+CPIN=%s", pin);

  /* Wait for modem to boot */
  while( gsm.state < STATE_ASK_PIN )
    delay_ms(1);		/* Allow uC to sleep */

  if (STATE_ASK_PIN == gsm.state) {
    ret = gsm_cmd(cmd);
    lua_pushinteger(L, ret);
  } else {
    lua_pushinteger(L, AT_OK);
  }
  return 1;
}
int gsm_is_gprs_enabled(lua_State *L);
int gsm_gprs_enable(lua_State *L);

static const char ctrlZ[] = {26, 0};
int gsm_send_sms(lua_State *L)
{
  const char *number;
  const char *text;
  int ret;
  
  number = luaL_checklstring(L, -2, NULL);
  text = luaL_checklstring(L, -1, NULL);
  if (!number || !text)
    return 0;

  if (AT_OK != gsm_cmd("AT+CMGF=1")) {		/* Set to text mode */
    lua_pushinteger(L, AT_ERROR);
    return 1;
  }

  gsm_uart_write("AT+CMGS=\"");
  gsm_uart_write(number);

  gsm_uart_write("\"\r");
  gsm_wait(">");	 /* Send SMS cmd and wait for '>' to appear */
  gsm_uart_write(text);
  ret = gsm_cmd(ctrlZ);		/* CTRL-Z ends message */
  lua_pushinteger(L, ret);
  return 1;
}

int gsm_lua_wait(lua_State *L)
{
  int ret;
  const char *text;
  text = luaL_checklstring(L, -1, NULL);
  if (!text)
    return 0;
  ret = gsm_wait(text);
  lua_pushinteger(L, ret);
  return 1;
}

int gsm_state(lua_State *L)
{
  lua_pushinteger(L, gsm.state);
  return 1;
}

/* Enable GSM module */
int gsm_set_power_state(lua_State *L)
{
  enum Power_mode next = luaL_checkinteger(L, -1);
  int status_pin = platform_pio_op(STATUS_PORT, STATUS_PIN, PLATFORM_IO_PIN_GET);

  switch(next) {
  case POWER_ON:
    if (0 == status_pin) {
      gsm_enable_voltage();
      gsm_toggle_power_pin();
      set_hw_flow();
    }
    break;
  case POWER_OFF:
    if (1 == status_pin) {
      gsm_toggle_power_pin();
    }
    break;
  }
  return 0;
}

int gsm_is_ready(lua_State *L)
{
  lua_pushboolean(L, gsm.state == STATE_READY);
  return 1;
}

int gsm_flag_is_set(lua_State *L)
{
  int flag = luaL_checkinteger(L, -1);
  lua_pushboolean(L, (gsm.flags&flag) != 0);
  return 1;
}

/* Export Lua GSM library */

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

const LUA_REG_TYPE gsm_map[] =
{
#if LUA_OPTIMIZE_MEMORY > 0
  /* Functions, Map name to func (I was lazy)*/
#define F(name,func) { LSTRKEY(#name), LFUNCVAL(func) }
  { LSTRKEY("set_power_state") , LFUNCVAL(gsm_set_power_state) },
  F(is_ready, gsm_is_ready),
  F(flag_is_set, gsm_flag_is_set),
  F(state, gsm_state),
  F(is_pin_required, gsm_is_pin_required),
  F(send_pin, gsm_send_pin),
  F(send_sms, gsm_send_sms),
  F(cmd, gsm_send_cmd),
  F(wait, gsm_lua_wait),

  /* CONSTANTS */
#define MAP(a) { LSTRKEY(#a), LNUMVAL(a) }
  MAP( POWER_ON ),
  MAP( POWER_OFF ),
  MAP( STATE_OFF),
  MAP( STATE_BOOTING ),
  MAP( STATE_ASK_PIN ),
  MAP( STATE_WAIT_NETWORK ),
  MAP( STATE_READY ),
  MAP( STATE_ERROR ),
  MAP( GPS_READY ),
  MAP( SIM_INSERTED ),
  { LSTRKEY("OK"), LNUMVAL(AT_OK) },
  { LSTRKEY("FAIL"), LNUMVAL(AT_FAIL) },
  { LSTRKEY("ERROR"), LNUMVAL(AT_ERROR) },
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
