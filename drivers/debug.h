#ifndef _DEBUG_H
#define _DEBUG_H

#include "usb_serial.h"
#include "chprintf.h"

#ifndef NO_DEBUG

/* Assume debug output to be this USB-serial port */
#define printf(...) if(SDU.config->usbp->state == USB_ACTIVE){chprintf((BaseSequentialStream *)&SDU, __VA_ARGS__);}

#define D_ENTER() printf("%s:%s(): enter\r\n", __FILE__, __FUNCTION__)
#define D_EXIT() printf("%s:%s(): exit\r\n", __FILE__, __FUNCTION__)
#define _DEBUG(...) do{ \
	  printf("%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
	  printf(__VA_ARGS__); \
     } while(0)

#else

#define D_ENTER()
#define D_EXIT()
#define _DEBUG(...)
#endif /* NO_DEBUG */

#endif /* _DEBUG_H */
