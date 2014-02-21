#ifndef _HTTP_H
#define _HTTP_H

typedef struct HTTP_Response_t {
    int code;
    unsigned int content_len;
    char content[0];
} HTTP_Response;

HTTP_Response *http_get(const char *url);
HTTP_Response *http_post(const char *url, const char *data, const char *content_type);
void http_free(HTTP_Response *response);

#endif
