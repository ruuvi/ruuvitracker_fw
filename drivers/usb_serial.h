#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "ch.h"
#include "hal.h"

extern SerialUSBDriver SDU;

void usb_serial_init(void);

#endif /* USB_SERIAL_H */
