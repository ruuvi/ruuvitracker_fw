/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for the RuuviTracker REVA <http://www.ruuvipenkki.fi>
 */

/*
 * Board identifier.
 */
#define BOARD_RUUVITRACKER_REVA
#define BOARD_NAME "RuuviTracker REVA"

/*
 * Board frequencies.
 */
#define STM32_LSECLK            32768
#define STM32_HSECLK            12000000 // !External crystal freq!

/*
 * MCU type, supported types are defined in ./os/hal/platforms/hal_lld.h.
 * Note: Older board revisions should define STM32F10X_HD instead, please
 *       verify the STM32 model mounted on your board. The change also
 *       affects your linker script.
 */
#define STM32F10X_HD

/*
 * IO pins assignments.
 */
/* GPIOA */
#define GPIOA_ACCMAGN_INT1	0
#define GPIOA_ACCMAGN_INT2	1
#define GPIOA_USART2_TX		2
#define GPIOA_USART2_RX		3
#define GPIOA_DAC_OUT1		4
#define GPIOA_SPI1_SCK		5
#define GPIOA_SPI1_MISO		6
#define GPIOA_SPI1_MOSI		7
#define GPIOA_ACCMAGN_DRDY	8
#define GPIOA_USART1_TX		9
#define GPIOA_USART1_RX		10
#define GPIOA_USB_DM		11
#define GPIOA_USB_DP		12
#define GPIOA_JTAG_TMS		13
#define GPIOA_JTAG_TCK		14
#define GPIOA_JTAG_TCI		15

/* GPIOB */
#define GPIOB_JTAG_TDO		3
#define GPIOB_JTAG_TRST		4
#define GPIOB_I2C1_SCL		6
#define GPIOB_I2C1_SDA 		7
#define GPIOB_FLAG1		8
#define GPIOB_FLAG2		9
#define GPIOB_USART3_TX		10
#define GPIOB_USART3_RX		11
#define GPIOB_USART3_CTS	13
#define GPIOB_USART3_RTS	14

/* GPIOC */
#define GPIOC_PC0		0
#define GPIOC_PC1		1
#define GPIOC_PC2		2
#define GPIOC_RING		3
#define GPIOC_PWRKEY		4
#define GPIOC_DTR		5
#define GPIOC_DCD		6
#define GPIOC_SD_CARD_INSERT	7
#define GPIOC_SDIO_D0		8
#define GPIOC_SDIO_D1		9
#define GPIOC_SDIO_D2		10
#define GPIOC_SDIO_D3		11
#define GPIOC_SDIO_CLK		12
#define GPIOC_MICROSD_CS	13
#define GPIOC_LED1              14
#define GPIOC_LED2              15

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_ANALOG(n)           (0 << (((n) & 7) * 4))
#define PIN_OUTPUT_PP_10(n)     (1 << (((n) & 7) * 4))
#define PIN_OUTPUT_PP_2(n)      (2 << (((n) & 7) * 4))
#define PIN_OUTPUT_PP_50(n)     (3 << (((n) & 7) * 4))
#define PIN_INPUT(n)            (4 << (((n) & 7) * 4))
#define PIN_OUTPUT_OD_10(n)     (5 << (((n) & 7) * 4))
#define PIN_OUTPUT_OD_2(n)      (6 << (((n) & 7) * 4))
#define PIN_OUTPUT_OD_50(n)     (7 << (((n) & 7) * 4))
#define PIN_INPUT_PUD(n)        (8 << (((n) & 7) * 4))
#define PIN_ALTERNATE_PP_10(n)  (9 << (((n) & 7) * 4))
#define PIN_ALTERNATE_PP_2(n)   (10 << (((n) & 7) * 4))
#define PIN_ALTERNATE_PP_50(n)  (11 << (((n) & 7) * 4))
#define PIN_ALTERNATE_OD_10(n)  (13 << (((n) & 7) * 4))
#define PIN_ALTERNATE_OD_2(n)   (14 << (((n) & 7) * 4))
#define PIN_ALTERNATE_OD_50(n)  (15 << (((n) & 7) * 4))
#define PIN_UNDEFINED(n)        PIN_INPUT_PUD(n)

/*
 * Port A setup.
 */
#define VAL_GPIOACRL    (PIN_INPUT(0)           | /* ACCMAGN_INT1 	*/  \
                         PIN_INPUT(1)    	| /* ACCMAGN_INT2       */  \
                         PIN_ALTERNATE_PP_50(2) | /* USART2_TX          */  \
                         PIN_INPUT(3)           | /* USART2_RX          */  \
                         PIN_OUTPUT_PP_50(4)    | /* DAC_OUT1           */  \
                         PIN_ALTERNATE_PP_50(5) | /* SPI1_SCK.          */  \
                         PIN_INPUT(6)           | /* SPI1_MISO.         */  \
                         PIN_ALTERNATE_PP_50(7))  /* SPI1_MOSI.         */
#define VAL_GPIOACRH    (PIN_INPUT(8) 		| /* ACCMAGN_DRDY       */  \
                         PIN_ALTERNATE_PP_50(9) | /* USART1_TX.         */  \
                         PIN_INPUT(10)          | /* USART1_RX.         */  \
                         PIN_INPUT_PUD(11)      | /* USB_DM.            */  \
                         PIN_INPUT_PUD(12)      | /* USB_DP.            */  \
                         PIN_INPUT(13)          | /* JTAG_TMS           */  \
                         PIN_INPUT(14)          | /* JTAG_TCK           */  \
                         PIN_INPUT(15))           /* JTAG_TCI           */
#define VAL_GPIOAODR    0xFFFFFFFF

/*
 * Port B setup.
 */
#define VAL_GPIOBCRL    (PIN_UNDEFINED(0)    	| /*                    */  \
                         PIN_UNDEFINED(1)       | /*                    */  \
                         PIN_UNDEFINED(2)    	| /*                    */  \
                         PIN_INPUT(3)           | /* JTAG_TDO           */  \
                         PIN_INPUT(4)           | /* JTAG_TRST          */  \
                         PIN_UNDEFINED(5)    	| /*                    */  \
                         PIN_ALTERNATE_OD_50(6) | /* I2C1_SCL           */  \
                         PIN_ALTERNATE_OD_50(7))  /* I2C1_SDA           */
#define VAL_GPIOBCRH    (PIN_INPUT(8)           | /* FLAG1              */  \
                         PIN_INPUT(9) 		| /* FLAG2              */  \
                         PIN_ALTERNATE_PP_50(10)| /* USART3_TX      	*/  \
                         PIN_INPUT(11)   	| /* USART3_RX      	*/  \
                         PIN_UNDEFINED(12)	| /*                    */  \
                         PIN_INPUT(13)      	| /* USART3_CTS         */  \
                         PIN_OUTPUT_PP_50(14)   | /* USART3_RTS         */  \
                         PIN_UNDEFINED(15))
#define VAL_GPIOBODR    0xFFFFFFFF

/*
 * Port C setup.
 */
#define VAL_GPIOCCRL    (PIN_OUTPUT_PP_50(0) 	| /* PC0                */  \
                         PIN_OUTPUT_PP_50(1) 	| /* PC1                */  \
                         PIN_OUTPUT_PP_50(2)	| /* PC2                */  \
                         PIN_INPUT(3)       	| /* RING               */  \
                         PIN_OUTPUT_PP_50(4)   	| /* PWRKEY             */  \
                         PIN_INPUT(5)       	| /* DTR                */  \
                         PIN_INPUT(6)       	| /* DCD                */  \
                         PIN_INPUT(7))            /* SD_CARD_INSERT     */
#define VAL_GPIOCCRH    (PIN_ALTERNATE_PP_50(8) | /* SDIO D0.           */  \
                         PIN_ALTERNATE_PP_50(9) | /* SDIO D1.           */  \
                         PIN_ALTERNATE_PP_50(10)| /* SDIO D2.           */  \
                         PIN_ALTERNATE_PP_50(11)| /* SDIO D3.           */  \
                         PIN_ALTERNATE_PP_50(12)| /* SDIO CLK.          */  \
                         PIN_OUTPUT_PP_50(13)   | /* MICROSD_CS         */  \
                         PIN_OUTPUT_PP_50(14)   | /* LED1               */  \
                         PIN_OUTPUT_PP_50(15))    /* LED2               */
#define VAL_GPIOCODR    0xFFFFFFFF

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
