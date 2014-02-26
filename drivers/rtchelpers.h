#ifndef RTCHELPERS_H
#define RTCHELPERS_H
#include "ch.h"
#include "hal.h"
#include "chrtclib.h"

/**
 * shell command to set or get alarm0
 *
 * The set argument takes unixtime (seconds since epoch)
 */
void cmd_alarm(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * shell command to set or display the RTC datetime
 *
 * The set argument takes unixtime (seconds since epoch)
 */
void cmd_date(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * Shell command to set the recurring wakeup timer (or get the value)
 *
 * The set argument takes interval as seconds
 */
void cmd_wakeup(BaseSequentialStream *chp, int argc, char *argv[]);

#endif