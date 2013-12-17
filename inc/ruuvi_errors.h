#ifndef __RUUVI_ERRORS_H__
#define __RUUVI_ERRORS_H__

typedef enum rt_error
{
    RT_ERR_OK = 0,
    RT_ERR_TIMEOUT = -1, // obviously timeout
    RT_ERR_ERROR = -2 // General error
    // Add more error codes as we get there
    
} rt_error;



#endif