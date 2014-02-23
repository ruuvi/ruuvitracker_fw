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
    (void)argv;
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
}

static void cmd_gsm(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc < 1) {
        chprintf(chp, "Usage: gsm [apn <apn_name>] | [start] | [cmd <str>]\r\n");
        return;
    }
    if (0 == strcmp(argv[0], "apn")) {
        gsm_set_apn(argv[1]);
    } else if (0 == strcmp(argv[0], "start")) {
        gsm_start();
    } else if (0 == strcmp(argv[0], "cmd")) {
        gsm_cmd(argv[1]);
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

static const ShellCommand commands[] = {
    {"mem", cmd_mem},
    {"threads", cmd_threads},
    {"test", cmd_test},
    {"gps_test", cmd_gps},
    {"gsm", cmd_gsm},
    {"http", cmd_http},
    {"stop", cmd_stop},
    {"standby", cmd_standby},
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
static WORKING_AREA(waThread1, 128);

__attribute__((noreturn))
static void Thread1(void *arg)
{
    (void)arg;
    chRegSetThreadName("blinker");
    while (TRUE) {
        systime_t time;

        time = SDU.config->usbp->state == USB_ACTIVE ? 250 : 500;
        palClearPad(GPIOB, GPIOB_LED1);
        chThdSleepMilliseconds(time);
        palSetPad(GPIOB, GPIOB_LED1);
        chThdSleepMilliseconds(time);
    }
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
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, (tfunc_t)Thread1, NULL);
    palClearPad(GPIOB, GPIOB_LED2);

    /*
     * Normal main() thread activity, in this demo it does nothing except
     * sleeping in a loop and check the button state.
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
