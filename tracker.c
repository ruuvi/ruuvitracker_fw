#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ch.h"
#include "hal.h"
#include "test.h"

#include "shell.h"
#include "chprintf.h"
#include "power.h"
#include "drivers/usb_serial.h"
#include "drivers/gps.h"
#include "drivers/gsm.h"
#include "drivers/http.h"
#include "drivers/sha1.h"
#include "drivers/debug.h"
#include "drivers/reset_button.h"


#define SHA1_STR_LEN	40
#define SHA1_LEN	20
#define BLOCKSIZE	64
#define IPAD 0x36
#define OPAD 0x5C

/* Helper to get hash from library */
static void get_hash(SHA1Context *sha, char *p)
{
	int i;
	for(i=0; i<5; i++) {
		*p++ = (sha->Message_Digest[i]>>24)&0xff;
		*p++ = (sha->Message_Digest[i]>>16)&0xff;
		*p++ = (sha->Message_Digest[i]>>8)&0xff;
		*p++ = (sha->Message_Digest[i]>>0)&0xff;
	}
}

static char *sha1_hmac(const char *secret, const char *msg)
{
	static char response[SHA1_STR_LEN];
	SHA1Context sha1,sha2;
	char key[BLOCKSIZE];
	char hash[SHA1_LEN];
	int i;
	size_t len;

	len = strlen(secret);

	//Fill with zeroes
	memset(key, 0, BLOCKSIZE);

	if (len > BLOCKSIZE) {
		//Too long key, shorten with hash
		SHA1Reset(&sha1);
		SHA1Input(&sha1, (const unsigned char *)secret, len);
		SHA1Result(&sha1);
		get_hash(&sha1, key);
	} else {
		memcpy(key, secret, len);
	}

	//XOR key with IPAD
	for (i=0; i<BLOCKSIZE; i++) {
		key[i] ^= IPAD;
	}

	//First SHA hash
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha1, (const unsigned char *)msg, strlen(msg));
	SHA1Result(&sha1);
	get_hash(&sha1, hash);

	//XOR key with OPAD
	for (i=0; i<BLOCKSIZE; i++) {
		key[i] ^= IPAD ^ OPAD;
	}

	//Second hash
	SHA1Reset(&sha2);
	SHA1Input(&sha2, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha2, (const unsigned char *)hash, SHA1_LEN);
	SHA1Result(&sha2);

	for(i = 0; i < 5 ; i++) {
		sprintf(response+i*8,"%.8x", sha2.Message_Digest[i]);
	}
	return response;
}

struct json_t {
     char *name;
     char *value;
};

#define ELEMS_IN_EVENT 6
#define MAC_INDEX 6
struct json_t js_elems[ELEMS_IN_EVENT + 1] = {
     /* Names must be in alphabethical order */
     { "latitude",      NULL },
     { "longitude",     NULL },
     { "session_code",  NULL },
     { "time",          NULL },
     { "tracker_code",  "sepeto" },
     { "version",       "1" },
     /* Except "mac" that must be last element */
     { "mac",           NULL },
};

/* Replace elements value */
void js_replace(char *name, char *value)
{
     int i;
     for (i=0;i<ELEMS_IN_EVENT;i++) {
	  if (0 == strcmp(name, js_elems[i].name)) {
	       if (js_elems[i].value)
		    free(js_elems[i].value);
	       js_elems[i].value = value;
	       break;
	  }
     }
}

void calculate_mac(char *secret)
{
     static char str[4096]; //Assume that it fits
     int i;
     str[0] = 0;
     for (i=0;i<ELEMS_IN_EVENT;i++) {
	  strcat(str, js_elems[i].name);
	  strcat(str, ":");
	  strcat(str, js_elems[i].value);
	  strcat(str, "|");
     }
	 js_elems[MAC_INDEX].value = sha1_hmac(secret, str);
     _DEBUG("Calculated MAC %s\r\nSTR: %s\r\n", js_elems[MAC_INDEX].value, str);
}

char *js_tostr(void)
{
     int i;
     static char str[4096]; //Again..assume.. when we crash, fix this.
     str[0] = '{';
     str[1] = 0;
     for (i=0;i<ELEMS_IN_EVENT+1;i++) {
	  if (i)
	       strcat(str, ",");
	  strcat(str, "\"");
	  strcat(str, js_elems[i].name);
	  strcat(str, "\": \"");
	  strcat(str, js_elems[i].value);
	  strcat(str, "\" ");
     }
     strcat(str, "}");
     return str;
}

static void send_event(struct gps_data_t *gps)
{
     static char buf[255];
     HTTP_Response *response;
     char *json;
     static int first_time = 1;

     sprintf(buf, "%f", (float)gps->lat);
     js_replace("latitude", strdup(buf));
     sprintf(buf, "%f", (float)gps->lon);
     js_replace("longitude", strdup(buf));
     sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
                     gps->dt.year, gps->dt.month, gps->dt.day, gps->dt.hh, gps->dt.mm, gps->dt.sec);
     js_replace("time", strdup(buf));
     if (first_time) {
	  js_replace("session_code", strdup(buf));
	  first_time = 0;
     }
     calculate_mac("sepeto");
     json = js_tostr();

     _DEBUG("Sending JSON event:\r\n%s\r\nlen = %d\r\n", json, strlen(json));
     response = http_post("http://dev-server.ruuvitracker.fi/api/v1-dev/events", json, "application/json");
     if (!response) {
	  _DEBUG("HTTP POST failed\r\n");
     } else {
	  _DEBUG("HTTP response %d:\r\n%s\r\n", response->code, response->content);
	  http_free(response);
     }
}

static WORKING_AREA(myWA, 4096);
__attribute__((noreturn))
static void tracker_th(void *args)
{
     struct gps_data_t gps;
     (void)args;
     while (TRUE) {
	  chThdSleepMilliseconds(5000);
	  /* Wait for fix */
	  while (GPS_FIX_TYPE_NONE == gps_has_fix())
	       chThdSleepMilliseconds(1000);
	  gps = gps_get_data_nonblock();
	  send_event(&gps);
     }
}

int main(void)
{
     halInit();
     chSysInit();
     usb_serial_init();
	 button_init();
     gsm_start();
     gsm_set_apn("internet.saunalahti");
     gps_start();
     chThdCreateStatic(myWA, sizeof(myWA),
                          NORMALPRIO, (tfunc_t)tracker_th, NULL);
     while (TRUE) {
	  chThdSleepMilliseconds(1000);
     }
}
