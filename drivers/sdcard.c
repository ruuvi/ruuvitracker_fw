#include "sdcard.h"
#include "chprintf.h"
#include <stdio.h>
#include <string.h>
#include "drivers/debug.h"

FATFS MMC_FS;
MMCDriver MMCD1;
static bool_t fs_ready = FALSE;
static SPIConfig hs_spicfg = { NULL, GPIOB, 15, 0 };
static SPIConfig ls_spicfg = { NULL, GPIOB, 15, SPI_CR1_BR_2 | SPI_CR1_BR_1 };
static MMCConfig mmc_cfg = { &SPID1, &ls_spicfg, &hs_spicfg };

// where these go ? mmc_is_protected, mmc_is_inserted


void sdcard_enable(void)
{
    D_ENTER();
    power_request(SDCARD_POWER_DOMAIN);
    // Wait for the regulator to stabilize
    chThdSleepMilliseconds(100);
    sdcard_mmcd_init();
    D_EXIT();
}

void sdcard_disable(void)
{
    D_ENTER();
    if (sdcard_fs_ready())
    {
        sdcard_unmount();
    }
    power_release(SDCARD_POWER_DOMAIN);
    D_EXIT();
}

bool_t sdcard_fs_ready(void)
{
    return fs_ready;
}

FRESULT sdcard_unmount(void)
{
    D_ENTER();
    FRESULT err;
    err = f_mount(0, NULL);
    mmcDisconnect(&MMCD1);
    fs_ready = FALSE;
    D_EXIT();
    return err;
}

FRESULT sdcard_mount(void)
{
    D_ENTER();
    FRESULT err;
    if (!mmc_lld_is_card_inserted(&MMCD1))
    {
        _DEBUG("SD: No card detected!\r\n");
        D_EXIT();
        return FR_NOT_READY;
    }
    _DEBUG("SD: mmcConnect\r\n");
    chThdSleepMilliseconds(100);
    if(mmcConnect(&MMCD1))
    {
        _DEBUG("SD: Failed to connect to card\r\n");
        D_EXIT();
        return FR_NOT_READY;
    }
    else
    {
        _DEBUG("SD: Connected to card\r\n");
    }
    
    _DEBUG("SD: f_mount\r\n");
    chThdSleepMilliseconds(100);
    err = f_mount(0, &MMC_FS);
    if(err != FR_OK)
    {
        _DEBUG("SD: f_mount() failed %d\r\n", err);
        mmcDisconnect(&MMCD1);
        D_EXIT();
        return err;
    }
    else
    {
        _DEBUG("SD: File system mounted\r\n");
    }
    fs_ready = TRUE;
    D_EXIT();
    return err;
}

void sdcard_insert_handler(eventid_t id)
{
    (void)id;
    D_ENTER();
    sdcard_mount();
    D_EXIT();
}

void sdcard_remove_handler(eventid_t id)
{
    (void)id;
    // TODO: Do we need to do something else to unmount the fs ?? (or is it too late)
    sdcard_unmount();
}

void sdcard_mmcd_init(void)
{
    D_ENTER();
    _DEBUG("SD: mmcObjectInit\r\n");
    mmcObjectInit(&MMCD1);
    _DEBUG("SD: mmcStart\r\n");
    mmcStart(&MMCD1, &mmc_cfg);
    _DEBUG("SD: Done\r\n");
    D_EXIT();
}

void sdcard_cmd_enable(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    (void)argc;
    D_ENTER();
    if (sdcard_fs_ready())
    {
        chprintf(chp, "Already mounted (and thus must be enabled)\r\n");
        D_EXIT();
        return;
    }
    chprintf(chp, "Enabling card regulator\r\n");
    sdcard_enable();
}

void sdcard_cmd_mount(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    (void)argc;
    D_ENTER();
    if (sdcard_fs_ready())
    {
        chprintf(chp, "Already mounted\r\n");
        D_EXIT();
        return;
    }
    chprintf(chp, "Enabling card regulator\r\n");
    sdcard_enable();
    chprintf(chp, "Mounting filesystem\r\n");
    sdcard_mount();
    if (!sdcard_fs_ready())
    {
        chprintf(chp, "Mount failed\r\n");
        D_EXIT();
        return;
    }
    chprintf(chp, "Card mounted\r\n");
    D_EXIT();
}

void sdcard_cmd_unmount(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    (void)argc;
    D_ENTER();
    if (!sdcard_fs_ready())
    {
        chprintf(chp, "Already unmounted\r\n");
        D_EXIT();
        return;
    }
    chprintf(chp, "Unmounting filesystem\r\n");
    sdcard_unmount();
    chThdSleepMilliseconds(100);
    chprintf(chp, "Disabling card regulator\r\n");
    sdcard_disable();
    chprintf(chp, "Done\r\n");
    D_EXIT();
}

FRESULT sdcard_scan_files(BaseSequentialStream *chp, char *path)
{
    D_ENTER();
    chprintf(chp, "path=%s\r\n", path);
    chThdSleepMilliseconds(100);
    FRESULT res;
    FILINFO fno;
    DIR dir;
    char new_dir[30]; // This ought to be enough
    bool path_is_root = 0;
    if (strncmp("/", path, 2) == 0)
    {
        path_is_root = 1;
    }
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            // TODO: Size directory vs file ? other info ??
            if (!path_is_root)
            {
                chprintf(chp, "%s/%s\n", path, fn);
            }
            else
            {
                chprintf(chp, "%s\n",fn);
            }
            chThdSleepMilliseconds(100);
            // TODO: Do not recurse
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                chprintf(chp, "^^^ is dir\n");
                chThdSleepMilliseconds(100);
                // If we're listing root, use the dir without path separator
                if (!path_is_root)
                {
                    sprintf(new_dir, "%s/%s", path, fn);
                }
                else
                {
                    sprintf(new_dir, "%s", fn);
                }
                res = sdcard_scan_files(chp, new_dir);
                if (res != FR_OK) break;
            } else {                                       /* It is a file. */
                //chprintf(chp, "%s/%s\n", path, fn);
            }
        }
    }

    D_EXIT();
    return res;
}

void sdcard_cmd_ls(BaseSequentialStream *chp, int argc, char *argv[])
{
    D_ENTER();
    if (!sdcard_fs_ready())
    {
        chprintf(chp, "Not mounted\r\n");
        D_EXIT();
        return;
    }
    if (argc < 1)
    {
        sdcard_scan_files(chp, "/");
    }
    else
    {
        sdcard_scan_files(chp, argv[0]);
    }
    D_EXIT();
}

