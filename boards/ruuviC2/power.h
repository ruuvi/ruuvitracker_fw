#ifndef POWER_H
#define POWER_H

enum POWER_DOMAIN {
     LDO2=0,
     LDO3,
     LDO4,
     GSM,
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

#endif /* POWER_H */
