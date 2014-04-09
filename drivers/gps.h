/*
 *  Simcom 908 GPS driver for Ruuvitracker.
 *
 * @author: Tomi Hautakoski
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
struct gps_data_t gps_get_data_nonblock(void);

/**
 * Request GPS data.
 * This function waits for next GPS data to arrive.
 * \return gps_data_t structure.
 */
struct gps_data_t gps_get_data(void);

#endif

