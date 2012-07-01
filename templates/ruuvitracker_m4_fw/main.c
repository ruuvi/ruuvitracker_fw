/******************************************************************************
* Project        : RuuviTracker                                               *
* File Name      : main.c                                                     *
* Author         : Lauri Jämsä / lauri.jamsa@ruuvipenkki.fi / ruuvipenkki.fi  *
* Created        : 01/07/2012                                                 *
* Description    : Main file                                                  *
* License        : BSD                                                        *
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "platform_config.h"
#include "hw_config.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
static __IO uint32_t TimingDelay;

/* Private function prototypes ----------------------------------------------*/
void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);

/* Private functions --------------------------------------------------------*/


/**
  * @brief  Main function
  * @param  None
  * @retval None
  */
int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	/*!< At this stage the microcontroller clock setting is already configured, 
	this is done through SystemInit() function which is called from startup
	file (startup_stm32f4xx.S) before to branch to application main.
	To reconfigure the default setting of SystemInit() function, refer to
	system_stm32f4xx.c file
	*/  

	/* SysTick end of count event each 10ms */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);


	Set_System();

	while (1)
	{
		GPIO_SetBits(LED1_PORT, LED1);
		GPIO_SetBits(LED2_PORT, LED2);
		/* Insert 500 ms delay */
		Delay(50);

		GPIO_ResetBits(LED1_PORT, LED1);
		GPIO_ResetBits(LED2_PORT, LED2);
		/* Insert 500 ms delay */
		Delay(50);
	}
}


/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
	/* User can add his own implementation to report the file name and line number,
	ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	while (1)
	{
	}
}
#endif

/**
  * @}
  */
