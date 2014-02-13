#include "ch.h"
#include "hal.h"
#include "power.h"

/* Prototypes */
static void enable_ldo2(void);
static void disable_ldo2(void);
static void enable_ldo3(void);
static void disable_ldo3(void);
static void enable_ldo4(void);
static void disable_ldo4(void);
static void enable_gsm_fet(void);
static void disable_gsm_fet(void);

struct power_domains_t {
     enum POWER_DOMAIN domain;
     void (*enable)(void);
     void (*disable)(void);
     uint8_t used;
};

/* Define power domain handlers, MUST BE IN SAME ORDER as POWER_DOMAINS */
static struct power_domains_t power_domains[] = {
     { LDO2, enable_ldo2, disable_ldo2, 0 },
     { LDO3, enable_ldo3, disable_ldo3, 0 },
     { LDO4, enable_ldo4, disable_ldo4, 0 },
     { GSM, enable_gsm_fet, disable_gsm_fet, 0 },
};

static BinarySemaphore sem;
#define LOCK chBSemWait(&sem);
#define UNLOCK chBSemSignal(&sem);

void power_init(void)
{
     chBSemInit(&sem, FALSE);
}

void power_request(enum POWER_DOMAIN p)
{
     LOCK;
     if (!power_domains[p].used)
	  power_domains[p].enable();
     power_domains[p].used++;
     UNLOCK;
}

void power_release(enum POWER_DOMAIN p)
{
     LOCK;
     power_domains[p].used--;
     if (!power_domains[p].used)
	  power_domains[p].disable();
     UNLOCK;
}

/* Actual functions to enable or disable power */

static void enable_ldo2(void)
{
     palSetPad(GPIOC, GPIOC_ENABLE_LDO2);
}

static void disable_ldo2(void)
{
     palClearPad(GPIOC, GPIOC_ENABLE_LDO2);
}

static void enable_ldo3(void)
{
     palSetPad(GPIOC, GPIOC_ENABLE_LDO3);
}

static void disable_ldo3(void)
{
     palClearPad(GPIOC, GPIOC_ENABLE_LDO3);
}

static void enable_ldo4(void)
{
     palSetPad(GPIOC, GPIOC_ENABLE_LDO4);
}

static void disable_ldo4(void)
{
     palClearPad(GPIOC, GPIOC_ENABLE_LDO4);
}

static void enable_gsm_fet(void)
{
     palSetPad(GPIOC, GPIOC_ENABLE_GSM_VBAT);
}

static void disable_gsm_fet(void)
{
     palClearPad(GPIOC, GPIOC_ENABLE_GSM_VBAT);
}
