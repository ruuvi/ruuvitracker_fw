// Generic headers
#include "platform.h"
#include "platform_conf.h"
#include "elua_int.h"
#include "common.h"
#include "delay.h"

// Platform specific includes
#include "stm32f4xx_conf.h"
#include "usbd_cdc_vcp.h"
#include "ringbuff.h"
#include "gsm.h"
#include "gps.h"
#include <delay.h>

//Prototypes
void usart_received(int id, u8 c);

// ****************************************************************************
// UART

// All possible STM32 uarts defs
USART_TypeDef *const stm32_usart[] =          { USART1, USART2, USART3, UART4, UART5, USART6};
const u8 stm32_usart_AF[] =       { GPIO_AF_USART1, GPIO_AF_USART2, GPIO_AF_USART3, GPIO_AF_UART4, GPIO_AF_UART5, GPIO_AF_USART6};
#if defined( ELUA_BOARD_STM32F4ALT )
static GPIO_TypeDef *const usart_gpio_rx_port[] = { GPIOA, GPIOA, GPIOC, GPIOC, GPIOD };
static GPIO_TypeDef *const usart_gpio_tx_port[] = { GPIOA, GPIOA, GPIOC, GPIOC, GPIOD };
static const u16 usart_gpio_rx_pin[] = { GPIO_Pin_10, GPIO_Pin_3, GPIO_Pin_11, GPIO_Pin_11, GPIO_Pin_2 };
static const u8 usart_gpio_rx_pin_source[] = { GPIO_PinSource10, GPIO_PinSource3, GPIO_PinSource11, GPIO_PinSource11, GPIO_PinSource2 };
static const u16 usart_gpio_tx_pin[] = { GPIO_Pin_9, GPIO_Pin_2, GPIO_Pin_10, GPIO_Pin_10, GPIO_Pin_12 };
static const u8 usart_gpio_tx_pin_source[] = { GPIO_PinSource9, GPIO_PinSource2, GPIO_PinSource10, GPIO_PinSource10, GPIO_PinSource12 };
#elif defined( ELUA_BOARD_RUUVIA )
static GPIO_TypeDef *const usart_gpio_rx_port[] = { GPIOA, GPIOA, GPIOB, GPIOC, GPIOD };
static GPIO_TypeDef *const usart_gpio_tx_port[] = { GPIOA, GPIOA, GPIOB, GPIOC, GPIOD };
static const u16 usart_gpio_rx_pin[] = { GPIO_Pin_10, GPIO_Pin_3, GPIO_Pin_11, GPIO_Pin_11, GPIO_Pin_2 };
static const u8 usart_gpio_rx_pin_source[] = { GPIO_PinSource10, GPIO_PinSource3, GPIO_PinSource11, GPIO_PinSource11, GPIO_PinSource2 };
static const u16 usart_gpio_tx_pin[] = { GPIO_Pin_9, GPIO_Pin_2, GPIO_Pin_10, GPIO_Pin_10, GPIO_Pin_12 };
static const u8 usart_gpio_tx_pin_source[] = { GPIO_PinSource9, GPIO_PinSource2, GPIO_PinSource10, GPIO_PinSource10, GPIO_PinSource12 };
static GPIO_TypeDef *const usart_gpio_hwflow_port[] = { GPIOA, GPIOA, GPIOB };
static const u16 usart_gpio_cts_pin[] = { GPIO_Pin_11, GPIO_Pin_0, GPIO_Pin_13 };
static const u8 usart_gpio_cts_pin_source[] = { GPIO_PinSource11, GPIO_PinSource0, GPIO_PinSource13 };
static const u16 usart_gpio_rts_pin[] = { GPIO_Pin_12, GPIO_Pin_1, GPIO_Pin_14 };
static const u8 usart_gpio_rts_pin_source[] = { GPIO_PinSource12, GPIO_PinSource1, GPIO_PinSource14 };
#elif defined( ELUA_BOARD_RUUVIB1 )
static GPIO_TypeDef *const usart_gpio_rx_port[] = { GPIOB, GPIOD, GPIOD, GPIOC, GPIOD };
static GPIO_TypeDef *const usart_gpio_tx_port[] = { GPIOB, GPIOD, GPIOD, GPIOC, GPIOD };
static const u16 usart_gpio_rx_pin[] = { GPIO_Pin_7, GPIO_Pin_5, GPIO_Pin_8, GPIO_Pin_11, GPIO_Pin_2 };
static const u8 usart_gpio_rx_pin_source[] = { GPIO_PinSource7, GPIO_PinSource5, GPIO_PinSource8, GPIO_PinSource11, GPIO_PinSource2 };
static const u16 usart_gpio_tx_pin[] = { GPIO_Pin_6, GPIO_Pin_6, GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_12 };
static const u8 usart_gpio_tx_pin_source[] = { GPIO_PinSource6, GPIO_PinSource6, GPIO_PinSource9, GPIO_PinSource10, GPIO_PinSource12 };
static GPIO_TypeDef *const usart_gpio_hwflow_port[] = { GPIOA, GPIOD, GPIOB };
static const u16 usart_gpio_cts_pin[] = { GPIO_Pin_11, GPIO_Pin_3, GPIO_Pin_13 };
static const u8 usart_gpio_cts_pin_source[] = { GPIO_PinSource11, GPIO_PinSource3, GPIO_PinSource13 };
static const u16 usart_gpio_rts_pin[] = { GPIO_Pin_12, GPIO_Pin_4, GPIO_Pin_14 };
static const u8 usart_gpio_rts_pin_source[] = { GPIO_PinSource12, GPIO_PinSource4, GPIO_PinSource14 };
#else
#error "Define UART pins/ports for this board in uart.c"
#endif

#define BUFF_SIZE	4096

struct rbuff *rbuff[NUM_UART];

static void usart_init(u32 id, USART_InitTypeDef *initVals)
{
	/* Configure USART IO */
	GPIO_InitTypeDef GPIO_InitStructure;

	//Connect pin to USARTx_Tx
	GPIO_PinAFConfig(usart_gpio_tx_port[id], usart_gpio_tx_pin_source[id], stm32_usart_AF[id]);
	//Connect pin to USARTx_Rx
	GPIO_PinAFConfig(usart_gpio_rx_port[id], usart_gpio_rx_pin_source[id], stm32_usart_AF[id]);

	/* Configure USART Tx Pin as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = usart_gpio_tx_pin[id];
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //push pull or open drain
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //pull-up or pull-down
	GPIO_Init(usart_gpio_tx_port[id], &GPIO_InitStructure);

	/* Configure USART Rx Pin as input floating */
	GPIO_InitStructure.GPIO_Pin = usart_gpio_rx_pin[id];
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //pull-up or pull-down
	GPIO_Init(usart_gpio_rx_port[id], &GPIO_InitStructure);

	USART_DeInit(stm32_usart[id]);

	/* Configure USART */
	USART_Init(stm32_usart[id], initVals);

	/* create buffer */
	rbuff[id] = rbuff_new(BUFF_SIZE);

	/* Enable USART */
	USART_Cmd(stm32_usart[id], ENABLE);
	USART_ITConfig( stm32_usart[id], USART_IT_RXNE, ENABLE); // enable the USART1 receive int
}

void uarts_init()
{
	// Enable clocks.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
}

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = baud;

	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	switch( databits ) {
	case 5:
	case 6:
	case 7:
	case 8:
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		break;
	case 9:
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		break;
	default:
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		break;
	}

	switch (stopbits) {
	case PLATFORM_UART_STOPBITS_1:
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		break;
	case PLATFORM_UART_STOPBITS_2:
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
		break;
	default:
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
		break;
	}

	switch (parity) {
	case PLATFORM_UART_PARITY_EVEN:
		USART_InitStructure.USART_Parity = USART_Parity_Even;
		break;
	case PLATFORM_UART_PARITY_ODD:
		USART_InitStructure.USART_Parity = USART_Parity_Odd;
		break;
	default:
		USART_InitStructure.USART_Parity = USART_Parity_No;
		break;
	}

	usart_init(id, &USART_InitStructure);

	return TRUE;
}

void platform_s_uart_send( unsigned id, u8 data )
{
	while(USART_GetFlagStatus(stm32_usart[id], USART_FLAG_TXE) == RESET) {
	}
	USART_SendData(stm32_usart[id], data);
	if (id == 0) {
		VCP_SendChar(data);
	}
}

/*** Receive functions */

int platform_s_uart_recv( unsigned id, timer_data_type timeout )
{
	struct rbuff *q= rbuff[id];
	if( timeout != 0 )  {
		while(rbuff_is_empty(q))
			delay_ms(1);
	}
	if(rbuff_is_empty(q))
		return -1;
	return rbuff_pop(q);
}

/* Push byte to the receiving buffer */
/* TODO: Buffer overrun errors not handled */
void usart_received(int id, u8 c)
{
	rbuff_push(rbuff[id], c);
}

/* Interrupt handler. Called from USART RX interrupt */
void all_usart_irqhandler( int id )
{
	u8 c = USART_ReceiveData(stm32_usart[id]);
	usart_received(id, c);

#ifdef BUILD_GSM
	if ( (id == GSM_UART_ID) && (c == '\n')) {
		elua_add_c_hook(gsm_line_received);
	}
#endif
#ifdef BUILD_GPS
	if ( (id == GPS_UART_ID) && (c == '\n')) {
		elua_add_c_hook(gps_line_received);
	}
#endif
}



int platform_s_uart_set_flow_control( unsigned id, int type )
{
	USART_TypeDef *usart = stm32_usart[ id ];
	int temp = 0;
	GPIO_InitTypeDef GPIO_InitStructure;

	if( id >= 3 ) // on STM32 only USART1 through USART3 have hardware flow control ([TODO] but only on high density devices?)
		return PLATFORM_ERR;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

	if( type == PLATFORM_UART_FLOW_NONE ) {
		usart->CR3 &= ~USART_HardwareFlowControl_RTS_CTS;
		GPIO_InitStructure.GPIO_Pin = usart_gpio_rts_pin[ id ] | usart_gpio_cts_pin[ id ];
		GPIO_Init( usart_gpio_hwflow_port[ id ], &GPIO_InitStructure );
		return PLATFORM_OK;
	}
	if( type & PLATFORM_UART_FLOW_CTS ) {
		temp |= USART_HardwareFlowControl_CTS;
		GPIO_InitStructure.GPIO_Pin = usart_gpio_cts_pin[ id ];
		GPIO_Init( usart_gpio_hwflow_port[ id ], &GPIO_InitStructure );
		GPIO_PinAFConfig(usart_gpio_hwflow_port[id], usart_gpio_cts_pin_source[id], stm32_usart_AF[id]);
	}
	if( type & PLATFORM_UART_FLOW_RTS ) {
		temp |= USART_HardwareFlowControl_RTS;
		GPIO_InitStructure.GPIO_Pin = usart_gpio_rts_pin[ id ];
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_Init( usart_gpio_hwflow_port[ id ], &GPIO_InitStructure );
		GPIO_PinAFConfig(usart_gpio_hwflow_port[id], usart_gpio_rts_pin_source[id], stm32_usart_AF[id]);
	}
	usart->CR3 |= temp;
	return PLATFORM_OK;
}
