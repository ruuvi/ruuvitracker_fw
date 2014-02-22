#include "ch.h"
#include "hal.h"
#include "usb_serial.h"


/* Just reset the system */
static void callback2(void *arg)
{
    (void)arg;
    NVIC_SystemReset();
}

/*
 * Called after one second from button press.
 * Checks if button is still pressed.
 * If it is, sets a second callback that resets the device.
 */
static void callback1(void *arg)
{
    static VirtualTimer vt;
    (void)arg;

    // Still pressed?
    if (PAL_LOW == palReadPad(GPIOA, GPIOA_BUTTON))
        return;

    chSysLockFromIsr();
    if (chVTIsArmedI(&vt))
        chVTResetI(&vt);
    chVTSetI(&vt, S2ST(4), callback2, NULL); //Reset after 5 seconds
    chSysUnlockFromIsr();
    /* Set both leds. NOTE: ChibiOS is still running so
     * these can be controlled from any other thread */
    palSetPad(GPIOB, GPIOB_LED1);
    palSetPad(GPIOB, GPIOB_LED2);
}

/*
 * Triggered when the button is pressed.
 * Sets the timer callback to check the button status after a second.
 */
static void extcb1(EXTDriver *extp, expchannel_t channel) {
    static VirtualTimer vt;

    (void)extp;
    (void)channel;

    palSetPad(GPIOB, GPIOB_LED2);
    chSysLockFromIsr();
    if (chVTIsArmedI(&vt))
        chVTResetI(&vt);

    chVTSetI(&vt, S2ST(1), callback1, NULL);
    chSysUnlockFromIsr();
}

/*
 * EXT configuration table
 * We need only PA0
 */
static const EXTConfig extcfg = {
    {
        {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL}
    }
};

void button_init(void)
{
    /*
     * Activates the EXT driver 1.
     */
    extStart(&EXTD1, &extcfg);
}
