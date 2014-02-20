/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */
#ifndef _GSM_H
#define _GSM_H

#define GSM_CMD_LINE_END "\r\n"

enum Power_mode { POWER_OFF=0, POWER_ON, CUT_OFF };
enum Reply { AT_OK=0, AT_FAIL, AT_ERROR, AT_TIMEOUT };

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

/* Internals */
void gsm_set_power_state(enum Power_mode mode);
void gsm_init(void);
void gsm_toggle_power_pin(void);
int gsm_request_serial_port(void);
void gsm_release_serial_port(void);


#endif	/* GSM_H */
