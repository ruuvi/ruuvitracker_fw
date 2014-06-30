#include "drivers/rtchelpers.h"
#include "chprintf.h"
#include <string.h>
#include <stdlib.h>
#include "chrtclib.h"
#include <time.h>

void cmd_alarm(BaseSequentialStream *chp, int argc, char *argv[])
{
  int i = 0;

  RTCAlarm alarmspec;

  (void)argv;
  if (argc < 1) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    rtcGetAlarm(&RTCD1, 0, &alarmspec);
    chprintf(chp, "%D%s",alarmspec," - alarm in STM internal format\r\n");
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    i = atol(argv[1]);
    alarmspec.tv_datetime = ((i / 10) & 7 << 4) | (i % 10) | RTC_ALRMAR_MSK4 |
                            RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2;
    rtcSetAlarm(&RTCD1, 0, &alarmspec);
    return;
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: alarm get\r\n");
  chprintf(chp, "       alarm set N\r\n");
  chprintf(chp, "where N is alarm time in seconds\r\n");
}

void cmd_date(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argv;
  struct tm timp;
  time_t unix_time;

  if (argc == 0) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    unix_time = rtcGetTimeUnixSec(&RTCD1);

    if (unix_time == -1){
      chprintf(chp, "incorrect time in RTC cell\r\n");
    }
    else{
      chprintf(chp, "%D%s",unix_time," - unix time\r\n");
      rtcGetTimeTm(&RTCD1, &timp);
      chprintf(chp, "%s%s",asctime(&timp)," - formatted time string\r\n");
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    unix_time = atol(argv[1]);
    if (unix_time > 0){
      rtcSetTimeUnixSec(&RTCD1, unix_time);
      return;
    }
    else{
      goto ERROR;
    }
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: date get\r\n");
  chprintf(chp, "       date set N\r\n");
  chprintf(chp, "where N is time in seconds sins Unix epoch\r\n");
  chprintf(chp, "you can get current N value from unix console by the command\r\n");
  chprintf(chp, "%s", "date +\%s\r\n");
  return;
}

void cmd_wakeup(BaseSequentialStream *chp, int argc, char *argv[])
{
  RTCWakeup wakeupspec;
  int i = 0;

  (void)argv;
  if (argc < 1) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    rtcGetPeriodicWakeup_v2(&RTCD1, &wakeupspec);
    chprintf(chp, "%D%s",wakeupspec," - alarm in STM internal format\r\n");
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    i = atoi(argv[1]);
    wakeupspec.wakeup = ((uint32_t)4) << 16; /* select 1 Hz clock source */
    wakeupspec.wakeup |= i-1; /* set counter value to i-1. Period will be i+1 seconds. */
    rtcSetPeriodicWakeup_v2(&RTCD1, &wakeupspec);
    return;
  }

ERROR:
  chprintf(chp, "Usage: wakeup get\r\n");
  chprintf(chp, "       wakeup set N\r\n");
  chprintf(chp, "where N is recurring alarm time in seconds\r\n");
}

