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
#include "lrotable.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "gsm.h"
#include "ringbuff.h"
#include <delay.h>
#include <string.h>
#include <stdlib.h>
#include <slre.h>

#ifdef BUILD_GSM

#ifdef DEBUG
#define D_ENTER() printf("%s:%s(): enter\n", __FILE__, __FUNCTION__)
#define D_EXIT() printf("%s:%s(): exit\n", __FILE__, __FUNCTION__)
#define _DEBUG(fmt, args...) printf("%s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#else
#define D_ENTER()
#define D_EXIT()
#define _DEBUG(fmt, args...)
#endif

// For plaform_pio_op() portnums
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2
#define PORT_D 3
#define PORT_E 4

#if defined( ELUA_BOARD_RUUVIB1 )
#define STATUS_PIN GPIO_Pin_12
#define STATUS_PORT PORT_E
#define DTR_PIN    GPIO_Pin_14
#define DTR_PORT   PORT_C
#define POWER_PIN  GPIO_Pin_2
#define POWER_PORT PORT_E
#define ENABLE_PIN  GPIO_Pin_15
#define ENABLE_PORT PORT_C
#elif defined( ELUA_BOARD_RUUVIC1 )
#define STATUS_PIN  GPIO_Pin_15
#define STATUS_PORT PORT_C
#define DTR_PIN     GPIO_Pin_5
#define DTR_PORT    PORT_C
#define POWER_PIN   GPIO_Pin_4
#define POWER_PORT  PORT_C
#define ENABLE_PIN  GPIO_Pin_7
#define ENABLE_PORT PORT_C
#else
#error "Define GSM pins in gsm.c"
#endif

#define BUFF_SIZE	64

#define GSM_CMD_LINE_END "\r\n"

#define TIMEOUT_MS   5000        /* Default timeout 5s */
#define TIMEOUT_HTTP 35000      /* Http timeout, 30s */

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
        GPRS_READY      = 0x08,
        CALL            = 0x10,
        INCOMING_CALL   = 0x20,
        TCP_ENABLED     = 0x40,
};

/* Modem Status */
struct gsm_modem {
	enum Power_mode power_mode;
	enum State state;
	volatile int waiting_reply;
	volatile int raw_mode;
	enum Reply reply;
	int flags;
	enum CFUN cfun;
	int last_sms_index;
	char ap_name[64];
} static gsm = {		/* Initial status */
	.power_mode=POWER_OFF,
	.state = STATE_OFF,
};


/* Handler functions for AT replies*/
static void handle_ok(char *line);
static void handle_fail(char *line);
static void handle_error(char *line);
static void set_hw_flow();
static void parse_cfun(char *line);
static void gpsready(char *line);
static void sim_inserted(char *line);
static void no_sim(char *line);
static void incoming_call(char *line);
static void call_ended(char *line);
static void parse_network(char *line);
static void parse_sapbr(char *line);
static void parse_sms_in(char *line);
static void socket_receive(char *line);
static void pdp_off(char *line);
static void socket_closed(char *line);

typedef struct Message Message;
struct Message {
	char *msg;			/* Message string */
	enum State next_state;	/* Atomatic state transition */
	void (*func)();		/* Function to call on message */
};

/* Messages from modem */
static Message urc_messages[] = {
	/* Unsolicited Result Codes (URC messages) */
	{ "RDY",                    .next_state=STATE_BOOTING },
	{ "NORMAL POWER DOWN",      .next_state=STATE_OFF },
	{ "^\\+CPIN: NOT INSERTED", .next_state=STATE_ERROR,        .func = no_sim },
	{ "\\+CPIN: READY",         .next_state=STATE_WAIT_NETWORK, .func = sim_inserted },
	{ "\\+CPIN: SIM PIN",       .next_state=STATE_ASK_PIN,      .func = sim_inserted },
	{ "\\+CFUN:",                                               .func = parse_cfun },
	{ "Call Ready",             .next_state=STATE_READY },
	{ "GPS Ready",       .func = gpsready },
	{ "\\+COPS:",        .func = parse_network },
	{ "\\+SAPBR:",       .func = parse_sapbr },
	{ "\\+PDP: DEACT",   .func = pdp_off },
	{ "\\+RECEIVE",      .func = socket_receive },
	/* Return codes */
	{ "OK",           .func = handle_ok },
	{ "FAIL",         .func = handle_fail },
	{ "ERROR",        .func = handle_error },
	{ "\\+CME ERROR", .func = handle_error },
	{ "\\+CMS ERROR", .func = handle_error },                       /* TODO: handle */
	/* During Call */
	{ "NO CARRIER",   .func = call_ended }, /* This is general end-of-call */
	{ "NO DIALTONE",  .func = call_ended },
	{ "BUSY",         .func = handle_fail },
	{ "NO ANSWER",    .func = handle_fail },
	{ "RING",         .func = incoming_call },
	/* SMS */
	{ "\\+CMTI:",     .func = parse_sms_in },
	/* SOCKET */
	{ "\\d, CLOSED",    .func = socket_closed },
	{ NULL } /* Table must end with NULL */
};

/* Some interrupt helpers */
int gsm_int_dummy_set_status( elua_int_resnum resnum, int state )
{
	return 0;
}
int gsm_int_dummy_get_status( elua_int_resnum resnum )
{
	return ENABLE;
}

int gsm_int_call_get_flag  ( elua_int_resnum resnum, int clear )
{
	return (gsm.flags&INCOMING_CALL)!=0;
}

int gsm_int_sms_get_flag  ( elua_int_resnum resnum, int clear )
{
//TODO: implement
	return 0;
}

static void pdp_off(char *line)
{
	gsm.flags &= ~TCP_ENABLED;
}

static void incoming_call(char *line)
{
	gsm.flags |= INCOMING_CALL;
	cmn_int_handler( INT_GSM_CALL, 0 );
}

static void call_ended(char *line)
{
	gsm.flags &= ~CALL;
	gsm.flags &= ~INCOMING_CALL;
}

static void parse_sms_in(char *line)
{
	if (1 == sscanf(line, "+CMTI: \"SM\",%d", &gsm.last_sms_index)) {
		cmn_int_handler( INT_GSM_SMS, gsm.last_sms_index);
	}
}

static void parse_network(char *line)
{
	char network[64];
	/* Example: +COPS: 0,0,"Saunalahti" */
	if (1 == sscanf(line, "+COPS: 0,0,\"%s", network)) {
		*strchr(network,'"') = 0;
		printf("GSM: Registered to network %s\n", network);
		gsm.state = STATE_READY;
	}
}

#define STATUS_CONNECTING 0
#define STATUS_CONNECTED 1
static const char sapbr_deact[] = "+SAPBR 1: DEACT";

static void parse_sapbr(char *line)
{
	int status;

	/* Example: +SAPBR: 1,1,"10.172.79.111"
	 * 1=Profile number
	 * 1=connected, 0=connecting, 3,4 =closing/closed
	 * ".." = ip addr
	 */
	if (1 == sscanf(line, "+SAPBR: %*d,%d", &status)) {
		switch(status) {
		case STATUS_CONNECTING:
		case STATUS_CONNECTED:
			gsm.flags |= GPRS_READY;
			break;
		default:
			gsm.flags &= ~GPRS_READY;
		}
	} else if (0 == strncmp(line, sapbr_deact, strlen(sapbr_deact))) {
		gsm.flags &= ~GPRS_READY;
	}
}

static void sim_inserted(char *line)
{
	gsm.flags |= SIM_INSERTED;
}

static void no_sim(char *line)
{
	gsm.flags &= ~SIM_INSERTED;
}

static void gpsready(char *line)
{
	gsm.flags |= GPS_READY;
}

static void parse_cfun(char *line)
{
	if (strchr(line,'0')) {
		gsm.cfun = CFUN_0;
	} else if (strchr(line,'1')) {
		gsm.cfun = CFUN_1;
	} else {
		printf("GSM: Unknown CFUN state\n");
	}
}
static void handle_ok(char *line)
{
	gsm.reply = AT_OK;
	gsm.waiting_reply = 0;
}

static void handle_fail(char *line)
{
	gsm.reply = AT_FAIL;
	gsm.waiting_reply = 0;
	printf("GSM: Fail\n");
}

static void handle_error(char *line)
{
	gsm.reply = AT_ERROR;
	gsm.waiting_reply = 0;
	printf("GSM ERROR: %s\n", line);
}


/**
 * Send AT command to modem.
 * Waits for modem to reply with 'OK', 'FAIL' or 'ERROR'
 * return AT_OK, AT_FAIL or AT_ERROR
 */
int gsm_cmd(const char *cmd)
{
	int retry;
	unsigned int t;

	/* Flush buffers */
	gsm_disable_raw_mode();
	while(-1 != platform_s_uart_recv(GSM_UART_ID, 0))
		;;

	gsm.waiting_reply = 1;
	for(retry=0; retry<3; retry++) {
		t = systick_get_raw();                  /* Get System timer current value */
		t += TIMEOUT_MS*systick_get_hz()/1000;

		gsm_uart_write(cmd);
		gsm_uart_write(GSM_CMD_LINE_END);

		while(gsm.waiting_reply) {
			if (t < systick_get_raw()) {
				/* Timeout */
				gsm.reply = AT_TIMEOUT;
				break;
			}
			delay_ms(1);
		}
		if (gsm.reply != AT_TIMEOUT)
			break;
	}
	gsm.waiting_reply = 0;

	if (gsm.reply != AT_OK)
		printf("GSM: '%s' failed (%d)\n", cmd, gsm.reply);

	if (retry == 3) {             /* Modem not responding */
		printf("GSM: Modem not responding, RESETING\n");
		gsm_reset_modem();
		return AT_TIMEOUT;
	}
	return gsm.reply;
}

int gsm_cmd_fmt(const char *fmt, ...)
{
	char cmd[256];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( cmd, 256, fmt, ap );
	va_end( ap );
#ifdef DEBUG
	return gsm_cmd_check(cmd);
#else
	return gsm_cmd(cmd);
#endif
}

int gsm_cmd_check(const char *cmd)
{
	int rc;
	printf("GSM CMD: %s\n", cmd);
	rc = gsm_cmd(cmd);
	if (rc) {
		switch(rc) {
		case AT_TIMEOUT:
			printf("TIMEOUT\n");
			break;
		case AT_FAIL:
			printf("Failed\n");
			break;
		case AT_ERROR:
			printf("Returned error\n");
			break;
		}
	}
	return rc;
}
#ifdef DEBUG
// If we have DEBUG enabled then rewrite all following calls to gsm_cmd to gsm_cmd_check (which does some debug output)
#define gsm_cmd gsm_cmd_check
#endif


void gsm_set_raw_mode()
{
	gsm.raw_mode = 1;
}
void gsm_disable_raw_mode()
{
	gsm.raw_mode = 0;
}
int gsm_is_raw_mode()
{
	return gsm.raw_mode;
}

/* Wait for specific pattern */
int gsm_wait_cpy(const char *pattern, int timeout, char *line, size_t buf_size)
{
	D_ENTER();
	char buf[256];
	int c, ret;
	int i;
	unsigned int t;

	t = systick_get_raw();
	t+=timeout*systick_get_hz()/1000;

	int was_raw_mode = gsm_is_raw_mode();
	gsm_set_raw_mode();

	i = 0;
	ret = AT_OK;
	while(1) {
		c = platform_s_uart_recv(GSM_UART_ID, 0);
		/* Nothing received */
		if (-1 == c) {
			if (t < systick_get_raw()) {
				ret = AT_TIMEOUT;
				goto WAIT_END;
			} else {
				delay_ms(1);
			}
			continue;
		}
		buf[i++] = c;
		buf[i] = 0;
		_DEBUG("buf=%s, pattern=%s\n", buf, pattern);
		if (0 == strcmp("ERROR", buf))
		{
			ret = AT_ERROR;
			goto WAIT_END;
		}
		if (NULL == slre_match(0, pattern, buf, i)) { //Match
			break;
		}
		if (i == 256) //Buffer full, start new
			i = 0;
		if (c == '\n') //End of line, start new buffer at next char
			i = 0;
	}


	if (line)
	{
		strcpy(line, buf);
		i = strlen(line);
		while (1)
		{
			c = platform_s_uart_recv(GSM_UART_ID, 0);
			if(-1 != c)
			{
				line[i++] = c;
				line[i] = 0;
				_DEBUG("strlen(line)=%d line=%s\n", strlen(line), line);
			}
			if ('\n' == c)
			{
				break;
			}
			if (systick_get_raw() > t)
			{
				ret = AT_TIMEOUT;
				break;
			}
		}
	}
WAIT_END:
	D_EXIT();
	if (!was_raw_mode)
		gsm_disable_raw_mode();

	return ret;
}

int gsm_wait(const char *pattern, int timeout)
{
	return gsm_wait_cpy(pattern, timeout, NULL, 0);
}

int gsm_cmd_wait(const char *cmd, const char *response, int timeout)
{
	int r;
	gsm_set_raw_mode();
	gsm_uart_write(cmd);
	gsm_uart_write(GSM_CMD_LINE_END);
	printf("GSM CMD: %s\n", cmd);
	r = gsm_wait(response, timeout);
	gsm_disable_raw_mode();
	return r;
}
int gsm_cmd_wait_fmt(const char *response, int timeout, char *fmt, ...)
{
	char cmd[256];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( cmd, 256, fmt, ap );
	va_end( ap );
	return gsm_cmd_wait(cmd, response, timeout);
}

/* Read line from GSM serial port to buffer */
int gsm_read_line(char *buf, int max_len)
{
	D_ENTER();
	int i,c,t;
	int was_raw = gsm_is_raw_mode();
	gsm_set_raw_mode();
	t = systick_get_raw();
	t+=TIMEOUT_MS*systick_get_hz()/1000;
	for(i=0; i<(max_len-1);) {
		c = platform_s_uart_recv(GSM_UART_ID, 0);
		// If we have a valid byte add it to buffer
		if (-1 != c)
		{
			buf[i++] = c;
			buf[i]=0;
			_DEBUG("strlen(buf)=%d buf=%s\n", strlen(buf), buf);
		}
		// If last received char was newline we have a full line
		if ('\n' == c)
		{
			break;
		}
		// Check for timeout
		if (systick_get_raw() > t)
		{
			// TODO: tell the caller this was a timeout
			// TODO2: normalize the timeout check routines
			break;
		}
		// If we got no data wait 1ms
		if (-1 == c)
		{
			delay_ms(1);
		}
	}
	if (!was_raw)
		gsm_disable_raw_mode();
	D_EXIT();
	return i;
}

int gsm_read_raw(char *buf, int max_len)
{
	int i,c,t;
	int was_raw = gsm_is_raw_mode();
	gsm_set_raw_mode();
	t = systick_get_raw();
	t+=TIMEOUT_MS*systick_get_hz()/1000;
	for(i=0; i<max_len;) {
		c = platform_s_uart_recv(GSM_UART_ID, 0);
		if (-1 != c) {
			buf[i++] = c;
		} else if (systick_get_raw() > t) {
			break;
		} else {
			delay_ms(1);
		}
	}
	if (!was_raw)
		gsm_disable_raw_mode();
	return i;
}

static void gsm_set_serial_speed(int speed)
{
	platform_uart_setup( GSM_UART_ID, speed, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1);
}

static void gsm_set_flow(int enabled)
{
	if (enabled)
		platform_s_uart_set_flow_control( GSM_UART_ID, PLATFORM_UART_FLOW_CTS | PLATFORM_UART_FLOW_RTS);
	else
		platform_s_uart_set_flow_control( GSM_UART_ID, PLATFORM_UART_FLOW_NONE);
}

/* Setup IO ports. Called in platform_setup() from platform.c */
void gsm_setup_io()
{
	/* Power pin */
	platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
	platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_SET);
	/* DTR pin */
	platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
	platform_pio_op(DTR_PORT, DTR_PIN, PLATFORM_IO_PIN_CLEAR);
	/* Status pin */
	platform_pio_op(STATUS_PORT, STATUS_PIN, PLATFORM_IO_PIN_DIR_INPUT);
	/* Enable_voltage */
	platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_DIR_OUTPUT);
	platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);

	// Serial port
	gsm_set_serial_speed(115200);
	gsm_set_flow(0);
}

static void set_mode_to_9600()
{
	D_ENTER();
	gsm_set_flow(0);
	gsm_set_serial_speed(9600);
	gsm.flags |= HW_FLOW_ENABLED; /* Just to fool gsm_uart_write to send full speed */
	gsm_uart_write("AT" GSM_CMD_LINE_END);
	gsm_uart_write("AT" GSM_CMD_LINE_END);
	gsm.flags &= ~HW_FLOW_ENABLED; /* Remove fooling */
	if (AT_OK == gsm_cmd("AT")) {
		gsm_cmd("AT+CPIN?");    /* Check PIN, Functionality and Network status */
		gsm_cmd("AT+CFUN?");    /* Responses of these will feed the state machine */
		gsm_cmd("AT+COPS?");
		gsm_cmd("AT+SAPBR=2,1"); /* Query GPRS status */
		gsm_cmd("ATE0");
		/* We should now know modem's real status */
		/* Assume that gps is ready, there is no good way to check it */
		gsm.flags |= GPS_READY;
	}
	D_EXIT();
}

static void set_hw_flow()
{
	unsigned int timeout=5000;
	D_ENTER();
	gsm_uart_write("AT" GSM_CMD_LINE_END); // Try to make the autobauding wake up
	gsm_uart_write("AT" GSM_CMD_LINE_END); // Try to make the autobauding wake up
	// TODO: Check the actual boot message (IIII\xff\xff\xff\xff) for baudrate validation. Figure out the missing RDY later.
	while(gsm.state < STATE_BOOTING) {
		delay_ms(1);
		if (!(timeout--)) {
			_DEBUG("%s","No boot messages from uart. assume autobauding\n");
			set_mode_to_9600();
			break;
		}
	}
	if (AT_OK == gsm_cmd("AT+IFC=2,2")) {
		gsm_set_flow(1);
		gsm.flags |= HW_FLOW_ENABLED;
		gsm_cmd("ATE0"); 		/* Disable ECHO */
		gsm_cmd("AT+CSCLK=0");      /* Do not allow module to sleep */
	}
	_DEBUG("%s","HW flow enabled\n");
	D_EXIT();
}

void gsm_enable_voltage()
{
	D_ENTER();
	platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_CLEAR);
	delay_ms(100);		/* Give 100ms for voltage to settle */
	D_EXIT();
}

void gsm_disable_voltage()
{
	platform_pio_op(ENABLE_PORT, ENABLE_PIN, PLATFORM_IO_PIN_SET);
}

void gsm_toggle_power_pin()
{
	platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_CLEAR);
	delay_ms(2000);
	platform_pio_op(POWER_PORT, POWER_PIN, PLATFORM_IO_PIN_SET);
}

/* Send *really* slow. Add inter character delays */
static void slow_send(const char *line)
{
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

void gsm_uart_write_fmt(const char *fmt, ...)
{
	char cmd[256];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( cmd, 256, fmt, ap );
	va_end( ap );
	gsm_uart_write(cmd);
}

/* Find message matching current line */
static Message *lookup_urc_message(const char *line)
{
	int n;
	for(n=0; urc_messages[n].msg; n++) {
		if (0 == slre_match(0, urc_messages[n].msg, line, strlen(line))) {
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
	if (gsm_is_raw_mode())
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
	/* Skip empty lines */
	if (0 == i)
		return;
	if (0 == strcmp(GSM_CMD_LINE_END, buf))
		return;

	_DEBUG("recv: %s\n", buf);

	m = lookup_urc_message(buf);
	if (m) {
		if (m->next_state) {
			gsm.state = m->next_state;
		}
		if (m->func) {
			m->func(buf);
		}
	}
}

/* Enable GSM module */
void gsm_set_power_state(enum Power_mode mode)
{
	int status_pin = platform_pio_op(STATUS_PORT, STATUS_PIN, PLATFORM_IO_PIN_GET);

	switch(mode) {
	case POWER_ON:
		if (0 == status_pin) {
			gsm_enable_voltage();
			gsm_toggle_power_pin();
			set_hw_flow();
		} else {                    /* Modem already on. Possibly warm reset */
			if (gsm.state == STATE_OFF) {
				gsm_cmd("AT+CPIN?");    /* Check PIN, Functionality and Network status */
				gsm_cmd("AT+CFUN?");    /* Responses of these will feed the state machine */
				gsm_cmd("AT+COPS?");
				gsm_cmd("AT+SAPBR=2,1"); /* Query GPRS status */
				set_hw_flow();
				gsm_cmd("ATE0");
				/* We should now know modem's real status */
				/* Assume that gps is ready, there is no good way to check it */
				gsm.flags |= GPS_READY;
			}
		}
		break;
	case POWER_OFF:
		if (1 == status_pin) {
			gsm_toggle_power_pin();
			gsm.state = STATE_OFF;
			gsm.flags = 0;
		}
		break;
	case CUT_OFF:
		gsm_disable_voltage();
		gsm.state = STATE_OFF;
		gsm.flags = 0;
		break;
	}
}

void gsm_reset_modem()
{
	gsm_set_power_state(POWER_OFF);
	delay_ms(2000);
	gsm_set_power_state(POWER_ON);
}


/* Check if GPS flag is set */
int gsm_is_gps_ready()
{
	if((gsm.flags&GPS_READY) != 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int gsm_gprs_enable()
{
	int rc;
	/* Wait for network */
	while(gsm.state < STATE_READY)
		delay_ms(TIMEOUT_MS);

	/* Check if already enabled, response is parsed in URC handler */
	rc = gsm_cmd("AT+SAPBR=2,1");
	if (rc)
		return rc;

	if (gsm.flags&GPRS_READY)
		return 0;

	rc = gsm_cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
	if (rc)
		return rc;
	rc = gsm_cmd_fmt("AT+SAPBR=3,1,\"APN\",\"%s\"", gsm.ap_name);
	if (rc)
		return rc;
	rc = gsm_cmd("AT+SAPBR=1,1");

	if (AT_OK == rc)
		gsm.flags |= GPRS_READY;
	return rc;
}

int gsm_gprs_disable()
{
	gsm_cmd("AT+SAPBR=2,1");
	if (!(gsm.flags&GPRS_READY))
		return 0; //Was not enabled

	return gsm_cmd("AT+SAPBR=0,1"); //Close Bearer
}

/* -------------------- L U A   I N T E R F A C E --------------------*/

/* COMMON FUNCTIONS */

static int gsm_lua_set_power_state(lua_State *L)
{
	enum Power_mode next = luaL_checkinteger(L, -1);
	gsm_set_power_state(next);
	return 0;
}

static int gsm_power_on(lua_State *L)
{
	gsm_set_power_state(POWER_ON);
	return 0;
}

static int gsm_power_off(lua_State *L)
{
	gsm_set_power_state(POWER_OFF);
	return 0;
}

static int gsm_reset_modemL(lua_State *L)
{
	gsm_reset_modem();
	return 0;
}

static int gsm_send_cmd(lua_State *L)
{
	const char *cmd;
	int ret;

	cmd = luaL_checklstring(L, -1, NULL);
	if (!cmd)
		return 0;

	ret = gsm_cmd(cmd);
	lua_pushinteger(L, ret);

	if (ret != AT_OK) {
		printf("GSM: Cmd failed (%s) returned %d\n", cmd, ret);
	}
	return 1;
}

static int gsm_is_pin_required(lua_State *L)
{
	/* Wait for modem to boot */
	while( gsm.state < STATE_ASK_PIN )
		delay_ms(1);		/* Allow uC to sleep */

	lua_pushboolean(L, STATE_ASK_PIN == gsm.state);
	return 1;
}

static int gsm_send_pin(lua_State *L)
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

static int gsm_set_apn(lua_State *L)
{
	const char *apn = luaL_checkstring(L, 1);
	strncpy(gsm.ap_name, apn, sizeof(gsm.ap_name));
	return 0;
}

static int gsm_lua_wait(lua_State *L)
{
	int ret,timeout=TIMEOUT_MS;
	char line[256];
	const char *text;
	text = luaL_checklstring(L, 1, NULL);
	if (2 <= lua_gettop(L)) {
		timeout = luaL_checkinteger(L, 2);
	}
	ret = gsm_wait_cpy(text, timeout, line, sizeof(line));
	lua_pushinteger(L, ret);
	lua_pushstring(L, line);
	return 2;
}

static int gsm_state(lua_State *L)
{
	lua_pushinteger(L, gsm.state);
	return 1;
}


static int gsm_is_ready(lua_State *L)
{
	lua_pushboolean(L, gsm.state == STATE_READY);
	return 1;
}

static int gsm_flag_is_set(lua_State *L)
{
	int flag = luaL_checkinteger(L, -1);
	lua_pushboolean(L, (gsm.flags&flag) != 0);
	return 1;
}

static int gsm_get_caller(lua_State *L)
{
	char line[128];
	char number[64];
	gsm_set_raw_mode();
	gsm_uart_write("AT+CLCC" GSM_CMD_LINE_END);
	while(1) {
		gsm_read_line(line, 128);
		if(1 == sscanf(line, "+CLCC: %*d,%*d,4,0,%*d,\"%s", number)) {
			*strchr(number,'"') = 0;
			lua_pushstring(L, number);
			gsm_disable_raw_mode();
			return 1;
		}
		if(0 == strcmp(line, "OK\r\n")) /* End of responses */
			break;
	}
	gsm_disable_raw_mode();
	return 0;
}


/* SMS FUNCTIONS */

static const char ctrlZ[] = {26, 0};
static int gsm_send_sms(lua_State *L)
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
	gsm_wait(">", TIMEOUT_MS);	 /* Send SMS cmd and wait for '>' to appear */
	gsm_uart_write(text);
	ret = gsm_cmd(ctrlZ);		/* CTRL-Z ends message. Wait for 'OK' */
	lua_pushinteger(L, ret);
	return 1;
}

static int gsm_read_sms(lua_State *L)
{
	char tmp[256], msg[161], number[100];
	const char *err;
	int r,t;
	int index = luaL_checkinteger(L, 1);
	r = gsm_cmd("AT+CMGF=1");         /* Set to text mode */
	switch(r)
	{
		case AT_OK:
		break;
		case AT_TIMEOUT:
			return luaL_error( L, "Timeout when setting SMS mode" );
			break;
		default:
		case AT_FAIL:
		case AT_ERROR:
			return luaL_error( L, "Unknown error when setting SMS mode" );
			break;
	}
	gsm_set_raw_mode();
	snprintf(tmp, 161, "AT+CMGR=%d" GSM_CMD_LINE_END, index);
	gsm_uart_write(tmp);
	r = gsm_wait_cpy("\\+CMGR:", TIMEOUT_MS, tmp, sizeof(tmp));
	switch(r)
	{
		case AT_OK:
		break;
		case AT_TIMEOUT:
			gsm_disable_raw_mode();
			return luaL_error( L, "Timeout when reading SMS content" );
			break;
		default:
		case AT_FAIL:
		case AT_ERROR:
			gsm_disable_raw_mode();
			return luaL_error( L, "Unknown error when reading SMS content" );
			break;
	}
	/* Example: +CMGR: "REC READ","+358403445818","","13/05/13,18:00:15+12" */
	// TODO: We probably should parse the message time (as string) and pass it to the Lua table as well
	err = slre_match(0, "^\\+CMGR:\\s\"[^\"]+\",\"([^\"]+)", tmp, strlen(tmp),
	                 SLRE_STRING, sizeof(number), number);
	if (NULL != err) {
		printf("GSM: Error parsing \"%s\": %s\n", tmp, err);
		gsm_disable_raw_mode();
		return 0;
	}
	msg[0] = 0;
	tmp[0] = 0;
	t = systick_get_raw();
	t+=TIMEOUT_MS*systick_get_hz()/1000;
	do {
		gsm_read_line(tmp, sizeof(tmp));
		// TODO: Actually we should stop at empty line followed by OK, the SMS might contain an empty line as well...
		if (   0 == strcmp(GSM_CMD_LINE_END, tmp) /* Stop at empty line */
			/* && 0 != strlen(msg) /* but only if message is not empty */
			) 
			break;
		strcat(msg, tmp);
		if (systick_get_raw() > t) {
			return luaL_error( L, "Timeout when waiting for end-of-message" );
			break;
		}
	} while(1);
	r = gsm_wait("OK", TIMEOUT_MS);
	// TODO: Strip the final CRLR from the message ??
	gsm_disable_raw_mode();
	switch(r)
	{
		case AT_OK:
		break;
		case AT_TIMEOUT:
			return luaL_error( L, "Timeout when waiting for OK" );
			break;
		default:
		case AT_FAIL:
		case AT_ERROR:
			return luaL_error( L, "Unknown error when waiting for OK" );
			break;
	}

	/* Response table */
	lua_newtable(L);
	lua_pushstring(L, "from");
	lua_pushstring(L, number);
	lua_settable(L, -3);
	lua_pushstring(L, "text");
	lua_pushstring(L, msg);
	lua_settable(L, -3);
	return 1;
}

static int gsm_delete_sms(lua_State *L)
{
	char cmd[20];
	int ret;
	int index = luaL_checkinteger(L, 1);
	snprintf(cmd,64,"AT+CMGD=%d", index);
	ret = gsm_cmd(cmd);
	lua_pushinteger(L, ret);
	return 1;
}


/* HTTP FUNCTIONS */

typedef enum method_t { GET, POST } method_t; /* HTTP methods */

static int gsm_http_init(const char *url)
{
	int ret;

	if (AT_OK != gsm_gprs_enable())
		return -1;

	ret = gsm_cmd("AT+HTTPINIT");
	if (ret != AT_OK)
		return -1;
	ret = gsm_cmd("AT+HTTPPARA=\"CID\",\"1\"");
	if (ret != AT_OK)
		return -1;
	ret = gsm_cmd_fmt("AT+HTTPPARA=\"URL\",\"%s\"", url);
	if (ret != AT_OK)
		return -1;
#if defined( ELUA_BOARD_RUUVIC1 )
	ret = gsm_cmd("AT+HTTPPARA=\"UA\",\"RuuviTracker/SIM968\"");
#else
	ret = gsm_cmd("AT+HTTPPARA=\"UA\",\"RuuviTracker/SIM908\"");
#endif
	if (ret != AT_OK)
		return -1;
	ret = gsm_cmd("AT+HTTPPARA=\"REDIR\",\"1\"");
	if (ret != AT_OK)
		return -1;
	ret = gsm_cmd("AT+HTTPPARA=\"TIMEOUT\",\"30\"");
	if (ret != AT_OK)
		return -1;
	return 0;
}

static void gsm_http_clean()
{
	if (AT_OK != gsm_cmd("AT+HTTPTERM")) {
		gsm_gprs_disable();         /* This should get it to respond */
	}
}

static int gsm_http_send_content_type(const char *content_type)
{
	return gsm_cmd_fmt("AT+HTTPPARA=\"CONTENT\",\"%s\"", content_type);
}

static int gsm_http_send_data(const char *data)
{
	int r;
	r = gsm_cmd_wait_fmt("DOWNLOAD", TIMEOUT_MS, "AT+HTTPDATA=%d,1000", strlen(data));
	if (AT_OK != r) {
		return r;
	}
	gsm_uart_write(data);
	return gsm_cmd(GSM_CMD_LINE_END);
}

static int gsm_http_handle(lua_State *L, method_t method,
                           const char *data, const char *content_type)
{
	int status=0,len,ret;
	char resp[64];
	const char *url = luaL_checkstring(L, 1);
	char *buf=0;

	if (gsm_http_init(url) != 0) {
		printf("GSM: Failed to initialize HTTP\n");
		goto HTTP_END;
	}

	if (content_type) {
		if (gsm_http_send_content_type(content_type) != 0) {
			printf("GSM: Failed to set content type\n");
			goto HTTP_END;
		}
	}

	if (data) {
		if (gsm_http_send_data(data) != 0) {
			printf("GSM: Failed to send http data\n");
			goto HTTP_END;
		}
	}

	if (GET == method)
		ret = gsm_cmd("AT+HTTPACTION=0");
	else
		ret = gsm_cmd("AT+HTTPACTION=1");
	if (ret != AT_OK) {
		printf("GSM: HTTP Action failed\n");
		goto HTTP_END;
	}

	if (gsm_wait_cpy("\\+HTTPACTION", TIMEOUT_HTTP, resp, sizeof(resp)) == AT_TIMEOUT) {
		printf("GSM: HTTP Timeout\n");
		goto HTTP_END;
	}

	if (2 != sscanf(resp, "+HTTPACTION:%*d,%d,%d", &status, &len)) { /* +HTTPACTION:<method>,<result>,<lenght of data> */
		printf("GSM: Failed to parse response\n");
		goto HTTP_END;
	}

	/* Read response, if there is any */
	if (len < 0)
		goto HTTP_END;

	if (NULL == (buf = malloc(len+1))) {
		printf("GSM: Out of memory\n");
		status = 602;               /* HTTP: Out of memory */
		goto HTTP_END;
	}

	//Now read all bytes. block others from reading.
	gsm_set_raw_mode();
	gsm_uart_write("AT+HTTPREAD" GSM_CMD_LINE_END); //Read cmd
	gsm_wait_cpy("\\+HTTPREAD", TIMEOUT_MS, resp, sizeof(resp)); //Wait for header
	/* Rest of bytes are data */
	if (len != gsm_read_raw(buf, len)) {
		printf("GSM: Timeout waiting for HTTP DATA\n");
		free(buf);
		buf = 0;
		goto HTTP_END;
	}
	buf[len]=0;

HTTP_END:
	gsm_disable_raw_mode();
	gsm_http_clean();

	/* Create response structure */
	lua_newtable(L);
	lua_pushstring(L, "status");
	lua_pushinteger(L, status);
	lua_settable(L, -3);
	if (buf) {
		lua_pushstring(L, "content");
		lua_pushstring(L, buf);
		lua_settable(L, -3);
	}
	lua_pushstring(L, "is_success" );
	lua_pushboolean(L, ((200 <= status) && (status < 300))); /* If HTTP response was 2xx */
	lua_settable(L, -3);
	return 1;
}

static int gsm_http_get(lua_State *L)
{
	return gsm_http_handle(L, GET, NULL, NULL);
}

static int gsm_http_post(lua_State *L)
{
	const char *data = luaL_checkstring(L, 2); /* 1 is url, it is handled in http_handle function */
	const char *content_type = luaL_checkstring(L, 3);
	return gsm_http_handle(L, POST, data, content_type);
}


/* TCP FUNCTIONS */

typedef struct socket {
	int con;
	enum {DISCONNECTED, CONNECTED} state;
	struct rbuff *buf;
} socket;

/* Allocate 8 socket (SIM908 max connections) */
#define SIM_MAX_SOCKETS 8
socket sockets[SIM_MAX_SOCKETS];

/* Initialize GSM metatable */
static void gsm_tcp_initialize_meta(lua_State *L)
{
	int index;
	if(0 == luaL_newmetatable(L, "socket_meta")) /* Create our metatable */
		return;                                    /* Already created. */
	index = lua_gettop(L);
	lua_getglobal(L, "gsm");      /* Get gsm.socket to top */
	lua_pushstring(L, "socket");
	lua_gettable(L, -2);               /* Get gsm.socket to top of stack */
	lua_pushvalue(L, -1);              /* Copy it */
	lua_setfield(L, index, "__index"); /* socket_meta.__index = gsm.socket, gsm.socket popped */
	lua_pushstring(L, "close");         /* Get gsm.socket.close */
	lua_gettable(L, -2);
	lua_setfield(L, index, "__gc");    /* socket.meta.__gc = gsm.socket.close */
}

static int gsm_tcp_enable()
{
	int rc = 0;
	while(gsm.state < STATE_READY)
		delay_ms(100);

	if (gsm.flags&TCP_ENABLED)
		return 0;

	rc = gsm_cmd("AT+CIPMUX=1");  /* Multi IP */
	if (rc)
		return rc;
	rc = gsm_cmd("AT+CIPSPRT=1"); /* Report '>' */
	if (rc)
		return rc;
	rc = gsm_cmd("AT+CGATT?");
	if (rc)
		return rc;
	rc = gsm_cmd_fmt("AT+CSTT=\"%s\"", gsm.ap_name);
	if (rc)
		return rc;
	rc = gsm_cmd("AT+CIICR");
	if (rc)
		return rc;
	rc = gsm_cmd_wait("AT+CIFSR", "\\d+.\\d+.\\d+.\\d", TIMEOUT_MS);
	if (!rc)
		gsm.flags |= TCP_ENABLED;
	return rc;
}

static void gsm_tcp_end()
{
	gsm_cmd_wait("AT+CIPSHUT", "SHUT OK", TIMEOUT_MS);
}

enum MODE { TCP, UDP };
static int gsm_tcp_connect(lua_State *L)
{
	int i,rc;
	int mode = TCP;
	socket *s = 0;

	const char *addr = luaL_checkstring(L,1);
	const int port = luaL_checkinteger(L,2);
	if (3 == lua_gettop(L))
		mode = luaL_checkinteger(L, 3);

	gsm_tcp_initialize_meta(L);

	if (AT_OK != gsm_tcp_enable()) {
		printf("GSM: Cannot enable GPRS PDP\n");
		gsm_tcp_end();
		return 0;
	}

	/* Find free socket */
	for(i=0; i<SIM_MAX_SOCKETS; i++) {
		if (sockets[i].state == DISCONNECTED) {
			s = &sockets[i];
			s->con = i;
			break;
		}
	}
	if (NULL == s) {
		printf("GSM: No more free sockets\n");
		return 0;
	}

	/* Connect */
	rc = gsm_cmd_fmt("AT+CIPSTART=%d,\"%s\",\"%s\",%d",i,(TCP==mode)?"TCP":"UDP", addr, port);
	rc = gsm_wait("CONNECT OK", 2*TIMEOUT_HTTP);
	if (AT_OK != rc) {
		printf("GSM: Connection failed\n");
		return 0;
	}

	/* Push pointer to socket as a userdata, this allow us to use metatable (lightuserdata have not metatable) */
	*((socket **)lua_newuserdata(L, sizeof(socket *))) = s;
	luaL_getmetatable(L, "socket_meta");
	lua_setmetatable(L, -2);

	/* Prepare socket */
	s->buf = rbuff_new(100); //More memory will be allocated on receive function
	s->state = CONNECTED;
	printf("Connected\n");
	return 1; // lua_newuserdata pushed alredy to stack
}

static int gsm_tcp_clean(int socket)
{
	sockets[socket].state = DISCONNECTED;
	/* Empty receive buffers */
	rbuff_delete(sockets[socket].buf);
	return 0;
}

static int gsm_tcp_read(lua_State *L)
{
	int size,i;
	char *p;
	socket *s = *(socket **)luaL_checkudata(L, 1, "socket_meta");

	if (!rbuff_is_empty(s->buf)) {
		if (lua_gettop(L)>=2) {     /* Size given */
			size=luaL_checkint(L,2);
			p = malloc(size);
			if (NULL == p) {
				printf("GSM: Out of memory\n");
				return 0;
			}
			i=0;
			while(size) {
				if(rbuff_is_empty(s->buf)) {
					delay_ms(100);
					continue;
				}
				p[i++] = rbuff_pop(s->buf);
				size--;
			}
			lua_pushlstring(L, p, i);
			free(p);
		} else {                    /* No size parameter, read all */
			size = rbuff_len(s->buf);
			p = rbuff_get_raw(s->buf);
			lua_pushlstring(L, p, size);
		}
		return 1;
	}
	return 0;
}

static void socket_receive(char *line)
{
	int n,size,con;
	char c;
	socket *s;

	if(2 == sscanf(line, "+RECEIVE,%d,%d", &con, &size)) {
		gsm_set_raw_mode();
		s = &sockets[con];

		if ((s->buf->size - rbuff_len(s->buf)) > size) { /* Need more space */
			if (!rbuff_grow(s->buf, size + (s->buf->size - rbuff_len(s->buf)))) {
				printf("GSM: Failed to receive socket. Not enough memory\n");
				goto RECV_END;
			}
		}
		for (n=0; n<size; n++) {
			if (0 == gsm_read_raw(&c, 1)) {
				printf("GSM: Socket read failed\n");
				goto RECV_END;
			}
			rbuff_push(s->buf, c);
		}
	}
RECV_END:
	gsm_disable_raw_mode();
}

static int gsm_tcp_write(lua_State *L)
{
	socket *s = *(socket **)luaL_checkudata(L, 1, "socket_meta");
	size_t size;
	const char *str = luaL_checklstring(L, 2, &size);
	char cmd[64];

	if(DISCONNECTED == s->state)
		return luaL_error(L, "Socket disconnected");
	gsm_set_raw_mode();
	snprintf(cmd, 64, "AT+CIPSEND=%d,%d" GSM_CMD_LINE_END, s->con, size);
	gsm_uart_write(cmd);
	if (AT_OK != gsm_wait(">", TIMEOUT_MS) ) {
		printf("Failed to write socket\n");
		return 0;
	}
	gsm_uart_write(str);
	gsm_disable_raw_mode();
	if (AT_TIMEOUT == gsm_wait("SEND OK", TIMEOUT_HTTP)) {
		printf("GSM:Timeout sending socket data\n");
		return 0;
	}
	lua_pushinteger(L, size);
	return 1;
}

static int gsm_tcp_close(lua_State *L)
{
	socket *s = *(socket **)luaL_checkudata(L, 1, "socket_meta");
	if (s->state != DISCONNECTED) {
		gsm_cmd_wait_fmt("CLOSE OK", TIMEOUT_MS, "AT+CIPCLOSE=0,%d", s->con);
		s->state = DISCONNECTED;
	}
	gsm_tcp_clean(s->con);
	return 0;
}

/* Callback from URC */
void socket_closed(char *line)
{
	int s;
	if (1 == sscanf(line,"%d", &s)) {
		sockets[s].state = DISCONNECTED;
	}
}

/******** Export Lua GSM library *********/

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

/* Functions, Map name to func.*/
#define F(name,func) { LSTRKEY(#name), LFUNCVAL(func) }

const LUA_REG_TYPE socket_map[] = {
	F(read, gsm_tcp_read),
	F(write, gsm_tcp_write),
	F(connect, gsm_tcp_connect),
	F(close, gsm_tcp_close),
};

const LUA_REG_TYPE gsm_map[] = {
	F(set_power_state, gsm_lua_set_power_state),
	F(reset, gsm_reset_modemL),
	F(is_ready, gsm_is_ready),
	F(flag_is_set, gsm_flag_is_set),
	F(state, gsm_state),
	F(is_pin_required, gsm_is_pin_required),
	F(send_pin, gsm_send_pin),
	F(send_sms, gsm_send_sms),
	F(read_sms, gsm_read_sms),
	F(delete_sms, gsm_delete_sms),
	F(cmd, gsm_send_cmd),
	F(wait, gsm_lua_wait),
	F(power_on, gsm_power_on),
	F(power_off, gsm_power_off),
	F(get_caller, gsm_get_caller),
	F(set_apn, gsm_set_apn),

	/* CONSTANTS */
#define MAP(a) { LSTRKEY(#a), LNUMVAL(a) }
	MAP( POWER_ON ),
	MAP( POWER_OFF ),
	MAP( CUT_OFF ),
	MAP( STATE_OFF),
	MAP( STATE_BOOTING ),
	MAP( STATE_ASK_PIN ),
	MAP( STATE_WAIT_NETWORK ),
	MAP( STATE_READY ),
	MAP( STATE_ERROR ),
	MAP( GPS_READY ),
	MAP( SIM_INSERTED ),
	MAP( GPRS_READY ),
	MAP( INCOMING_CALL ),
	MAP( CALL ),
	{ LSTRKEY("OK"), LNUMVAL(AT_OK) },
	{ LSTRKEY("FAIL"), LNUMVAL(AT_FAIL) },
	{ LSTRKEY("ERROR"), LNUMVAL(AT_ERROR) },
	{ LSTRKEY("TIMEOUT"), LNUMVAL(AT_TIMEOUT) },

	/* SOCKET MAP */
	{ LSTRKEY("socket"), LROVAL(socket_map) },
	{ LNILKEY, LNILVAL }
};

/* Add HTTP related functions to another table */
const LUA_REG_TYPE http_map[] = {
	F(get, gsm_http_get),
	F(post, gsm_http_post),
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
