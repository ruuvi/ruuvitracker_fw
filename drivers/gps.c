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

/* Prototypes */
void gps_line_received(void);
int calculate_gps_checksum(const char *data);
int parse_gpzda(const char *line);
int parse_gpgga(const char *line);
int parse_gpgsa(const char *line);
int parse_gprmc(const char *line);
void parse_nmea_time_str(char *str, gps_datetime *dt);
void parse_nmea_date_str(char *str, gps_datetime *dt);
double degree2nmeadeg(double val);
double nmeadeg2degree(double val);

/* GPS device state */
struct gps_device {
	enum GPS_state state;
	int serial_port_validated;
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

/*
	// Construct ISO compatible time string, 2012-01-08T20:57:30.123Z
	sprintf(timestr, "%d-%02d-%02dT%02d:%02d:%02d.%03dZ",
	        gps_data.dt.year,
	        gps_data.dt.month,
	        gps_data.dt.day,
	        gps_data.dt.hh,
	        gps_data.dt.mm,
	        gps_data.dt.sec,
	        gps_data.dt.msec);
*/

/* ===================Internal functions begin=============================== */


/**
 * Receives lines from Simcom serial interfaces and parses them.
 */
static char buf[GPS_BUFF_SIZE+1];
static void gps_read_line(void)
{
	int c, i = 0;

	while('\n' != (c = chSequentialStreamGet(&SD2))) {
		if(-1 == c)
			break;
		if('\r' == c) /* skip */
			continue;
		buf[i++] = (char)c;
		if(i == GPS_BUFF_SIZE)
			break;
	}
	buf[i] = 0;

	if(i > 0) {
		if(strstr(buf, "IIII")) {
			return;				// GPS module outputs a 'IIII' as the first string, skip it
		}
		if(calculate_gps_checksum(buf)) {
			gps.serial_port_validated = TRUE;
			if(strstr(buf, "RMC")) {
				parse_gprmc(buf);
			} else if(strstr(buf, "GGA")) {  // Global Positioning System Fix Data
				parse_gpgga(buf);
			} else if(strstr(buf, "GSA")) {  // GPS DOP and active satellites
				parse_gpgsa(buf);
			} else if(strstr(buf, "GSV")) {  // Satellites in view
				//parse_gpgsv(buf); // TODO
			} else if(strstr(buf, "GLL")) {  // Geographic Position, Latitude / Longitude and time.
				//parse_gpgll(buf); // TODO
			} else if(strstr(buf, "VTG")) {  // Track Made Good and Ground Speed
				//parse_gpvtg(buf); // TODO
			} else if(strstr(buf, "ZDA")) {  // Date & Time
				//parse_gpzda(buf); // Not needed ATM, GPRMC has the same data&time data
			} else {
				//printf("GPS: input line '%s' doesn't match any supported GP sentences!\r\n", buf);
			}
		} else {
			//printf("GPS: error, calculated checksum does not match received!\r\n");
		}
	}
}

int calculate_gps_checksum(const char *data)
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

int parse_gpgga(const char *line)
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
		gps_data.n_satellites = n_sat;
		gps_data.altitude = altitude;
		return 0;
	}
}

int parse_gpzda(const char *line)
{
	char time[15];
	int day, month, year;
	const char *error;

	memset(time, 0, sizeof(time));
	error = slre_match(0, "^\\$GPZDA,([^,]*),([^,]*),([^,]*),([^,]*),[^,]*,[^,]*",
	                   line, strlen(line),
	                   SLRE_STRING, sizeof(time), time,
	                   SLRE_INT, sizeof(day), &day,
	                   SLRE_INT, sizeof(month), &month,
	                   SLRE_INT, sizeof(year), &year);
	if(error != NULL) {
		//printf("GPS: Error parsing GPZDA string '%s': %s\r\n", line, error);
		return -1;
	} else {
		parse_nmea_time_str(time, &gps_data.dt);
		gps_data.dt.day = day;
		gps_data.dt.month = month;
		gps_data.dt.year = year;
		/*
		printf("GPS: hh: %d\n", gps_data.dt.hh);
		printf("GPS: mm: %d\n", gps_data.dt.mm);
		printf("GPS: sec: %d\n", gps_data.dt.sec);
		printf("GPS: msec: %d\n", gps_data.dt.msec);
		printf("GPS: day: %d\n", gps_data.dt.day);
		printf("GPS: month: %d\n", gps_data.dt.month);
		printf("GPS: year: %d\n", gps_data.dt.year);
			*/
		return 0;
	}
}

int parse_gpgsa(const char *line)
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
		/*
		printf("GPS: pdop: %f\n", gps_data.pdop);
		printf("GPS: hdop: %f\n", gps_data.hdop);
		printf("GPS: vdop: %f\n", gps_data.vdop);
		*/
		return 0;
	}
}

int parse_gprmc(const char *line)
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

		/*
		printf("GPS: hh: %d\n", gps_data.dt.hh);
		printf("GPS: mm: %d\n", gps_data.dt.mm);
		printf("GPS: sec: %d\n", gps_data.dt.sec);
		printf("GPS: msec: %d\n", gps_data.dt.msec);
		printf("GPS: day: %d\n", gps_data.dt.day);
		printf("GPS: month: %d\n", gps_data.dt.month);
		printf("GPS: year: %d\n", gps_data.dt.year);
		printf("GPS: status: %s\n", status);
		printf("GPS: latitude: %f\n", lat);
		printf("GPS: ns_indicator: %s\n", ns);
		printf("GPS: longitude: %f\n", lon);
		printf("GPS: ew_indicator: %s\n", ew);
		printf("GPS: speed_ms: %f\n", speed_ms);
		printf("GPS: heading: %f\n", heading);
		*/
		return 0;
	}
}

// Time string should be the following format: '093222.000'
void parse_nmea_time_str(char *str, gps_datetime *dt)
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
void parse_nmea_date_str(char *str, gps_datetime *dt)
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

double nmeadeg2degree(double val)
{
	double deg = ((int)(val / 100));
	val = deg + (val - deg * 100) / 60;
	return val;
}

double degree2nmeadeg(double val)
{
	double int_part;
	double fra_part;
	fra_part = modf(val, &int_part);
	val = int_part * 100 + fra_part * 60;
	return val;
}

/*
 * Worker thread
 */
static WORKING_AREA(waGPS, 4096);
static Thread *worker = NULL;
__attribute__((noreturn))
static void gps_thread(void *arg) {
	(void)arg;
	sdStart(&SD2, NULL);
	gps_power_on();
	memset(&gps_data, 0, sizeof(gps_data));
	gps_data.fix_type  = GPS_FIX_TYPE_NONE;
	while(!chThdShouldTerminate()) {
		LOCK;
		gps_read_line();
		gps_data.last_update = chTimeNow();
		UNLOCK;
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

struct gps_data_t gps_get_data(void)
{
	struct gps_data_t d;
	LOCK;
	d = gps_data;
	UNLOCK;
	return d;
}
