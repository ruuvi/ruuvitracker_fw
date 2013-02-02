/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.c
  * @author  MCD Application Team, modified by Seppo Takalo
  * @date    2013-02-01
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_vcp.h"
#include "usb_conf.h"

#include <string.h> /* for strlen */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
LINE_CODING linecoding =
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };


/* These are external variables imported from CDC core to be used for IN 
   transfer management. Data sent from microcontroller to host. */
extern uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */
extern uint32_t APP_Rx_ptr_out;	 /* CDC driver's internal pointer, used to detect overflow */

#define FROM_HOST_MAX 128
__IO uint8_t From_Host_Buffer[FROM_HOST_MAX]; /* data received from host gets stored in this buffer */
__IO uint32_t From_Host_Idx_Write, From_Host_Idx_Read;


/* Private function prototypes -----------------------------------------------*/
static uint16_t VCP_Init     (void);
static uint16_t VCP_DeInit   (void);
static uint16_t VCP_Ctrl     (uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataTx   (uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataRx   (uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef VCP_fops = 
{
  VCP_Init,
  VCP_DeInit,
  VCP_Ctrl,
  VCP_DataTx,
  VCP_DataRx
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  VCP_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16_t VCP_Init(void)
{
	return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_DeInit(void)
{
	return USBD_OK;
}


/**
  * @brief  VCP_Ctrl
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{ 
	(void) Len;
	switch (Cmd) {
	case SEND_ENCAPSULATED_COMMAND:
		/* Not  needed for this driver */
		break;

	case GET_ENCAPSULATED_RESPONSE:
		/* Not  needed for this driver */
		break;

	case SET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case GET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case CLEAR_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case SET_LINE_CODING:
		linecoding.bitrate = (uint32_t) (Buf[0] | (Buf[1] << 8) | (Buf[2] << 16)
				| (Buf[3] << 24));
		linecoding.format = Buf[4];
		linecoding.paritytype = Buf[5];
		linecoding.datatype = Buf[6];
		/* Set the new configuration */
		/* VCP_COMConfig(OTHER_CONFIG); */
		break;

	case GET_LINE_CODING:
		Buf[0] = (uint8_t) (linecoding.bitrate);
		Buf[1] = (uint8_t) (linecoding.bitrate >> 8);
		Buf[2] = (uint8_t) (linecoding.bitrate >> 16);
		Buf[3] = (uint8_t) (linecoding.bitrate >> 24);
		Buf[4] = linecoding.format;
		Buf[5] = linecoding.paritytype;
		Buf[6] = linecoding.datatype;
		break;

	case SET_CONTROL_LINE_STATE:
		/* Not  needed for this driver */
		break;

	case SEND_BREAK:
		/* Not  needed for this driver */
		break;

	default:
		break;
	}

	return USBD_OK;
}


extern int USBD_USR_isavailable();
/**
  * @brief  VCP_DataTx
  *         CDC received data to be send over USB IN endpoint are managed in 
  *         this function.
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
uint16_t VCP_DataTx(uint8_t* Buf, uint32_t Len)
{
	uint32_t i;

	for (i = 0; i < Len; i++) {
	  if (VCP_SendChar(*(Buf + i)) == USBD_FAIL)
	    return USBD_FAIL;
	}

	return USBD_OK;
}

/**
  * @brief  VCP_SendRaw
  *         Alias for VCP_DataTx
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */
uint32_t VCP_SendRaw(uint8_t* Buf, uint32_t Len)
{
	return VCP_DataTx(Buf, Len);
}

/**
  * @brief  VCP_SendString
  *         Sends a character-string over USB IN (calls VCP_DataTx)
  * @param  s: String to be sent
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */
uint32_t VCP_SendString(char* s)
{
	return VCP_SendRaw((uint8_t*)s, strlen(s));
}

/**
  * @brief  VCP_SendChar
  *         Sends a character over USB IN (like VCP_DataTx but just for one character)
  * @param  c: character to be sent
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */
uint32_t VCP_SendChar(char c)
{
	/* To avoid buffer overflow */
	if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
	  APP_Rx_ptr_in = 0; // Roll back
	}
	if (APP_Rx_ptr_in < APP_Rx_ptr_out) { // We have rolled back.
	  if (APP_Rx_ptr_in == (APP_Rx_ptr_out-1)) { //No more room
	    return USBD_FAIL; //Buffer full
	  }
	}
	APP_Rx_Buffer[APP_Rx_ptr_in++] = c;

	return USBD_OK;
}

/**
  * @brief  VCP_HasReceived
  *         Returns the number of bytes that have been received from
  *         the host through USB OUT
  * @param  none
  * @retval fill-state of the buffer: 0=no data, !=0 data in buffer
  */
uint32_t VCP_HasReceived(void)
{
	uint32_t used;

	if (From_Host_Idx_Write >= From_Host_Idx_Read) {
		used = From_Host_Idx_Write - From_Host_Idx_Read;
	} else {
		used = FROM_HOST_MAX - (From_Host_Idx_Read - From_Host_Idx_Write);
	}

	return used;
}

/**
  * @brief  VCP_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will block any OUT packet reception on USB endpoint
  *         until exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result
  *         in receiving more data while previous ones are still not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK
  */
extern void usart_received(int id, u8 c);
uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len)
{
	// n.b. this function is called from the ISR
	uint32_t i;

	for (i = 0; i < Len; i++) {
	  /* Pipe received data to UART0 buffer (TODO: do not hardcode this)*/
	  usart_received(0, Buf[i]);
	}

	return USBD_OK;
}

/**
  * @brief  VCP_GetReceived
  *         Read data from the FIFO-buffer of received data from the host.
  * @param  none
  * @retval received byte
  */
uint8_t VCP_GetReceived(void)
{
	uint8_t retval;

	if (VCP_HasReceived() > 0) {
		retval = From_Host_Buffer[From_Host_Idx_Read];
		From_Host_Idx_Read++;
		if (From_Host_Idx_Read >= FROM_HOST_MAX) {
			From_Host_Idx_Read -= FROM_HOST_MAX;
		}
	} else {
		retval = 0;
	}
	return retval;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
