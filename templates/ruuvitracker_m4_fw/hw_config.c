/******************************************************************************
* Project        : RuuviTracker                                               *
* File Name      : hw_config.c                                                *
* Author         : Lauri Jämsä / lauri.jamsa@ruuvipenkki.fi / ruuvipenkki.fi  *
* Created        : 01/07/2012                                                 *
* Description    : HW specific source file                                    *
* License        : BSD                                                        *
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "hw_config.h"
#include "platform_config.h"

/* Private functions --------------------------------------------------------*/


/******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None.
* Return         : None.
******************************************************************************/
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure LED1's GPIO port and pin */
	GPIO_InitStructure.GPIO_Pin = LED1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(LED1_PORT, &GPIO_InitStructure);
	GPIO_SetBits(LED1_PORT, LED1);

	/* Configure LED2's GPIO port and pin */
	GPIO_InitStructure.GPIO_Pin = LED2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(LED2_PORT, &GPIO_InitStructure);
	GPIO_SetBits(LED2_PORT, LED2);
}









/******************************************************************************
* Function Name  : RCC_Configuration
* Description    : Configures the different system clocks.
* Input          : None.
* Return         : None.
******************************************************************************/
void RCC_Configuration(void)
{
	/* Enable peripheral clocks ----------------------------*/
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
}










/******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configure interrupts.
* Input          : None.
* Return         : None.
******************************************************************************/
void NVIC_Configuration(void)
{
}








/******************************************************************************
* Function Name  : EXTI_Configuration (external interrupt config)
* Description    : Configure interrupts.
* Input          : None.
* Return         : None.
******************************************************************************/
void EXTI_Configuration(void)
{
}







/******************************************************************************
* Function Name  : USART_Configuration
* Description    : Configure USARTs
* Input          : None.
* Return         : None.
******************************************************************************/

void USART_Configuration(void)
{
	USART_InitTypeDef USART_InitStructure;

	#ifdef RUUVITRACKER_DEBUG_USART
		USART_DeInit(USART1);

		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

		/* Configure USART1 */
		USART_Init(USART1, &USART_InitStructure);

		/* Enable the USART1 */
		USART_Cmd(USART1, ENABLE);

		/* Enable USART1 Receive and Transmit interrupts (if we don't use polling) */
		//USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		//USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	#endif

	#ifdef RUUVITRACKER_GPS_UART
		USART_DeInit(USART2);

		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

		/* Configure USART2 */
		USART_Init(USART2, &USART_InitStructure);

		/* Enable the USART2 */
		USART_Cmd(USART2, ENABLE);

		/* Enable USART2 Receive and Transmit interrupts (if we don't use polling) */
		//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
		//USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	#endif

	#ifdef RUUVITRACKER_GSM_MODULE_UART
		USART_DeInit(USART3);

		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

		/* Configure USART2 */
		USART_Init(USART3, &USART_InitStructure);

		/* Enable the USART2 */
		USART_Cmd(USART3, ENABLE);

		/* Enable USART3 Receive and Transmit interrupts (if we don't use polling) */
		//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
		//USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
	#endif
}



/******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power.
* Input          : None.
* Return         : None.
******************************************************************************/
void Set_System(void)
{
	/* Setup the microcontroller system. Initialize the Embedded Flash
	Interface, initialize the PLL and update the SystemFrequency variable.
	SystemInit() is called from startup file (startup_stm32f10x_xx.s)
	before to branch to application main. To reconfigure the default
	setting of SystemInit() function, refer to system_stm32f10x.c file */
	
	//SystemInit();

	/* Setup SysTick Timer for 10 usec interrupts  */
	//if (SysTick_Config(SystemCoreClock/100000)) while (1);

	RCC_Configuration();

	NVIC_Configuration();

	GPIO_Configuration();

	EXTI_Configuration();

	USART_Configuration();
}




#if 0

/******************************************************************************
* Function Names : SetDelay_10us, SetDelay_100us, SetDelay_1ms ...
* Description    : For delay purposes
* Input          : Delay time
* Return         : None.
******************************************************************************/
void SetDelay_10us(__IO unsigned long int nTime) {
	TimingDelay = nTime;
	while(TimingDelay != 0);
}
void SetDelay_100us(__IO unsigned long int nTime) {
	TimingDelay = nTime * 10; // 10us * 10 = 100us
	while(TimingDelay != 0);
}
void SetDelay_1ms(__IO unsigned long int nTime) {
	TimingDelay = nTime * 100; // 10us * 100 = 1ms
	while(TimingDelay != 0);
}
void TimingDelay_Decrement(void) {
	if (TimingDelay != 0x00) TimingDelay--;
}
#endif
