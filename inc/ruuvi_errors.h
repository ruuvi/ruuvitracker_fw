#ifndef __RUUVI_ERRORS_H__
#define __RUUVI_ERRORS_H__
#include "delay.h"

#ifndef D_EXIT
#define D_EXIT()
#define _DEBUG(fmt, args...)
#endif

typedef enum rt_error
{
    RT_ERR_OK = 0,
    RT_ERR_FAIL = -1, // Failure, not really an error, it might be normal and expected even, just an easier way to pass boolean status to the caller
    RT_ERR_TIMEOUT = -2, // obviously timeout
    RT_ERR_ERROR = -3 // General error
    // Add more error codes as we get there
    
} rt_error;

#define RT_DEFAULT_TIMEOUT (1500) // ms
#define RT_TIMEOUT_INIT() unsigned int RT_TIMEOUT_STARTED = systick_get_raw();
#define RT_TIMEOUT_REINIT() RT_TIMEOUT_STARTED = systick_get_raw();
#define RT_TIMEOUT_CHECK(ms) if ((systick_get_raw() - RT_TIMEOUT_STARTED) > ms) { _DEBUG("%s\n", "timeout!"); D_EXIT(); return RT_ERR_TIMEOUT; }

#endif