/***
 * Skeleton code for RuuviTracker REVA ChibiOS project
 * by Teemu Rasi 2012, teemu.rasi@gmail.com
 *
 * Modified from chibios-skeleton project originally made by
 * Kalle Vahlman, <zuh@iki.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
// ChibiOS specific includes
#include "ch.h"
#include "hal.h"

// Project includes
#include "utils.h"

// Setup a working area with a 32 byte stack for LED flashing thread
static WORKING_AREA(led_thread, 32);
static WORKING_AREA(led_thread2, 32);

// Thread for GPIOC_LED1
static msg_t led1Thread(void *UNUSED(arg))
{
    while(TRUE)
    {
        // Toggle GPIOC_LED1
        palTogglePad(GPIOC, GPIOC_LED1);
	// Send one character to serial port
	sdPutTimeout(&SD1, 'C', 1);
        // Sleep
        chThdSleepMilliseconds(1000);
    }
    return 0;
}

// Thread for GPIOC_LED2
static msg_t led2Thread(void *UNUSED(arg))
{
    while(TRUE)
    {
        // Toggle GPIOC_LED2
        palTogglePad(GPIOC, GPIOC_LED2);
        // Sleep
        chThdSleepMilliseconds(500);
    }
    return 0;
}

int main(void) {

    // Initialize ChibiOS HAL and core
    halInit();
    chSysInit();
    
    // Start serial driver for USART1 using default configuration
    sdStart(&SD1, NULL);

    // Start a thread dedicated to flashing GPIOC_LED1 and testing serial write
    chThdCreateStatic(led_thread, sizeof(led_thread), NORMALPRIO, led1Thread, NULL);
    // Start a thread dedicated to flashing GPIOC_LED2
    chThdCreateStatic(led_thread2, sizeof(led_thread2), NORMALPRIO, led2Thread, NULL);

    // main loop
    while (TRUE)
    {
	chThdSleepMilliseconds(500);
    }
    return 0;
}
