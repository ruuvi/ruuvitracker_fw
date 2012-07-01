/******************************************************************************
* Project        : RuuviTracker                                               *
* File Name      : platform_config.h                                          *
* Author         : Lauri Jämsä / lauri.jamsa@ruuvipenkki.fi / ruuvipenkki.fi  *
* Created        : 01/07/2012                                                 *
* Description    : HW specific platform config header file                    *
* License        : BSD                                                        *
******************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

/* Includes -----------------------------------------------------------------*/
/* Defines ------------------------------------------------------------------*/

/* Statusled defines */
#define LED1_PORT					GPIOC
#define LED2_PORT					GPIOC
#define LED1						GPIO_Pin_14
#define LED2						GPIO_Pin_15

/* USART1 defines */
#define USART1_PORT					GPIOA
#define USART1_RXPIN				GPIO_Pin_10
#define USART1_TXPIN				GPIO_Pin_9

/* USART2 defines */
#define USART2_PORT					GPIOA
#define USART2_RXPIN				GPIO_Pin_3
#define USART2_TXPIN				GPIO_Pin_2

/* USART3 defines */
#define USART3_PORT					GPIOB
#define USART3_RXPIN				GPIO_Pin_10
#define USART3_TXPIN				GPIO_Pin_11
#define USART3_CTSPIN				GPIO_Pin_13
#define USART3_RTSPIN				GPIO_Pin_14


#endif /* __PLATFORM_CONFIG_H */
