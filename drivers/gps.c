/*
 *  Simcom 908/968 GPS Driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski
 * @author: Seppo Takalo - For chibios
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

#define printf(...) while(0){}

#define GPS_BUFF_SIZE	128
#define GPRMC (1<<0)
#define GPGSA (1<<1)
#define GPGGA (1<<2)

/* Prototypes */
static void gps_read_lines(void);
static void gps_parse_line(const char *line);
static int calculate_gps_checksum(const char *data);
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
}

static void gps_power_off(void)
{
    power_release(LDO3);
    power_release(LDO2);
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

/**
 * Parse received line.
 */
static void gps_parse_line(const char *line)
{
    if(strstr(line, "IIII")) {
        return;	// GPS module outputs a 'IIII' as the first string, skip it
    }
    if(calculate_gps_checksum(line)) {
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
        }
    }
}

static int calculate_gps_checksum(const char *data)
{
    char checksum = 0;
    char received_checksum = 0;
    char *checksum_index;

    if((checksum_index = strstr(data, "*")) == NULL) { // Find the beginning of checksum
        //printf("GPS: error, cannot find the beginning of checksum from input line '%s'\n", data);
        return FALSE;
    }
    sscanf(checksum_index + 1, "%02hhx", &received_checksum);
    /*	checksum_index++;
    	*(checksum_index + 2) = 0;
    	received_checksum = strtol(checksum_index, NULL, 16);*/

    /* Loop through data, XORing each character to the next */
    data = strstr(data, "$");
    data++;
    while(*data != '*') {
        checksum ^= *data;
        data++;
    }

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
        //printf("GPS: Error parsing GPGGA string '%s': %s\n", line, error);
        return -1;
    } else {
        if(gps_data.n_satellites != n_sat) {
            printf("GPS: Number of satellites in view: %d\r\n", n_sat);
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
        //printf("GPS: Error parsing GPGSA string '%s': %s\n", line error);
        return -1;
    } else {
        LOCK;
        switch(gps_fix_type) {
        case GPS_FIX_TYPE_NONE:
            if(gps_data.fix_type != GPS_FIX_TYPE_NONE)
                printf("GPS: No GPS fix\r\n");
            gps_data.fix_type = GPS_FIX_TYPE_NONE;
            gps.state = STATE_ON;
            gps_data.lat = 0.0;
            gps_data.lon = 0.0;
            return -1;
            break;
        case GPS_FIX_TYPE_2D:
            if(gps_data.fix_type != GPS_FIX_TYPE_2D)
                printf("GPS: fix type 2D\r\n");
            gps_data.fix_type = GPS_FIX_TYPE_2D;
            gps.state = STATE_HAS_2D_FIX;
            break;
        case GPS_FIX_TYPE_3D:
            if(gps_data.fix_type != GPS_FIX_TYPE_3D)
                printf("GPS: fix type 3D\r\n");
            gps_data.fix_type = GPS_FIX_TYPE_3D;
            gps.state = STATE_HAS_3D_FIX;
            break;
        default:
            printf("GPS: Error, unknown GPS fix type!\r\n");
            return -1;
            break;
        }
        //postpone this until GPRMC: gps_data.fix_type = gps_fix_type;
        gps_data.pdop = pdop;
        gps_data.hdop = hdop;
        gps_data.vdop = vdop;
        UNLOCK;
        /*
        printf("GPS: pdop: %f\n", gps_data.pdop);
        printf("GPS: hdop: %f\n", gps_data.hdop);
        printf("GPS: vdop: %f\n", gps_data.vdop);
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
        printf("GPS: Error parsing GPRMC string '%s': %s\n", line, error);
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
            break;
        case STATE_HAS_3D_FIX:
            gps_data.fix_type = GPS_FIX_TYPE_3D;
            break;
        default:
            gps_data.fix_type = GPS_FIX_TYPE_NONE;
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
