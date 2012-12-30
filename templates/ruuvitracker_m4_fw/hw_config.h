/******************************************************************************
* Project        : RuuviTracker                                               *
* File Name      : hw_config.h                                                *
* Author         : Lauri Jämsä / lauri.jamsa@ruuvipenkki.fi / ruuvipenkki.fi  *
* Created        : 01/07/2012                                                 *
* Description    : Header file                                                *
* License        : BSD                                                        *
******************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

/* Includes -----------------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/* Exported macro -----------------------------------------------------------*/
/* Exported define ----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
void Set_System(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void RCC_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void EXTI_Configuration(void);
void USART_Configuration(void);
void SetDelay_10us(__IO unsigned long int nTime);
void SetDelay_100us(__IO unsigned long int nTime);
void SetDelay_1ms(__IO unsigned long int nTime);
void TimingDelay_Decrement(void);

/* External variables -------------------------------------------------------*/
static __IO unsigned long int TimingDelay;

#endif  /*__HW_CONFIG_H*/
