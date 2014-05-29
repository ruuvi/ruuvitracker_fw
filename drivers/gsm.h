/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Seppo Takalo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _GSM_H
#define _GSM_H
#include "ch.h"

#define GSM_CMD_LINE_END "\r\n"

enum Power_mode { POWER_OFF=0, POWER_ON, CUT_OFF };
enum Reply { AT_OK=0, AT_FAIL, AT_ERROR, AT_TIMEOUT };

typedef struct 
{
    char number[100]; // PONDER: this is probably way too large
    char msg[161];
    char time[26]; // When receiving
    int  mr; // Message-reference (when sending)
} gsm_sms_t;

extern gsm_sms_t gsm_sms_default_container;

// Event signal flag
extern EventSource gsm_evt_sms_arrived;


/* C-API */
int gsm_cmd(const char *cmd);                                            /* Send AT command to modem. Returns AT_OK, AT_FAIL or AT_ERROR */
int gsm_cmd_fmt(const char *fmt, ...);                                   /* Send formatted string command to modem. */
int gsm_cmd_wait(const char *cmd, const char *response, int timeout);    /* Send string and wait for a specific response */
int gsm_cmd_wait_fmt(const char *response, int timeout, char *fmt, ...); /* Send formatted string and wait for response */
int gsm_wait(const char *pattern, int timeout);                          /* wait for a pattern to appear */
int gsm_wait_cpy(const char *pattern, int timeout, char *buf, unsigned int buf_size); /* Wait and copy rest of the line to a buffer */
void gsm_set_apn(const char *apn);                                       /* Set GPRS access point name. Example "internet" */
void gsm_uart_write(const char *line);                                   /* Send byte-string to GSM serial port */
void gsm_reset_modem(void);                                              /* Reset modem */
int gsm_read_raw(char *buf, int max_len);                                /* Read bytes from serial port */
int gsm_gprs_enable(void);                                               /* Enable GPRS data */
int gsm_gprs_disable(void);                                              /* Disable GPRS data */
void gsm_start(void);                                                    /* Start the modem */
void gsm_stop(void);                                                     /* Stop the modem, leaving power domain on */
void gsm_kill(void);                                                     /* Stop the modem, taking all power out */
int gsm_get_state(void);                                                 /* Read state value */
int gsm_delete_sms(int index);                                           /* Delete SMS in index */
int gsm_send_sms(gsm_sms_t *message);                                    /* Send SMS message */
int gsm_read_sms(int index, gsm_sms_t *message);                         /* Read SMS message from index */


/* Internals */
void gsm_set_power_state(enum Power_mode mode);
void gsm_init(void);
void gsm_toggle_power_pin(void);
int gsm_request_serial_port(void);
void gsm_release_serial_port(void);


#endif	/* GSM_H */
