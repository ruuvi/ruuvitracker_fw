/*
 *  Simcom 908 GPS driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski
 */

#ifndef _GPS_H_
#define _GPS_H_

/* GPS Fix type */
typedef enum {
	GPS_FIX_TYPE_NONE = 1,
	GPS_FIX_TYPE_2D = 2,
	GPS_FIX_TYPE_3D = 3
} fix_t;

/* GPS Internal State */
enum GPS_state {
        STATE_UNKNOWN = 0,
        STATE_OFF = 1,
        STATE_ON = 2,
        STATE_HAS_2D_FIX = 3,
        STATE_HAS_3D_FIX = 4,
        STATE_ERROR = 5
};

/* Date and time data */
typedef struct _gps_datetime {
	int hh;
	int mm;
	int sec;
	int msec;
	int day;
	int month;
	int year;
} gps_datetime;

/* Location data */
struct gps_data_t {
	fix_t   fix_type;         /* Satellite fix type */
	int     n_satellites;     /* Number of satellites in view */
	double	lat;              /* Latitude */
	double	lon;
	double	speed;
	double	heading;
	double	altitude;
	double	pdop;
	double	hdop;
	double	vdop;
	gps_datetime dt;
	systime_t last_update;
};


/******** API ***************/

/**
 * Start GPS module and processing
 */
void gps_start(void);

/**
 * Stop GPS processing and shuts down the module
 */
void gps_stop(void);

/**
 * Reports GPS fix type.
 * \return Fix type.
 */
fix_t gps_has_fix(void);

/**
 * Request GPS data.
 * This function queries last known values parsed from GPS messages.
 * \return gps_data_t structure.
 */
struct gps_data_t gps_get_data(void);

#endif

