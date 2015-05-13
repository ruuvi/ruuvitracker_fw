#define RUUVITRACKER_C3

#define MICROPY_HW_BOARD_NAME       "RuuviTracker_C3"
#define MICROPY_HW_MCU_NAME         "STM32F405RG"
#define MICROPY_PY_SYS_PLATFORM     "ruuvitracker"

#define MICROPY_HW_HAS_SWITCH       (1)
/**
 * Until micropython supports SPI interface in addition to SDIO, we do not have SDCARD as far as the kernel is concerned
#define MICROPY_HW_HAS_SDCARD       (1)
*/
#define MICROPY_HW_HAS_SDCARD       (0)
#define MICROPY_HW_HAS_MMA7660      (0)
#define MICROPY_HW_HAS_LIS3DSH      (0)
#define MICROPY_HW_HAS_LCD          (0)
#define MICROPY_HW_ENABLE_RNG       (1)
#define MICROPY_HW_ENABLE_RTC       (1)
#define MICROPY_HW_ENABLE_TIMER     (1)
#define MICROPY_HW_ENABLE_SERVO     (1)
#define MICROPY_HW_ENABLE_DAC       (0) // All the DAC capable pins are reserved for other uses.
#define MICROPY_HW_ENABLE_SPI1      (1) // Only SPI1 available, other pins reserved
#define MICROPY_HW_ENABLE_SPI2      (0)
#define MICROPY_HW_ENABLE_SPI3      (0)
#define MICROPY_HW_ENABLE_CAN       (0) // All the CAN capable pins are reserved for other uses.


// HSE is 16MHz
#define MICROPY_HW_CLK_PLLM (16)
// TODO: Doublecheck these numbers they're copied from PYBv10 which has 8MHz HSE <- just about every other board has same numbers regardles of their HSE (8, 12 or 25)
#define MICROPY_HW_CLK_PLLN (336)
#define MICROPY_HW_CLK_PLLP (RCC_PLLP_DIV2)
#define MICROPY_HW_CLK_PLLQ (7)

// We do not have 32kHz crystal for the RTC
#define MICROPY_HW_RTC_USE_LSE      (0)


// map RT WKUP to USRSW (though this may be a bad idea when using accelerometer wakeup...
#define MICROPY_HW_USRSW_PIN        (pin_A0)
#define MICROPY_HW_USRSW_PULL       (GPIO_PULLDOWN)
#define MICROPY_HW_USRSW_EXTI_MODE  (GPIO_MODE_IT_RISING)
#define MICROPY_HW_USRSW_PRESSED    (1)

// The pyboard has 4 LEDs
#define MICROPY_HW_LED1             (pin_B8) // red
#define MICROPY_HW_LED2             (pin_B9) // green
// Apparently we do not have to map 4 LEDs
//#define MICROPY_HW_LED3             (pin_B8) // yellow (just map the red)
//#define MICROPY_HW_LED4             (pin_B9)  // blue  (just map the green)
#define MICROPY_HW_LED_OTYPE        (GPIO_MODE_OUTPUT_PP)
#define MICROPY_HW_LED_ON(pin)      (pin->gpio->BSRRL = pin->pin_mask)
#define MICROPY_HW_LED_OFF(pin)     (pin->gpio->BSRRH = pin->pin_mask)

// SD card detect switch
#define MICROPY_HW_SDCARD_DETECT_PIN        (pin_C10)
#define MICROPY_HW_SDCARD_DETECT_PULL       (GPIO_PULLUP)
#define MICROPY_HW_SDCARD_DETECT_PRESENT    (GPIO_PIN_RESET)

// USB config
/**
 * We do not have VUSB detect nor OTG
#define MICROPY_HW_USB_VBUS_DETECT_PIN (pin_A9)
#define MICROPY_HW_USB_OTG_ID_PIN      (pin_A10)
*/

// I2C config
#define MICROPY_HW_I2C1_SCL (pin_B6)
#define MICROPY_HW_I2C1_SDA (pin_B7)


// UARTs config
// GSM DEBUG port
#define MICROPY_HW_UART1_PORT    GPIOA
#define MICROPY_HW_UART1_PINS    (GPIO_PIN_10 | GPIO_PIN_9)
// GPS port (not using RTS/CTS but they should be defined)
#define MICROPY_HW_UART2_PORT    GPIOA
#define MICROPY_HW_UART2_PINS    (GPIO_PIN_2 | GPIO_PIN_3)
#define MICROPY_HW_UART2_RTS     GPIO_PIN_1
#define MICROPY_HW_UART2_CTS     GPIO_PIN_0
// GSM normal port
#define MICROPY_HW_UART3_PORT    GPIOB
#define MICROPY_HW_UART3_PINS    (GPIO_PIN_10 | GPIO_PIN_11)
#define MICROPY_HW_UART3_RTS     GPIO_PIN_14
#define MICROPY_HW_UART3_CTS     GPIO_PIN_13
// Not used but should be defined
#define MICROPY_HW_UART4_PORT    GPIOA
#define MICROPY_HW_UART4_PINS    (GPIO_PIN_0 | GPIO_PIN_1)
// Not used but should be defined
#define MICROPY_HW_UART6_PORT    GPIOC
#define MICROPY_HW_UART6_PINS    (GPIO_PIN_6 | GPIO_PIN_7)
