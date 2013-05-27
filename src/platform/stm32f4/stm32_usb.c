#include "stm32f4xx.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
   
__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;

/* See USB Configuration fields from http://www.beyondlogic.org/usbnutshell/usb5.shtml */
/* Define Attribute positions in Configuration Descriptor */
#define BMATTRIBUTES 7
#define BMAXPOWER    8
/* Attributes */
#define BMATTR_BUS_POWERED 0x80
#define BMAXPOWER_500mA    0xFA

void usb_init()
{
  uint16_t len;
  uint8_t *desc;

  /* Modify STM32 Library's pre-definded descriptor for RuuviTracker */
  desc = USBD_CDC_cb.GetConfigDescriptor(0, &len);
  desc[BMATTRIBUTES] = BMATTR_BUS_POWERED; /* Define Ruuvi as a Bus powered device */
  desc[BMAXPOWER] = BMAXPOWER_500mA;       /* Ask for 500mA power. */

  USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_HS 
            USB_OTG_HS_CORE_ID,
#else            
            USB_OTG_FS_CORE_ID,
#endif  
            &USR_desc, 
            &USBD_CDC_cb, 
            &USR_cb);


}
