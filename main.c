/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <time.h>

#include "ch.h"
#include "hal.h"
#include "test.h"

#include "shell.h"
#include "drivers/debug.h"
#include "chprintf.h"
#include "power.h"
#include "drivers/usb_serial.h"
#include "drivers/gps.h"
#include "drivers/gsm.h"
#include "drivers/http.h"
#include "drivers/reset_button.h"
#include "drivers/rtchelpers.h"
#include "drivers/sdcard.h"
#include "drivers/testplatform.h"

/* I2C interface #1 */
// TODO: This should probably be defined in board.h or something. It needs to be globally accessible because in case of I2C timeout we *must* reinit the whole I2C subsystem
static const I2CConfig i2cfg1 = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};
// TODO: I guess this does not have to be global
static i2cflags_t i2c_errors = 0;


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)
#define TEST_WA_SIZE    THD_WA_SIZE(256)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[])
{
    size_t n, size;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: mem\r\n");
        return;
    }
    n = chHeapStatus(NULL, &size);
    chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
    chprintf(chp, "heap fragments   : %u\r\n", n);
    chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[])
{
    static const char *states[] = {THD_STATE_NAMES};
    Thread *tp;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: threads\r\n");
        return;
    }
    chprintf(chp, "    addr    stack prio refs     state time\r\n");
    tp = chRegFirstThread();
    do {
        chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %lu\r\n",
                 (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
                 (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
                 states[tp->p_state], (uint32_t)tp->p_time);
        tp = chRegNextThread(tp);
    } while (tp != NULL);
#ifdef CORTEX_ENABLE_WFI_IDLE
    chprintf(chp, "CRTX_ENABLE_WFI_IDLE=%d\r\n", CORTEX_ENABLE_WFI_IDLE);
#endif
#ifdef ENABLE_WFI_IDLE
    chprintf(chp, "ENBL_WFI_IDLE=%d\r\n", ENABLE_WFI_IDLE);
#endif
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[])
{
    Thread *tp;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: test\r\n");
        return;
    }
    tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriority(),
                             TestThread, chp);
    if (tp == NULL) {
        chprintf(chp, "out of memory\r\n");
        return;
    }
    chThdWait(tp);
}

/*
 * GPS Test command.
 * Enables GPS chip on RuuviTracker and pushed NMEA messages from serial port
 * to usb serial adapter
 */
static void cmd_gps(BaseSequentialStream *chp, int argc, char *argv[])
{
    struct gps_data_t gps;
    char buf[255];
    (void)argc;

    if (0 == strcmp(argv[0], "start")) {
        gps_start();
    } else if (0 == strcmp(argv[0], "stop")) {
        gps_stop();
    } else if (0 == strcmp(argv[0], "cmd")) {
        gps_cmd(argv[1]);
    } else if (0 == strcmp(argv[0], "test")) {
        chprintf(chp, "Enabling GPS module.\r\nPress enter to stop\r\n\r\n");
        gps_start();
        while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
            chprintf(chp, "\x1B[2J-------[GPS Tracking,  press enter to stop]----\r\n");
            if (GPS_FIX_TYPE_NONE != gps_has_fix()) {
                gps = gps_get_data();
                snprintf(buf, sizeof(buf),
                         "lat: %f\r\n"
                         "lon: %f\r\n"
                         "satellites: %d\r\n"
                         "time: %d-%d-%d %d:%d:%d\r\n",
                         (float)gps.lat,(float)gps.lon, gps.n_satellites,
                         gps.dt.year, gps.dt.month, gps.dt.day, gps.dt.hh, gps.dt.mm, gps.dt.sec);
                chprintf(chp, buf);
            } else {
                chprintf(chp, "waiting for fix\r\n");
            }
            chThdSleepMilliseconds(1000);
        }
        gps_stop();
        chprintf(chp, "GPS stopped\r\n");
    } else {
        chprintf(chp, "Unsupported subcommand %s\r\n", argv[0]);
    }
}

/**
 * Incoming SMS notifier
 */
static WORKING_AREA(wa_sms_thd, 2048);

static void sms_thd(void *arg)
{
    (void)arg;
    chRegSetThreadName("sms_thd");
    EventListener smslisten;
    eventmask_t   events;
    int sms_index;
    int stat;
    /*
     * Register the event listener with the event source.  This is the only
     * event this thread will be waiting for, so we choose the lowest eid.
     * However, the eid can be anything from 0 - 31.
     */
    chEvtRegister(&gsm_evt_sms_arrived, &smslisten, 0);
    while (!chThdShouldTerminate())
    {
        //_DEBUG("Waiting for SMS event\r\n");
        /*
         * We can now wait for our event.  Since we will only be waiting for
         * a single event, we should use chEvtWaitOne()
         */
        events = chEvtWaitOneTimeout(EVENT_MASK(0), 100);
        if (events != EVENT_MASK(0))
        {
            // Timed out
            continue;
        }
        _DEBUG("SMS event received\r\n");
        sms_index = chEvtGetAndClearFlags(&smslisten);
        _DEBUG("New SMS in index %d\r\n", sms_index);
        chThdSleepMilliseconds(100);
        stat = gsm_read_sms(sms_index, &gsm_sms_default_container);
        if (stat != AT_OK)
        {
            // The read func will report the errors for now.
            continue;
        }
        _DEBUG("Message from '%s' is: '%s'\r\n", gsm_sms_default_container.number, gsm_sms_default_container.msg);
    }
    chEvtUnregister(&gsm_evt_sms_arrived, &smslisten);
    chThdExit(0);
}


static void cmd_gsm(BaseSequentialStream *chp, int argc, char *argv[])
{
    static Thread *smsworker = NULL;
    int i;
    if (argc < 1) {
        chprintf(chp, "Usage: gsm [apn <apn_name>] | [start] | [cmd <str>]\r\n");
        return;
    }
    if (0 == strcmp(argv[0], "apn")) {
        gsm_set_apn(argv[1]);
    } else if (0 == strcmp(argv[0], "start")) {
        // SMS notifier thread
        smsworker = chThdCreateStatic(wa_sms_thd, sizeof(wa_sms_thd), NORMALPRIO, (tfunc_t)sms_thd, NULL);
        gsm_start();
    } else if (0 == strcmp(argv[0], "stop")) {
        gsm_stop();
        chThdTerminate(smsworker);
        chThdWait(smsworker);
        /**
         * Reminder: static threads cannot be released
        chThdRelease(smsworker);
         */
        smsworker = NULL;
    } else if (0 == strcmp(argv[0], "smsread")) {
        i = gsm_read_sms(atoi(argv[1]), &gsm_sms_default_container);
        if (i != AT_OK)
        {
            // The read func will report the errors for now.
            return;
        }
        chprintf(chp, "Message from '%s' is: '%s'\r\n", gsm_sms_default_container.number, gsm_sms_default_container.msg);
    } else if (0 == strcmp(argv[0], "smsdel")) {
        gsm_delete_sms(atoi(argv[1]));
    } else if (0 == strcmp(argv[0], "smssend")) {
        strncpy(gsm_sms_default_container.number, argv[1], sizeof(gsm_sms_default_container.number)-1);
        gsm_sms_default_container.msg[0] = 0x0;
        gsm_sms_default_container.mr = 0;
        for (i=2; i<argc; i++)
        {
            strcat(gsm_sms_default_container.msg, argv[i]);
            if (i<(argc-1))
            {
                strcat(gsm_sms_default_container.msg, " ");
            }
        }
        gsm_send_sms(&gsm_sms_default_container);
    } else if (0 == strcmp(argv[0], "kill")) {
        gsm_kill();
    } else if (0 == strcmp(argv[0], "cmd")) {
        gsm_cmd(argv[1]);
    } else {
        chprintf(chp, "Unsupported subcommand %s\r\n", argv[0]);
    }
}

static void cmd_http(BaseSequentialStream *chp, int argc, char *argv[])
{
    HTTP_Response *http;
    if (argc < 2) {
        chprintf(chp, "Usage: http [get <url>] | [post <url> <content> <content_type>]\r\n");
        return;
    }
    if (0 == strcmp(argv[0], "get")) {
        http = http_get(argv[1]);
        if (http) {
            chprintf(chp, "OK, Status %d\r\nContent-size: %d\r\nContent:\r\n%s\r\n", http->code, http->content_len, http->content);
            http_free(http);
        } else {
            chprintf(chp, "Failed\r\n");
        }
    } else if (0 == strcmp(argv[0], "post")) {
        chprintf(chp, "POST\r\nurl: %s\r\ntype: %s\r\ndata: %s\r\n", argv[1], argv[2], argv[3]);
        http = http_post(argv[1], argv[2], argv[3]);
        if (http) {
            chprintf(chp, "OK, Status %d\r\nContent-size: %d\r\nContent:\r\n%s\r\n", http->code, http->content_len, http->content);
            http_free(http);
        } else {
            chprintf(chp, "Failed\r\n");
        }
    }
}

/**
 * Test command to read first 6 registers of the accelerometer
 */
static void cmd_accread(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    msg_t status = RDY_OK;
    //uint8_t txbuff[1];
    uint8_t rxbuff[6];
    uint8_t txbuff[1];
    txbuff[0] = 0x0; // register
    uint8_t addr = 0x1d; // 7-bit address
    i2cAcquireBus(&I2CD1);
    i2cStart(&I2CD1, &i2cfg1);
    status = i2cMasterTransmitTimeout(&I2CD1, addr, txbuff, 1, rxbuff, 6, MS2ST(500));
    switch(status)
    {
        case RDY_OK:
        {
            uint8_t i;
            for (i=0; i<6; i++)
            {
                chprintf(chp, "Device 0x%02X reg 0x%02X value 0x%02X\r\n", addr, txbuff[0]+i, rxbuff[i]);
            }
            break;
        }
        case RDY_RESET:
            i2c_errors = i2cGetErrors(&I2CD1);
            chprintf(chp, "error for device at 0x%02X, errors: 0x%02X I2C1->SR1=0x%04X\r\n", addr, i2c_errors, I2C1->SR1);
            // Reset copied from http://forum.chibios.org/phpbb/viewtopic.php?f=2&t=777
            i2cStop(&I2CD1);
            I2C1->CR1 |= I2C_CR1_SWRST;
            i2cStart(&I2CD1, &i2cfg1);
            break;
        case RDY_TIMEOUT:
            chprintf(chp, "TIMEOUT for device at 0x%02X\r\n", addr, i2c_errors);
            // Reset copied from http://forum.chibios.org/phpbb/viewtopic.php?f=2&t=777
            i2cStop(&I2CD1);
            I2C1->CR1 |= I2C_CR1_SWRST;
            i2cStart(&I2CD1, &i2cfg1);
            break;
    }
    i2cStop(&I2CD1);
    i2cReleaseBus(&I2CD1);
}    


static const ShellCommand commands[] = {
    {"mem", cmd_mem},
    {"threads", cmd_threads},
    {"test", cmd_test},
    {"gps", cmd_gps},
    {"gsm", cmd_gsm},
    {"http", cmd_http},
    {"stop", cmd_stop},
    {"standby", cmd_standby},
    {"date", cmd_date},
    {"alarm", cmd_alarm},
    {"wakeup", cmd_wakeup},
    {"acc", cmd_accread},
    {"mount", sdcard_cmd_mount},
    {"ls", sdcard_cmd_ls},
    {"tp_sync", cmd_tp_sync},
    {"tp_set_syncpin", cmd_tp_set_syncpin},
    {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream *)&SDU,
    commands
};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waBlinkerThd, 128);

__attribute__((noreturn))
static void BlinkerThd(void *arg)
{
    (void)arg;
    chRegSetThreadName("blinker");
    systime_t time;
    /** 
     * Remove the noreturn attribute if using this check
    while (!chThdShouldTerminate())
    */
    while (TRUE)
    {
        time = SDU.config->usbp->state == USB_ACTIVE ? 250 : 500;
        palClearPad(GPIOB, GPIOB_LED1);
        chThdSleepMilliseconds(time);
        palSetPad(GPIOB, GPIOB_LED1);
        chThdSleepMilliseconds(time);
    }
    //chThdExit(0);
}



/*
 * Application entry point.
 */
static const EXTConfig extcfg; 
int main(void)
{
    Thread *shelltp = NULL;

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /**
     * initialize the HAL stuff the testplatform needs
     */
    tp_init();

    /*
     * Initializes a serial-over-USB CDC driver.
     */
    usb_serial_init();

    /* Initializes reset button PA0 */
    /**
     * This messes my button wakeup
    button_init();
     */
    extStart(&EXTD1, &extcfg);

    /*
     * Shell manager initialization.
     */
    shellInit();

    /*
     * Creates the blinker thread.
     */
    chThdCreateStatic(waBlinkerThd, sizeof(waBlinkerThd), NORMALPRIO, (tfunc_t)BlinkerThd, NULL);
    palClearPad(GPIOB, GPIOB_LED2);



    /*
     * Normal main() thread activity, in this demo it does nothing except
     * sleeping in a loop and check the USB state.
     */
    while (TRUE) {
        if (!shelltp && (SDU.config->usbp->state == USB_ACTIVE)) {
            palSetPad(GPIOB, GPIOB_LED2);
            shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
        } else if (chThdTerminated(shelltp)) {
            chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
            shelltp = NULL;           /* Triggers spawning of a new shell.        */
            palClearPad(GPIOB, GPIOB_LED2);
        }
        chThdSleepMilliseconds(1000);
    }
}
