#ifndef __RUUVI_ERRORS_H__
#define __RUUVI_ERRORS_H__
#include "delay.h"

typedef enum rt_error
{
    RT_ERR_OK = 0,
    RT_ERR_TIMEOUT = -1, // obviously timeout
    RT_ERR_ERROR = -2 // General error
    // Add more error codes as we get there
    
} rt_error;

#define RT_DEFAULT_TIMEOUT (150) // ms
#define RT_TIMEOUT_INIT() unsigned int RT_TIMEOUT_STARTED = systick_get_raw();
#define RT_TIMEOUT_CHECK(ms) if ((RT_TIMEOUT_STARTED - systick_get_raw()) > ms) { return RT_ERR_TIMEOUT; }

#endif