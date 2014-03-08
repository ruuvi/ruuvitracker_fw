/*
 *  Simcom 908/968 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "slre.h"
#include "gsm.h"
#include "ch.h"
#include "hal.h"
#include "power.h"
#include "debug.h"


/* ============ DEFINE GPIO PINS HERE =========*/
#define STATUS_PIN  GPIOC_GSM_STATUS
#define STATUS_PORT GPIOC
#define DTR_PIN     GPIOC_GSM_DTR
#define DTR_PORT    GPIOC
#define POWER_PIN   GPIOC_GSM_PWRKEY
#define POWER_PORT  GPIOC
#define ENABLE_PIN  GPIOC_ENABLE_GSM_VBAT
#define ENABLE_PORT GPIOC


#define BUFF_SIZE	64

#define TIMEOUT_MS   5000        /* Default timeout 5s */
#define TIMEOUT_HTTP 35000      /* Http timeout, 35s */

/* Modem state */
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
    BinarySemaphore waiting_reply;
    BinarySemaphore serial_sem;
    volatile int raw_mode;
    enum Reply reply;
    int flags;
    enum CFUN cfun;
    int last_sms_index;
    char ap_name[64];
};
static struct gsm_modem gsm = {		/* Initial status */
    .power_mode=POWER_OFF,
    .state = STATE_OFF,
};


/* Handler functions for AT replies*/
static void handle_ok(char *line);
static void handle_fail(char *line);
static void handle_error(char *line);
static void parse_cfun(char *line);
static void gpsready(char *line);
static void sim_inserted(char *line);
static void no_sim(char *line);
static void incoming_call(char *line);
static void call_ended(char *line);
static void parse_network(char *line);
static void parse_sapbr(char *line);
static void parse_sms_in(char *line);
//static void socket_receive(char *line);
static void pdp_off(char *line);
//static void socket_closed(char *line);

/* Other prototypes */
static void gsm_enable_hw_flow(void);
static void gsm_set_serial_flow_control(int enabled);

/* URC message */
typedef struct Message Message;
struct Message {
    char *msg;			    /* Message string */
    enum State next_state;	/* Automatic state transition */
    void (*func)(char *line);/* Function to call on message */
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
//	{ "\\+RECEIVE",      .func = socket_receive },
//	{ "\\d, CLOSED",    .func = socket_closed },
    { NULL } /* Table must end with NULL */
};

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
 * Called periodically from gsm_thread. May be stopped by reseting IOQueue from
 * Serial port.
 */
static void gsm_state_parser(void)
{
    Message *m;
    static char buf[BUFF_SIZE+1];
    int i=0;
    msg_t c;

    chBSemWait(&gsm.serial_sem);
    D_ENTER();
    while('\n' != (c = sdGet(&SD3))) {
        if (Q_RESET == c) { /* Someone requested serial port by resetting it */
            _DEBUG("%s", "Port reset. releasing\r\n");
            return; /* Leave port locked */
        }
        if (-1 == c)
            break;
        if ('\r' == c)
            continue;
        buf[i++] = (char)c;
        if(i==BUFF_SIZE)
            break;
    }
    chBSemSignal(&gsm.serial_sem);

    buf[i] = 0;
    /* Skip empty lines */
    if (0 == i)
        return;
    if (0 == strcmp(GSM_CMD_LINE_END, buf))
        return;

    _DEBUG("recv: %s\r\n", buf);

    /* Find state transition or callback */
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

static void pdp_off(char *line)
{
    (void)line;
    gsm.flags &= ~TCP_ENABLED;
}

static void incoming_call(char *line)
{
    (void)line;
    gsm.flags |= INCOMING_CALL;
}

static void call_ended(char *line)
{
    (void)line;
    gsm.flags &= ~CALL;
    gsm.flags &= ~INCOMING_CALL;
}

static void parse_sms_in(char *line)
{
    if (1 == sscanf(line, "+CMTI: \"SM\",%d", &gsm.last_sms_index)) {
        //TODO: implement this
    }
}

static void parse_network(char *line)
{
    char network[64];
    /* Example: +COPS: 0,0,"Saunalahti" */
    if (1 == sscanf(line, "+COPS: 0,0,\"%s", network)) {
        *strchr(network,'"') = 0;
        _DEBUG("GSM: Registered to network %s\n", network);
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
            _DEBUG("%s","GPRS not active\n");
            gsm.flags &= ~GPRS_READY;
        }
    } else if (0 == strncmp(line, sapbr_deact, strlen(sapbr_deact))) {
        gsm.flags &= ~GPRS_READY;
    }
}

static void sim_inserted(char *line)
{
    (void)line;
    gsm.flags |= SIM_INSERTED;
}

static void no_sim(char *line)
{
    (void)line;
    gsm.flags &= ~SIM_INSERTED;
}

static void gpsready(char *line)
{
    (void)line;
    gsm.flags |= GPS_READY;
}

static void parse_cfun(char *line)
{
    if (strchr(line,'0')) {
        gsm.cfun = CFUN_0;
    } else if (strchr(line,'1')) {
        gsm.cfun = CFUN_1;
    } else {
        _DEBUG("GSM: Unknown CFUN state: %s\n", line);
    }
}

static void handle_ok(char *line)
{
    (void)line;
    gsm.reply = AT_OK;
    chBSemSignal(&gsm.waiting_reply);
}

static void handle_fail(char *line)
{
    (void)line;
    gsm.reply = AT_FAIL;
    chBSemSignal(&gsm.waiting_reply);
    _DEBUG("%s","GSM: Fail\n");
}

static void handle_error(char *line)
{
    (void)line;
    gsm.reply = AT_ERROR;
    chBSemSignal(&gsm.waiting_reply);
    _DEBUG("GSM ERROR: %s\r\n", line);
}

void gsm_toggle_power_pin(void)
{
    palClearPad(POWER_PORT, POWER_PIN);
    chThdSleepMilliseconds(2000);
    palSetPad(POWER_PORT, POWER_PIN);
}

void gsm_uart_write(const char *str)
{
    while(*str)
        sdPut(&SD3, *str++);
}

/**
 * Send AT command to modem.
 * Waits for modem to reply with 'OK', 'FAIL' or 'ERROR'
 * return AT_OK, AT_FAIL or AT_ERROR
 */
int gsm_cmd(const char *cmd)
{
    int retry;

    _DEBUG("%s\r\n", cmd);

    /* Reset semaphore so we are sure that next time it is signalled
     * it is reply to this command */
    chBSemReset(&gsm.waiting_reply, TRUE);

    for(retry=0; retry<3; retry++) {
        gsm_uart_write(cmd);
        gsm_uart_write(GSM_CMD_LINE_END);
        if(RDY_OK != chBSemWaitTimeout(&gsm.waiting_reply, TIMEOUT_MS))
            gsm.reply = AT_TIMEOUT;
        if (gsm.reply != AT_TIMEOUT)
            break;
    }

    if (gsm.reply != AT_OK)
        _DEBUG("'%s' failed (%d)\r\n", cmd, gsm.reply);

    if (retry == 3) {             /* Modem not responding */
        _DEBUG("%s", "Modem not responding!\r\n");
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
    return gsm_cmd(cmd);
}

static int is_raw_mode = 0;
int gsm_request_serial_port(void)
{
    D_ENTER();
    if (is_raw_mode)
        return 1;
    /* Request serial port by disabling it from handler-thread */
    chSysLock();
    is_raw_mode = 1;
    if (!chBSemGetStateI(&gsm.serial_sem)) {
        chBSemWaitS(&gsm.serial_sem);
    }
    chIQResetI(&(&SD3)->iqueue);
    chSysUnlock();
    chThdSleepMilliseconds(10);
    D_EXIT();
    return 0;
}

void gsm_release_serial_port(void)
{
    D_ENTER();
    is_raw_mode = 0;
    chBSemSignal(&gsm.serial_sem);
    D_EXIT();
}


/* Wait for specific pattern */
int gsm_wait_cpy(const char *pattern, int timeout, char *line, size_t buf_size)
{
    D_ENTER();
    char buf[256];
    int ret;
    unsigned int i;
    msg_t c;
    int was_locked;

    _DEBUG("wait=%s\r\n", pattern);
    was_locked = gsm_request_serial_port();

    i = 0;
    ret = AT_OK;
    while(1) {
        c = sdGetTimeout(&SD3, timeout);
        /* Nothing received */
        if (Q_TIMEOUT == c) {
            ret = AT_TIMEOUT;
            goto WAIT_END;
        }
        buf[i++] = (char)c;
        buf[i] = 0;
        _DEBUG("buf=%s, pattern=%s\r\n", buf, pattern);
        if (0 == strcmp("ERROR", buf)) {
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


    if (line) {
        strcpy(line, buf);
        i = strlen(line);
        while (1) {
            if ( i == (buf_size-1) )
                break;
            c = sdGetTimeout(&SD3, timeout);
            if(Q_TIMEOUT == c) {
                break;
            } else if ('\n' == c) {
                break;
            }
            line[i++] = (char)c;
            line[i] = 0;
        }
    }
WAIT_END:
    if (!was_locked)
        gsm_release_serial_port();
    D_EXIT();
    return ret;
}

int gsm_wait(const char *pattern, int timeout)
{
    return gsm_wait_cpy(pattern, timeout, NULL, 0);
}

int gsm_cmd_wait(const char *cmd, const char *response, int timeout)
{
    int was_locked;
    int r;
    _DEBUG("CMD: %s\r\n", cmd);
    was_locked = gsm_request_serial_port();
    gsm_uart_write(cmd);
    gsm_uart_write(GSM_CMD_LINE_END);
    r = gsm_wait(response, timeout);
    if (!was_locked)
        gsm_release_serial_port();
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

int gsm_read_raw(char *buf, int max_len)
{
    msg_t r;
    int was_locked;
    D_ENTER();
    was_locked = gsm_request_serial_port();
    r = chnReadTimeout((BaseChannel*)&SD3, (void*)buf, max_len, TIMEOUT_MS);
    _DEBUG("read %d bytes\r\n", (int)r);
    if (!was_locked)
        gsm_release_serial_port();
    D_EXIT();
    return (int)r;
}

/* =========== SIM968 Helpers ================= */
static void set_mode_to_fixed_baud(void)
{
    D_ENTER();
    gsm_set_serial_flow_control(0);
    gsm_uart_write("AT" GSM_CMD_LINE_END);
    gsm_uart_write("AT" GSM_CMD_LINE_END);
    if (AT_OK == gsm_cmd("AT")) {
        gsm_cmd("AT+IPR=115200"); /* Switch to fixed baud rate */
        gsm_toggle_power_pin();   /* Reset modem */
        chThdSleepMilliseconds(2000);
        gsm_toggle_power_pin();
    } else {
        _DEBUG("%s", "Failed to synchronize autobauding mode\r\n");
    }
    D_EXIT();
}

static void gsm_enable_hw_flow()
{
    unsigned int timeout=5;
    D_ENTER();
    while(gsm.state < STATE_BOOTING) {
        chThdSleepMilliseconds(1000);
        if (!(timeout--)) {
            _DEBUG("%s","No boot messages from uart. assume autobauding\r\n");
            set_mode_to_fixed_baud();
            break;
        }
    }
    if (AT_OK == gsm_cmd("AT+IFC=2,2")) {
        gsm_set_serial_flow_control(1);
        gsm.flags |= HW_FLOW_ENABLED;
        gsm_cmd("ATE0"); 		/* Disable ECHO */
        gsm_cmd("AT+CSCLK=0");      /* Do not allow module to sleep */
    }
    _DEBUG("%s","HW flow enabled\r\n");
    D_EXIT();
}

/* Enable GSM module */
void gsm_set_power_state(enum Power_mode mode)
{
    int status_pin = palReadPad(STATUS_PORT, STATUS_PIN);

    switch(mode) {
    case POWER_ON:
        if (0 == status_pin) {
            gsm_toggle_power_pin();
            gsm_enable_hw_flow();
        } else {                    /* Modem already on. Possibly warm reset */
            if (gsm.state == STATE_OFF) {   /* Responses of these will feed the state machine */
                gsm_cmd("AT+CPIN?");        /* Check PIN */
                gsm_cmd("AT+CFUN?");        /* Functionality mode */
                gsm_cmd("AT+COPS?");        /* Registered to network */
                gsm_cmd("AT+SAPBR=2,1"); /* Query GPRS status */
                gsm_enable_hw_flow();
                /* We should now know modem's real status */
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
        _DEBUG("%s","CUT_OFF mode not yet handled\n");
        break;
    }
}

void gsm_reset_modem()
{
    gsm_set_power_state(POWER_OFF);
    chThdSleepMilliseconds(2000);
    gsm_set_power_state(POWER_ON);
}


int gsm_gprs_enable()
{
    int rc;
    /* Wait for network */
    while(gsm.state < STATE_READY)
        chThdSleepMilliseconds(TIMEOUT_MS);

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

void gsm_set_apn(const char *apn)
{
    strncpy(gsm.ap_name, apn, 64);
}



/* ============ PLATFORM SPECIFIC ==================== */

static void gsm_set_serial_flow_control(int enabled)
{
    if (enabled) {
        USART3->CR3 |= USART_CR3_RTSE;
        USART3->CR3 |= USART_CR3_CTSE;
    } else {
        USART3->CR3 &= ~USART_CR3_RTSE;
        USART3->CR3 &= ~USART_CR3_RTSE;
    }
}

static WORKING_AREA(waGSM, 4096);
static Thread *worker = NULL;
__attribute__((noreturn))
static void gsm_thread(void *arg)
{
    (void)arg;
    while(!chThdShouldTerminate()) {
        gsm_state_parser();
    }
    chThdExit(0);
    for(;;); /* Suppress warning */
}

/* ============ GENERAL ========================== */

void gsm_start(void)
{
    chBSemInit(&gsm.waiting_reply, TRUE);
    chBSemInit(&gsm.serial_sem, FALSE);
    sdStart(&SD3, NULL);
    power_request(GSM);
    worker = chThdCreateStatic(waGSM, sizeof(waGSM), NORMALPRIO, (tfunc_t)gsm_thread, NULL);
    gsm_set_power_state(POWER_ON);
}
