/*
 *  Simcom 908/968 GPS Driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski
 * @author: Seppo Takalo - For chibios
*/

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Tomi Hautakoski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "gps.h"
#include "slre.h"
#include "power.h"
#include "chprintf.h"
#include "debug.h"
#include <time.h>

#define GPS_BUFF_SIZE	128
#define GPRMC (1<<0)
#define GPGSA (1<<1)
#define GPGGA (1<<2)

EVENTSOURCE_DECL(gps_fix_updated);


/* Prototypes */
static void gps_read_lines(void);
static void gps_parse_line(const char *line);
static char calculate_gps_checksum(const char *data);
static int verify_gps_checksum(const char *data);
static int parse_gpgga(const char *line);
static int parse_gpgsa(const char *line);
static int parse_gprmc(const char *line);
static void parse_nmea_time_str(char *str, gps_datetime *dt);
static void parse_nmea_date_str(char *str, gps_datetime *dt);
static double nmeadeg2degree(double val);

/* GPS device state */
struct gps_device {
    enum GPS_state state;
    int serial_port_validated;
    int msgs_received;
};
static struct gps_device gps = {					/* Initial status */
    .state = STATE_OFF,
    .serial_port_validated = FALSE,	/* For production tests, MCU<->GPS serial line */
};

/* Current location data */
static struct gps_data_t gps_data = {
    .fix_type     = GPS_FIX_TYPE_NONE,  // Set initial values
    .n_satellites = 0,
    .lat          = 0.0,
    .lon          = 0.0,
    .speed        = 0.0,
    .heading      = 0.0,
    .altitude     = 0.0,
    .pdop         = 0.0,
    .hdop         = 0.0,
    .vdop         = 0.0,
};

/* Semaphore to protect data */
static BinarySemaphore gps_sem;
#define LOCK chBSemWait(&gps_sem);
#define UNLOCK chBSemSignal(&gps_sem);


/* ============= platform specific =====================*/

static void gps_power_on(void)
{
    power_request(LDO3);
    power_request(LDO2);
    power_request(GPS_VBACKUP);
}

static void gps_power_off(void)
{
    power_release(LDO3);
    power_release(LDO2);
    // Intentionally leave backup power on
    // TODO: Make a separate function (or pass an argument) to turn everything off
}


/* ===================Internal functions begin=============================== */


/**
 * Receives lines from serial interfaces and parses them.
 */
static char buf[GPS_BUFF_SIZE];
static void gps_read_lines(void)
{
    int i = 0;
    msg_t c;
    systime_t timeout = CH_FREQUENCY+1; /* Block for one secods waiting for first data */

    while(Q_TIMEOUT != (c = sdGetTimeout(&SD2, timeout))) {
        if (Q_RESET == c)
            return;
        if ('\r' == c) /* skip */
            continue;
        if ('\n' == c) { /* End of line */
            buf[i] = 0; /* Mark the end */
            gps_parse_line(buf);
            if (gps.msgs_received & (GPRMC|GPGGA|GPGSA)) /* All required messages */
                timeout = 10; /* Read rest of lines but do not block anymore */
            i = 0; /* Start from beginning */
            continue;
        }
        buf[i++] = (char)c;
        if (i == GPS_BUFF_SIZE)
            return; // Overflow
    }
}

void gps_uart_write(const char *str)
{
    while(*str)
        sdPut(&SD2, *str++);
}

int gps_cmd(const char *cmd)
{
    char cmd_wchecksum[256];
    char checksum = 0;
    if (strstr(cmd, "*") == NULL)
    {
        // No checksum, add it (we suppose no starting $ either)
        sprintf(cmd_wchecksum, "$%s*", cmd);
        checksum = calculate_gps_checksum(cmd_wchecksum);
        sprintf(cmd_wchecksum, "$%s*%2x", cmd, checksum);
        gps_uart_write(cmd_wchecksum);
        _DEBUG("Sent '%s' to GPS\r\n", cmd_wchecksum);
    }
    else
    {
        gps_uart_write(cmd);
        _DEBUG("Sent '%s' to GPS\r\n", cmd);
    }
    gps_uart_write(GPS_CMD_LINE_END);
    // TODO: parse the replies
    return 0;
}

int gps_cmd_fmt(const char *fmt, ...)
{
    char cmd[256];
    va_list ap;
    va_start( ap, fmt );
    vsnprintf( cmd, 256, fmt, ap );
    va_end( ap );
    return gps_cmd(cmd);
}

int gps_set_update_interval(int ms)
{
    return gps_cmd_fmt("PMTK300,%d,0,0,0,0", ms);
}

int gps_set_standby(bool state)
{
    return gps_cmd_fmt("PMTK161,%d", state);
}

/**
 * Parse received line.
 */
static void gps_parse_line(const char *line)
{
    if(strstr(line, "IIII")) {
        return;	// GPS module outputs a 'IIII' as the first string, skip it
    }
    if(verify_gps_checksum(line)) {
        gps.serial_port_validated = TRUE;
        if(strstr(line, "RMC")) {         // Required minimum data
            parse_gprmc(line);
            gps.msgs_received |= GPRMC;
        } else if(strstr(line, "GGA")) {  // Global Positioning System Fix Data
            parse_gpgga(line);
            gps.msgs_received |= GPGGA;
        } else if(strstr(line, "GSA")) {  // GPS DOP and active satellites
            parse_gpgsa(line);
            gps.msgs_received |= GPGSA;
        } else if(strstr(line, "GSV")) {  // GNSS satellite in view
            // TODO: parse
        } else if(strstr(line, "VTG")) {  // Course over ground and ground speed
            // TODO: parse
        }
        else
        {
            _DEBUG("Unhandled response: %s\r\n", line);
        }
    }
    else
    {
        _DEBUG("GPS Checksum failed: %s\r\n", line);
    }
}

static char calculate_gps_checksum(const char *data)
{
    char checksum = 0;
    /* Loop through data, XORing each character to the next */
    data = strstr(data, "$");
    data++;
    while(*data != '*') {
        checksum ^= *data;
        data++;
    }
    return checksum;
}

static int verify_gps_checksum(const char *data)
{
    char checksum = 0;
    char received_checksum = 0;
    char *checksum_index;

    if((checksum_index = strstr(data, "*")) == NULL) { // Find the beginning of checksum
        //_DEBUG("GPS: error, cannot find the beginning of checksum from input line '%s'\n", data);
        return FALSE;
    }
    sscanf(checksum_index + 1, "%02hhx", &received_checksum);
    /*	checksum_index++;
    	*(checksum_index + 2) = 0;
    	received_checksum = strtol(checksum_index, NULL, 16);*/

    checksum = calculate_gps_checksum(data);

    if(checksum == received_checksum) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int parse_gpgga(const char *line)
{
    int n_sat = 0;
    double altitude = 0.0;
    const char *error;

    error = slre_match(0, "^\\$GPGGA,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),[^,]*,([^,]*),[^,]*",
                       line, strlen(line),
                       SLRE_INT, sizeof(n_sat), &n_sat,
                       SLRE_FLOAT, sizeof(altitude), &altitude);
    if(error != NULL) {
        //_DEBUG("GPS: Error parsing GPGGA string '%s': %s\n", line, error);
        return -1;
    } else {
        if(gps_data.n_satellites != n_sat) {
            _DEBUG("GPS: Number of satellites in view: %d\r\n", n_sat);
        }
        LOCK;
        gps_data.n_satellites = n_sat;
        gps_data.altitude = altitude;
        UNLOCK;
        return 0;
    }
}

static int parse_gpgsa(const char *line)
{
    int gps_fix_type;
    double pdop, hdop, vdop;
    const char *error;

    error = slre_match(0, "^\\$G[PN]GSA,[^,]*,([^,]*),[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*)",
                       line, strlen(line),
                       SLRE_INT, sizeof(gps_fix_type), &gps_fix_type,
                       SLRE_FLOAT, sizeof(pdop), &pdop,
                       SLRE_FLOAT, sizeof(hdop), &hdop,
                       SLRE_FLOAT, sizeof(vdop), &vdop);

    if(error != NULL) {
        // TODO: add a check for incomplete sentence
        //_DEBUG("GPS: Error parsing GPGSA string '%s': %s\n", line error);
        return -1;
    } else {
        LOCK;
        switch(gps_fix_type) {
        case GPS_FIX_TYPE_NONE:
            if(gps_data.fix_type != GPS_FIX_TYPE_NONE)
                // Send the "fix lost" only once
                chEvtBroadcastFlags(&gps_fix_updated, GPS_FIX_TYPE_NONE);
                _DEBUG("GPS: No GPS fix\r\n");
            gps_data.fix_type = GPS_FIX_TYPE_NONE;
            gps.state = STATE_ON;
            gps_data.lat = 0.0;
            gps_data.lon = 0.0;
            return -1;
            break;
        case GPS_FIX_TYPE_2D:
            if(gps_data.fix_type != GPS_FIX_TYPE_2D)
                _DEBUG("GPS: fix type 2D\r\n");
            // The fix signal is sent from parse_gprmc after we have parsed to location
            gps_data.fix_type = GPS_FIX_TYPE_2D;
            gps.state = STATE_HAS_2D_FIX;
            break;
        case GPS_FIX_TYPE_3D:
            if(gps_data.fix_type != GPS_FIX_TYPE_3D)
                _DEBUG("GPS: fix type 3D\r\n");
            // The fix signal is sent from parse_gprmc after we have parsed to location
            gps_data.fix_type = GPS_FIX_TYPE_3D;
            gps.state = STATE_HAS_3D_FIX;
            break;
        default:
            _DEBUG("GPS: Error, unknown GPS fix type!\r\n");
            return -1;
            break;
        }
        //postpone this until GPRMC: gps_data.fix_type = gps_fix_type;
        gps_data.pdop = pdop;
        gps_data.hdop = hdop;
        gps_data.vdop = vdop;
        UNLOCK;
        /*
        _DEBUG("GPS: pdop: %f\n", gps_data.pdop);
        _DEBUG("GPS: hdop: %f\n", gps_data.hdop);
        _DEBUG("GPS: vdop: %f\n", gps_data.vdop);
        */
        return 0;
    }
}

static int parse_gprmc(const char *line)
{
    char time[15], status[2], ns[2], ew[2], date[7];
    double lat, lon, speed_ms, heading;

    const char *error;
    error = slre_match(0, "^\\$G[PN]RMC,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),[^,]*,[^,]*,[^,]*",
                       line, strlen(line),
                       SLRE_STRING, sizeof(time), time,
                       SLRE_STRING, sizeof(status), status,
                       SLRE_FLOAT, sizeof(lat), &lat,
                       SLRE_STRING, sizeof(ns), ns,
                       SLRE_FLOAT, sizeof(lon), &lon,
                       SLRE_STRING, sizeof(ew), ew,
                       SLRE_FLOAT, sizeof(speed_ms), &speed_ms,
                       SLRE_FLOAT, sizeof(heading), &heading,
                       SLRE_STRING, sizeof(date), date);

    if(error != NULL) {
        _DEBUG("GPS: Error parsing GPRMC string '%s': %s\n", line, error);
        return -1;
    } else {
        if(strcmp(status, "A") != 0) {
            return -1;
        }
        LOCK;
        parse_nmea_time_str(time, &gps_data.dt);
        parse_nmea_date_str(date, &gps_data.dt);
        gps_data.lat = nmeadeg2degree(lat);
        if(strstr(ns, "S")) {
            gps_data.lat = -1 * gps_data.lat;
        }
        gps_data.lon = nmeadeg2degree(lon);
        if(strstr(ew, "W")) {
            gps_data.lon = -1 * gps_data.lon;
        }
        /* Assume that we have received all required information
         * So let user API to know that we have fix */
        switch(gps.state) {
        case STATE_HAS_2D_FIX:
            gps_data.fix_type = GPS_FIX_TYPE_2D;
            chEvtBroadcastFlags(&gps_fix_updated, GPS_FIX_TYPE_2D);
            break;
        case STATE_HAS_3D_FIX:
            gps_data.fix_type = GPS_FIX_TYPE_3D;
            chEvtBroadcastFlags(&gps_fix_updated, GPS_FIX_TYPE_3D);
            break;
        default:
            gps_data.fix_type = GPS_FIX_TYPE_NONE;
            /**
             * We send this in parse_gpgsa (and only once)
            chEvtBroadcastFlags(&gps_fix_updated, GPS_FIX_TYPE_NONE);
             */
        }
        UNLOCK;
        return 0;
    }
}

// Time string should be the following format: '093222.000'
static void parse_nmea_time_str(char *str, gps_datetime *dt)
{
    char tmp[4];

    memcpy(tmp, str, 2);
    tmp[2] = 0;
    dt->hh = strtol(tmp, NULL, 10);
    memcpy(tmp, str+2, 2);
    dt->mm = strtol(tmp, NULL, 10);
    memcpy(tmp, str+4, 2);
    dt->sec = strtol(tmp, NULL, 10);
    memcpy(tmp, str+7, 3);
    dt->msec = strtol(tmp, NULL, 10);
    tmp[3] = 0;
}

// Date string should be the following format: '140413'
static void parse_nmea_date_str(char *str, gps_datetime *dt)
{
    char tmp[6];

    memcpy(tmp, str, 2);
    tmp[2] = 0;
    dt->day = strtol(tmp, NULL, 10);
    memcpy(tmp, str+2, 2);
    dt->month = strtol(tmp, NULL, 10);
    memcpy(tmp, str+4, 2);
    dt->year = 2000 + strtol(tmp, NULL, 10); // Its past Y2k now
}

static double nmeadeg2degree(double val)
{
    double deg = ((int)(val / 100));
    val = deg + (val - deg * 100) / 60;
    return val;
}

/*
 * Worker thread
 */
static WORKING_AREA(waGPS, 4096);
static Thread *worker = NULL;
__attribute__((noreturn))
static void gps_thread(void *arg)
{
    (void)arg;
    sdStart(&SD2, NULL);
    gps_power_on();
    memset(&gps_data, 0, sizeof(gps_data));
    gps_data.fix_type  = GPS_FIX_TYPE_NONE;
    while(!chThdShouldTerminate()) {
        gps.msgs_received = 0; /* Clear flags */
        gps_read_lines();
        gps_data.last_update = chTimeNow();
    }
    sdStop(&SD2);
    gps_power_off();
    chThdExit(0);
    for(;;); /* Suppress warning */
}

void gps_start()
{
    if (NULL == worker) {
        chBSemInit(&gps_sem, FALSE);
        worker = chThdCreateStatic(waGPS, sizeof(waGPS), NORMALPRIO, (tfunc_t)gps_thread, NULL);
    }
}

void gps_stop()
{
    if (worker) {
        chThdTerminate(worker);
        chThdWait(worker); /* May block if worker is blocked */
        worker = NULL;
    }
}

fix_t gps_has_fix(void)
{
    fix_t r;
    LOCK;
    r = gps_data.fix_type;
    UNLOCK;
    return r;
}

struct gps_data_t gps_get_data_nonblock(void) {
    struct gps_data_t d;
    LOCK;
    d = gps_data;
    UNLOCK;
    return d;
}

struct gps_data_t gps_get_data(void) {
    /* TODO: implement this */
    return gps_get_data_nonblock();
}

void gps_datetime2tm(struct tm *timp, gps_datetime *gpstime)
{
    timp->tm_sec = gpstime->sec;
    timp->tm_min = gpstime->mm;
    timp->tm_hour = gpstime->hh;
    timp->tm_mday = gpstime->day;
    timp->tm_mon = gpstime->month-1;
    timp->tm_year = gpstime->year-1900;
}
