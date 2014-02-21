#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ch.h"
#include "hal.h"
#include "http.h"
#include "gsm.h"
#include "debug.h"

#define TIMEOUT_MS 2000
#define TIMEOUT_HTTP 30000

/* HTTP FUNCTIONS */

typedef enum method_t { GET, POST } method_t; /* HTTP methods */

static int gsm_http_init(const char *url)
{
    int ret;

    if (AT_OK != gsm_gprs_enable())
        return -1;

    ret = gsm_cmd("AT+HTTPINIT");
    if (ret != AT_OK)
        return -1;
    ret = gsm_cmd("AT+HTTPPARA=\"CID\",\"1\"");
    if (ret != AT_OK)
        return -1;
    ret = gsm_cmd_fmt("AT+HTTPPARA=\"URL\",\"%s\"", url);
    if (ret != AT_OK)
        return -1;
    ret = gsm_cmd("AT+HTTPPARA=\"UA\",\"RuuviTracker/SIM968\"");
    if (ret != AT_OK)
        return -1;
    ret = gsm_cmd("AT+HTTPPARA=\"REDIR\",\"1\"");
    if (ret != AT_OK)
        return -1;
    ret = gsm_cmd("AT+HTTPPARA=\"TIMEOUT\",\"30\"");
    if (ret != AT_OK)
        return -1;
    return 0;
}

static void gsm_http_clean(void)
{
    if (AT_OK != gsm_cmd("AT+HTTPTERM")) {
        gsm_gprs_disable();         /* This should get it to respond */
    }
}

static int gsm_http_send_content_type(const char *content_type)
{
    return gsm_cmd_fmt("AT+HTTPPARA=\"CONTENT\",\"%s\"", content_type);
}

static int gsm_http_send_data(const char *data)
{
    int r;
    r = gsm_cmd_wait_fmt("DOWNLOAD", TIMEOUT_MS, "AT+HTTPDATA=%d,1000", strlen(data));
    if (AT_OK != r) {
        return r;
    }
    gsm_uart_write(data);
    return gsm_cmd(GSM_CMD_LINE_END);
}

static HTTP_Response *gsm_http_handle(method_t method, const char *url,
                                      const char *data, const char *content_type)
{
    int status=0,len,ret;
    char resp[64];
    HTTP_Response *response = NULL;

    if (gsm_http_init(url) != 0) {
        _DEBUG("%s","GSM: Failed to initialize HTTP\r\n");
        goto HTTP_END;
    }

    if (content_type) {
        if (gsm_http_send_content_type(content_type) != 0) {
            _DEBUG("%s", "GSM: Failed to set content type\r\n");
            goto HTTP_END;
        }
    }

    if (data) {
        if (gsm_http_send_data(data) != 0) {
            _DEBUG("%s", "GSM: Failed to send http data\r\n");
            goto HTTP_END;
        }
    }

    if (GET == method)
        ret = gsm_cmd("AT+HTTPACTION=0");
    else
        ret = gsm_cmd("AT+HTTPACTION=1");
    if (ret != AT_OK) {
        _DEBUG("GSM: HTTP Action failed\r\n");
        goto HTTP_END;
    }

    //block others from reading.
    gsm_request_serial_port();

    if (gsm_wait_cpy("\\+HTTPACTION", TIMEOUT_HTTP, resp, sizeof(resp)) == AT_TIMEOUT) {
        _DEBUG("GSM: HTTP Timeout\r\n");
        goto HTTP_END;
    }

    if (2 != sscanf(resp, "+HTTPACTION:%*d,%d,%d", &status, &len)) { /* +HTTPACTION:<method>,<result>,<lenght of data> */
        _DEBUG("GSM: Failed to parse response\r\n");
        goto HTTP_END;
    }
    _DEBUG("HTTP response %d len=%d\r\n", status, len);

    /* Continue to read response, if there is any */
    if (len <= 0)
        goto HTTP_END;

    if (NULL == (response = malloc(len+1+sizeof(HTTP_Response)))) {
        _DEBUG("GSM: Out of memory\r\n");
        status = 602;               /* HTTP: 602 = Out of memory */
        goto HTTP_END;
    }

    gsm_uart_write("AT+HTTPREAD" GSM_CMD_LINE_END); //Read cmd
    /* TODO: Implement these without platform specific serial port assumption */
    while ('+' != sdGet(&SD3)) /* Skip +HTTPREAD: ... */
        ;;
    while ('\n' != sdGet(&SD3)) /* To the end of line */
        ;;
    /* Rest of bytes are data */
    if (len != gsm_read_raw(response->content, len)) {
        _DEBUG("GSM: Timeout waiting for HTTP DATA\r\n");
        http_free(response);
        response = NULL;
        goto HTTP_END;
    }
    response->code = status;
    response->content_len = len;
    response->content[len]=0; // Mark end, so string functions can be used
    _DEBUG("HTTP: Got %d of data\r\n", len);

HTTP_END:
    gsm_release_serial_port();
    gsm_http_clean();

    return response;
}

/* ======== API ============== */

HTTP_Response *http_get(const char *url)
{
    return gsm_http_handle(GET, url, NULL, NULL);
}

HTTP_Response *http_post(const char *url, const char *data, const char *content_type)
{
    return gsm_http_handle(POST, url, data, content_type);
}

void http_free(HTTP_Response *response)
{
    if (!response)
        return;
    free(response);
}
