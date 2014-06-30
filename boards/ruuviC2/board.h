/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for STMicroelectronics STM32F4-Discovery board.
 */

/*
 * Board identifier.
 */
#define BOARD_RUUVITRACKERC2
#define BOARD_NAME                  "Ruuvitracker C2"


/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0
#endif

#if !defined(STM32_HSECLK)
#define STM32_HSECLK               16000000
#endif

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   300

/*
 * MCU type as defined in the ST header.
 */
#define STM32F40_41xxx

/*
 * IO pins assignments.
 */
#define GPIOA_BUTTON                0
#define GPIOA_ACC_INT2              1
#define GPIOA_GPS_RXD               2
#define GPIOA_GPS_TXD               3
#define GPIOA_GSM_DCD               4
#define GPIOA_SPI_SCK               5
#define GPIOA_SPI_MISO              6
#define GPIOA_SPI_MOSI              7
#define GPIOA_GSM_RI                8
#define GPIOA_USART1_RXD            9
#define GPIOA_USART1_TXD            10
#define GPIOA_USB_DM                11
#define GPIOA_USB_DP                12
#define GPIOA_JTAG_TMS              13
#define GPIOA_JTAG_TCK              14
#define GPIOA_JTAG_TDI              15

#define GPIOB_PIN0                  0
#define GPIOB_PIN1                  1
#define GPIOB_PIN2                  2
#define GPIOB_JTAG_TDO              3
#define GPIOB_JTAG_TRST             4
#define GPIOB_PIN5                  5
#define GPIOB_SCL                   6
#define GPIOB_SDA                   7
#define GPIOB_LED1                  8
#define GPIOB_LED2                  9
#define GPIOB_GSM_RXD               10
#define GPIOB_GSM_TXD               11
#define GPIOB_PIN12                 12
#define GPIOB_GSM_RTS               13
#define GPIOB_GSM_CTS               14
#define GPIOB_MICROSD_CS            15

#define GPIOC_ENABLE_LDO4           0
#define GPIOC_GPS_WAKEUP            1
#define GPIOC_PIN2                  2
#define GPIOC_PIN3                  3
#define GPIOC_GSM_PWRKEY            4
#define GPIOC_GSM_DTR               5
#define GPIOC_ENABLE_LDO3           6
#define GPIOC_ENABLE_GSM_VBAT       7
#define GPIOC_ENABLE_LDO2           8
#define GPIOC_GPS_1PPS_SIGNAL       9
#define GPIOC_SD_CARD_INSERTED      10
#define GPIOC_CHARGER_STATUS        11
#define GPIOC_DISABLE_CHARGER       12
#define GPIOC_GPS_V_BACKUP_PWR      13
#define GPIOC_GSM_NETLIGHT          14
#define GPIOC_GSM_STATUS            15

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_2M(n)            (0U << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1U << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2U << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3U << ((n) * 2))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2))
#define PIN_AFIO_AF(n, v)           ((v##U) << ((n % 8) * 4))

/*
 * GPIOA setup:
 *
 * PA0  - BUTTON                    (input floating).
 * PA1  - ACC_INT2                  (input floating).
 * PA2  - GPS_RXD                   (alternate 7).
 * PA3  - GPS_TXD                   (alternate 7).
 * PA4  - GSM_DCD                   (input floating).
 * PA5  - SPI_SCK                   (alternate 5).
 * PA6  - SPI_MISO                  (alternate 5).
 * PA7  - SPI_MOSI                  (alternate 5).
 * PA8  - GSM_RI                    (input floating).
 * PA9  - UART1_TXD                 (alternate 7).
 * PA10 - UART1_RXD                 (alternate 7).
 * PA11 - USB_DM                    (alternate 10).
 * PA12 - USB_DP                    (alternate 10).
 * PA13 - JTAG_TMS                  (alternate 0).
 * PA14 - JTAG_TCK                  (alternate 0).
 * PA15 - JTAG_TDI                  (alternate 0).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(GPIOA_BUTTON)         | \
                                     PIN_MODE_INPUT(GPIOA_ACC_INT2)       | \
                                     PIN_MODE_ALTERNATE(GPIOA_GPS_RXD)    | \
                                     PIN_MODE_ALTERNATE(GPIOA_GPS_TXD)    | \
                                     PIN_MODE_INPUT(GPIOA_GSM_DCD)        | \
                                     PIN_MODE_ALTERNATE(GPIOA_SPI_SCK)    | \
                                     PIN_MODE_ALTERNATE(GPIOA_SPI_MISO)   | \
                                     PIN_MODE_ALTERNATE(GPIOA_SPI_MOSI)   | \
                                     PIN_MODE_INPUT(GPIOA_GSM_RI)         | \
                                     PIN_MODE_ALTERNATE(GPIOA_USART1_TXD) | \
                                     PIN_MODE_ALTERNATE(GPIOA_USART1_RXD) | \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DM)     | \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DP)     | \
                                     PIN_MODE_ALTERNATE(GPIOA_JTAG_TMS)   | \
                                     PIN_MODE_ALTERNATE(GPIOA_JTAG_TCK)   | \
                                     PIN_MODE_ALTERNATE(GPIOA_JTAG_TDI))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_BUTTON)     | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACC_INT2)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_GPS_TXD)    | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_GPS_RXD)    | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DM)     | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DP)     | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_JTAG_TMS)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_JTAG_TCK)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_JTAG_TDI))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_100M(GPIOA_BUTTON)        | \
                                     PIN_OSPEED_100M(GPIOA_ACC_INT2)      | \
                                     PIN_OSPEED_50M(GPIOA_GPS_TXD)        | \
                                     PIN_OSPEED_50M(GPIOA_GPS_RXD)        | \
                                     PIN_OSPEED_50M(GPIOA_GSM_DCD)        | \
                                     PIN_OSPEED_100M(GPIOA_SPI_SCK)       | \
                                     PIN_OSPEED_100M(GPIOA_SPI_MISO)      | \
                                     PIN_OSPEED_100M(GPIOA_SPI_MOSI)      | \
                                     PIN_OSPEED_50M(GPIOA_GSM_RI)         | \
                                     PIN_OSPEED_50M(GPIOA_USART1_TXD)     | \
                                     PIN_OSPEED_50M(GPIOA_USART1_RXD)     | \
                                     PIN_OSPEED_100M(GPIOA_USB_DM)        | \
                                     PIN_OSPEED_100M(GPIOA_USB_DP)        | \
                                     PIN_OSPEED_100M(GPIOA_JTAG_TMS)      | \
                                     PIN_OSPEED_100M(GPIOA_JTAG_TCK)      | \
                                     PIN_OSPEED_100M(GPIOA_JTAG_TDI))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_FLOATING(GPIOA_BUTTON)     | \
                                     PIN_PUPDR_FLOATING(GPIOA_ACC_INT2)   | \
                                     PIN_PUPDR_FLOATING(GPIOA_GPS_RXD)    | \
                                     PIN_PUPDR_FLOATING(GPIOA_GPS_TXD)    | \
                                     PIN_PUPDR_FLOATING(GPIOA_GSM_DCD)    | \
                                     PIN_PUPDR_FLOATING(GPIOA_SPI_SCK)    | \
                                     PIN_PUPDR_PULLUP(GPIOA_SPI_MISO)   | \
                                     PIN_PUPDR_FLOATING(GPIOA_SPI_MOSI)   | \
                                     PIN_PUPDR_FLOATING(GPIOA_GSM_RI)     | \
                                     PIN_PUPDR_FLOATING(GPIOA_USART1_TXD) | \
                                     PIN_PUPDR_FLOATING(GPIOA_USART1_RXD) | \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_DM)     | \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_DP)     | \
                                     PIN_PUPDR_FLOATING(GPIOA_JTAG_TMS)   | \
                                     PIN_PUPDR_FLOATING(GPIOA_JTAG_TCK)   | \
                                     PIN_PUPDR_FLOATING(GPIOA_JTAG_TDI))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_BUTTON)       | \
                                     PIN_ODR_HIGH(GPIOA_ACC_INT2)     | \
                                     PIN_ODR_HIGH(GPIOA_GPS_RXD)      | \
                                     PIN_ODR_HIGH(GPIOA_GPS_TXD)      | \
                                     PIN_ODR_HIGH(GPIOA_GSM_DCD)      | \
                                     PIN_ODR_HIGH(GPIOA_SPI_SCK)      | \
                                     PIN_ODR_HIGH(GPIOA_SPI_MISO)     | \
                                     PIN_ODR_HIGH(GPIOA_SPI_MOSI)     | \
                                     PIN_ODR_HIGH(GPIOA_GSM_RI)       | \
                                     PIN_ODR_HIGH(GPIOA_USART1_TXD)   | \
                                     PIN_ODR_HIGH(GPIOA_USART1_RXD)   | \
                                     PIN_ODR_HIGH(GPIOA_USB_DM)       | \
                                     PIN_ODR_HIGH(GPIOA_USB_DP)       | \
                                     PIN_ODR_HIGH(GPIOA_JTAG_TMS)     | \
                                     PIN_ODR_HIGH(GPIOA_JTAG_TCK)     | \
                                     PIN_ODR_HIGH(GPIOA_JTAG_TDI))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_BUTTON, 0)      | \
                                     PIN_AFIO_AF(GPIOA_ACC_INT2, 0)    | \
                                     PIN_AFIO_AF(GPIOA_GPS_RXD, 7)     | \
                                     PIN_AFIO_AF(GPIOA_GPS_TXD, 7)     | \
                                     PIN_AFIO_AF(GPIOA_GSM_DCD, 0)     | \
                                     PIN_AFIO_AF(GPIOA_SPI_SCK, 5)     | \
                                     PIN_AFIO_AF(GPIOA_SPI_MISO, 5)    | \
                                     PIN_AFIO_AF(GPIOA_SPI_MOSI, 5))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_GSM_RI, 0)     | \
                                     PIN_AFIO_AF(GPIOA_USART1_TXD, 7) | \
                                     PIN_AFIO_AF(GPIOA_USART1_RXD, 7) | \
                                     PIN_AFIO_AF(GPIOA_USB_DM, 10)    | \
                                     PIN_AFIO_AF(GPIOA_USB_DP, 10)    | \
                                     PIN_AFIO_AF(GPIOA_JTAG_TMS, 0)   | \
                                     PIN_AFIO_AF(GPIOA_JTAG_TCK, 0)   | \
                                     PIN_AFIO_AF(GPIOA_JTAG_TDI, 0))

/*
 * GPIOB setup:
 *
 * PB0  - PIN0                      (input pulldown).
 * PB1  - PIN1                      (input pulldown).
 * PB2  - PIN2                      (input floating).
 * PB3  - JTAG_TDO                  (alternate 0).
 * PB4  - JTAG_TRST                 (alternate 0).
 * PB5  - PIN5                      (input pulldown).
 * PB6  - SCL                       (alternate 4).
 * PB7  - SDA                       (alternate 4).
 * PB8  - LED1                      (output, low).
 * PB9  - LED2                      (output, low).
 * PB10 - GSM_RXD                   (alternate 7).
 * PB11 - GSM_TXD                   (alternate 7).
 * PB12 - PIN12                     (input pulldown).
 * PB13 - GSM_RTS                   (alternate 7).
 * PB14 - GSM_CTS                   (alternate 7).
 * PB15 - MICROSD_CS                (output).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_INPUT(GPIOB_PIN0)          | \
                                     PIN_MODE_INPUT(GPIOB_PIN1)          | \
                                     PIN_MODE_INPUT(GPIOB_PIN2)      | \
                                     PIN_MODE_ALTERNATE(GPIOB_JTAG_TDO)  | \
                                     PIN_MODE_ALTERNATE(GPIOB_JTAG_TRST) | \
                                     PIN_MODE_INPUT(GPIOB_PIN5)      | \
                                     PIN_MODE_ALTERNATE(GPIOB_SCL)   | \
                                     PIN_MODE_ALTERNATE(GPIOB_SDA)   | \
                                     PIN_MODE_OUTPUT(GPIOB_LED1)     | \
                                     PIN_MODE_OUTPUT(GPIOB_LED2)     | \
                                     PIN_MODE_ALTERNATE(GPIOB_GSM_RXD)   | \
                                     PIN_MODE_ALTERNATE(GPIOB_GSM_TXD)   | \
                                     PIN_MODE_INPUT(GPIOB_PIN12)     | \
                                     PIN_MODE_ALTERNATE(GPIOB_GSM_RTS)   | \
                                     PIN_MODE_ALTERNATE(GPIOB_GSM_CTS)   | \
                                     PIN_MODE_OUTPUT(GPIOB_MICROSD_CS))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_PIN0) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN1)     | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN2)     | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_JTAG_TDO) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_JTAG_TRST)| \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN5)     | \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_SCL)      | \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_SDA)      | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LED1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LED2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GSM_RXD) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GSM_TXD) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GSM_RTS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GSM_CTS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_MICROSD_CS))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_50M(GPIOB_PIN0) |          \
                                     PIN_OSPEED_50M(GPIOB_PIN1) |          \
                                     PIN_OSPEED_50M(GPIOB_PIN2) |          \
                                     PIN_OSPEED_100M(GPIOB_JTAG_TDO) |          \
                                     PIN_OSPEED_100M(GPIOB_JTAG_TRST) |          \
                                     PIN_OSPEED_50M(GPIOB_PIN5) |          \
                                     PIN_OSPEED_100M(GPIOB_SCL) |          \
                                     PIN_OSPEED_100M(GPIOB_SDA) |          \
                                     PIN_OSPEED_50M(GPIOB_LED1) |          \
                                     PIN_OSPEED_50M(GPIOB_LED2) |          \
                                     PIN_OSPEED_50M(GPIOB_GSM_RXD) |       \
                                     PIN_OSPEED_50M(GPIOB_GSM_TXD) |       \
                                     PIN_OSPEED_50M(GPIOB_PIN12)   |       \
                                     PIN_OSPEED_50M(GPIOB_GSM_RTS) |       \
                                     PIN_OSPEED_50M(GPIOB_GSM_CTS) |       \
                                     PIN_OSPEED_50M(GPIOB_MICROSD_CS))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_PULLDOWN(GPIOB_PIN0)      | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_PIN1) |      \
                                     PIN_PUPDR_FLOATING(GPIOB_PIN2) |      \
                                     PIN_PUPDR_PULLUP(GPIOB_JTAG_TDO)  |      \
                                     PIN_PUPDR_PULLUP(GPIOB_JTAG_TRST) | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_PIN5) | \
                                     PIN_PUPDR_FLOATING(GPIOB_SCL)  | \
                                     PIN_PUPDR_FLOATING(GPIOB_SDA)  | \
                                     PIN_PUPDR_FLOATING(GPIOB_LED1) | \
                                     PIN_PUPDR_FLOATING(GPIOB_LED2) | \
                                     PIN_PUPDR_FLOATING(GPIOB_GSM_RXD) | \
                                     PIN_PUPDR_FLOATING(GPIOB_GSM_TXD) | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_PIN12)   | \
                                     PIN_PUPDR_FLOATING(GPIOB_GSM_RTS) | \
                                     PIN_PUPDR_FLOATING(GPIOB_GSM_CTS) | \
                                     PIN_PUPDR_FLOATING(GPIOB_MICROSD_CS))
#define VAL_GPIOB_ODR               (PIN_ODR_LOW(GPIOB_LED1) | \
                                     PIN_ODR_LOW(GPIOB_LED2) | \
                                     PIN_ODR_LOW(GPIOB_MICROSD_CS))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_PIN0, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOB_JTAG_TDO, 0) |            \
                                     PIN_AFIO_AF(GPIOB_JTAG_TRST, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOB_SCL, 4) |            \
                                     PIN_AFIO_AF(GPIOB_SDA, 4))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_LED1, 0) |           \
                                     PIN_AFIO_AF(GPIOB_LED2, 4) |            \
                                     PIN_AFIO_AF(GPIOB_GSM_RXD, 7) |         \
                                     PIN_AFIO_AF(GPIOB_GSM_TXD, 7) |          \
                                     PIN_AFIO_AF(GPIOB_PIN12, 0) |          \
                                     PIN_AFIO_AF(GPIOB_GSM_RTS, 7) |          \
                                     PIN_AFIO_AF(GPIOB_GSM_CTS, 7) |          \
                                     PIN_AFIO_AF(GPIOB_MICROSD_CS, 0))

/*
 * GPIOC setup:
 *
 * PC0  - ENABLE_LDO4               (output, low).
 * PC1  - GPS_WAKEUP                (output, low).
 * PC2  - PIN2                      (input pulldown).
 * PC3  - PIN3                      (input pulldown).
 * PC4  - GSM_PWRKEY                (output, open-drain, high).
 * PC5  - GSM_DTR                   (output, low).
 * PC6  - ENABLE_LDO3               (output, low).
 * PC7  - ENABLE_GSM_VBAT           (output, open-drain, low).
 * PC8  - ENABLE_LDO2               (output, low).
 * PC9  - GPS_1PPS_SIGNAL           (input floating).
 * PC10 - SD_CARD_INSERTED          (input, pullup).
 * PC11 - CHARGER_STATUS            (input floating).
 * PC12 - DISABLE_CHARGER           (input floating).
 * PC13 - GPS_V_BACKUP_PWR          (output, high).
 * PC14 - GSM_NETLIGHT              (input floating).
 * PC15 - GSM_STATUS                (input floating).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_OUTPUT(GPIOC_ENABLE_LDO4)    | \
                                     PIN_MODE_OUTPUT(GPIOC_GPS_WAKEUP)     | \
                                     PIN_MODE_INPUT(GPIOC_PIN2)        | \
                                     PIN_MODE_INPUT(GPIOC_PIN3)        | \
                                     PIN_MODE_OUTPUT(GPIOC_GSM_PWRKEY)     | \
                                     PIN_MODE_OUTPUT(GPIOC_GSM_DTR)    | \
                                     PIN_MODE_OUTPUT(GPIOC_ENABLE_LDO3)    | \
                                     PIN_MODE_OUTPUT(GPIOC_ENABLE_GSM_VBAT)| \
                                     PIN_MODE_OUTPUT(GPIOC_ENABLE_LDO2)    | \
                                     PIN_MODE_INPUT(GPIOC_GPS_1PPS_SIGNAL) | \
                                     PIN_MODE_INPUT(GPIOC_SD_CARD_INSERTED)| \
                                     PIN_MODE_INPUT(GPIOC_CHARGER_STATUS)  | \
                                     PIN_MODE_INPUT(GPIOC_DISABLE_CHARGER) | \
                                     PIN_MODE_OUTPUT(GPIOC_GPS_V_BACKUP_PWR) | \
                                     PIN_MODE_INPUT(GPIOC_GSM_NETLIGHT)    | \
                                     PIN_MODE_INPUT(GPIOC_GSM_STATUS))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_ENABLE_LDO4)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_GPS_WAKEUP)    | \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_GSM_PWRKEY)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_GSM_DTR)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_ENABLE_LDO3)   | \
                                     PIN_OTYPE_OPENDRAIN(GPIOC_ENABLE_GSM_VBAT)| \
                                     PIN_OTYPE_PUSHPULL(GPIOC_ENABLE_LDO2)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_GPS_V_BACKUP_PWR))
#define VAL_GPIOC_OSPEEDR           (0) /* Assume that low speed is enough for all outputs here */
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_PULLDOWN(GPIOC_PIN2)         | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_PIN3)     | \
                                     PIN_PUPDR_FLOATING(GPIOC_GPS_1PPS_SIGNAL) | \
                                     PIN_PUPDR_PULLUP(GPIOC_SD_CARD_INSERTED) | \
                                     PIN_PUPDR_FLOATING(GPIOC_CHARGER_STATUS) | \
                                     PIN_PUPDR_FLOATING(GPIOC_DISABLE_CHARGER) | \
                                     PIN_PUPDR_FLOATING(GPIOC_GSM_NETLIGHT) | \
                                     PIN_PUPDR_FLOATING(GPIOC_GSM_STATUS))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_GSM_PWRKEY)   | \
                                     PIN_ODR_HIGH(GPIOC_ENABLE_GSM_VBAT))
#define VAL_GPIOC_AFRL              (0)
#define VAL_GPIOC_AFRH              (0)



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
