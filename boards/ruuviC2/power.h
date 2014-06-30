#ifndef POWER_H
#define POWER_H

enum POWER_DOMAIN {
     LDO2=0,    // MicroSD, GPS Antenna
     LDO3,      // GPS_VCC
     LDO4,      // Expansion port
     GSM,
     GPS_VBACKUP, // GPS backup power, for warm starts...
};

/**
 * Initialize power domain handler.
 * called from  boardInit()
 */
void power_init(void);

/**
 * Request power domain.
 * Enable power to specified domain if it is not already enabled.
 */
void power_request(enum POWER_DOMAIN);

/**
 * Release power domain.
 * If no one uses specified power domain anymore, it is disabled.
 */
void power_release(enum POWER_DOMAIN);


/**
 * puts the MCU to STOP mode
 *
 * I/Os retain state but main clock is stopped
 */
void power_enter_stop(void);

/**
 * puts the MCU to STANDBY mode
 *
 * Everything is shut down, when the MCU comes back up it starts with frsh RAM etc
 */
void power_enter_standby(void);

/**
 * Helper to register a callback to the PA0 button and RTC wakeup
 *
 * Used by power_enter_stop() to register power_wakeup_callback()
 */
void register_power_wakeup_callback(extcallback_t cb);

/**
 * Callback that handles restarting clocks etc when coming back from STOP mode
 *
 * This will override any previous callback in the PA0 or RTC interrupts
 */
void power_wakeup_callback(EXTDriver *extp, expchannel_t channel);

/**
 * Shell command to enter STOP mode
 */
void cmd_stop(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * Shell command to enter STANDBY mode
 */
void cmd_standby(BaseSequentialStream *chp, int argc, char *argv[]);


#endif /* POWER_H */
