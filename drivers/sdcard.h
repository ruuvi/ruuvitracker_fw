#ifndef SDCARD_H
#define SDCARD_H
#include "ch.h"
#include "hal.h"
#include "power.h"
#include <ff.h>
// These are going to be needed all over the place so define them as externals
extern FATFS MMC_FS;
extern MMCDriver MMCD1;

#define SDCARD_POWER_DOMAIN LDO2

/**
 * powers up the sdcard 
 *
 * mounting the FS is handled by the MMC driver insertion callback (maybe...)
 *
 */
void sdcard_enable(void);

/**
 * powers down the sdcard (if nothing else is active in the same power domain)
 *
 * This will also unmount the filesystem regardless of other things in the power domain
 *
 */
void sdcard_disable(void);

/**
 * Check whether we have a mounted fs
 */
bool_t sdcard_fs_ready(void);

/**
 * MMCD system init 
 *
 * this should be called from main()
 */
void sdcard_mmcd_init(void);

/**
 * Attempts to mount a filesystem on the card
 * 
 * Returns the FRESULT from f_mount
 */
FRESULT sdcard_mount(void);

/**
 * Umounts the filesystem
 *
 * Returns the FRESULT from f_mount
 */
FRESULT sdcard_unmount(void);


/**
 * Shell command enable sdcard regulator
 */
void sdcard_cmd_enable(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * Shell command to (enable sdcard and) mount filesystem
 */
void sdcard_cmd_mount(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * Shell command ton unmount filesystem (and disable sdcard)
 */
void sdcard_cmd_unmount(BaseSequentialStream *chp, int argc, char *argv[]);

/** 
 * Shell command to list files on card
 */
void sdcard_cmd_ls(BaseSequentialStream *chp, int argc, char *argv[]);

/**
 * Helper used by cmd_ls
 */
FRESULT sdcard_scan_files(BaseSequentialStream *chp, char *path);

/**
 * These callbacks are called by the MMC driver
 *
 * They handle mounting/unmounting the filesystem
 */
void sdcard_insert_handler(eventid_t id);
void sdcard_remove_handler(eventid_t id);

#endif