#include "../include/http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *serialize(struct http_response *res, size_t *response_len) {
        size_t len = 0, pos = 0;
        int n;
        char *raw;

        if (!res || !response_len) {
            return NULL;
	}

	if(res->body != NULL) {
		res->body_len = strlen(res->body);
	}

        *response_len = 0;

        n = snprintf(NULL, 0, "HTTP/1.1 %d %s\r\n", res->status, res->reason ? res->reason : "");

        if (n < 0) {
            return NULL;
	}

        len += (size_t) n;

        for (size_t i = 0; i < res->num_headers; i++) {
            n = snprintf(NULL, 0, "%s: %s\r\n", res->headers[i].name, res->headers[i].value);

            if (n < 0) {
                return NULL;
	    }

            len += (size_t) n;
        }

        n = snprintf(NULL, 0, "Content-Length: %zu\r\n", res->body_len);

        if (n < 0) {
            return NULL;
	}

        len += (size_t) n;
        len += 2;
        len += res->body_len;

        raw = malloc(sizeof(char) * len + 1);
        if (!raw) {
            return NULL;
	}

        n = snprintf(raw + pos, len + 1 - pos, "HTTP/1.1 %d %s\r\n", res->status, res->reason ? res->reason : "");

        if (n < 0) {
            goto error;
	}

        pos += (size_t) n;

        for (size_t i = 0; i < res->num_headers; i++) {
            n = snprintf(raw + pos, len + 1 - pos, "%s: %s\r\n", res->headers[i].name, res->headers[i].value);

            if (n < 0) {
                goto error;
	    }

            pos += (size_t) n;
        }

        n = snprintf(raw + pos, len + 1 - pos, "Content-Length: %zu\r\n", res->body_len);

        if (n < 0)
            goto error;

        pos += (size_t)n;

        raw[pos++] = '\r';
        raw[pos++] = '\n';

        if (res->body_len > 0 && res->body) {
            memcpy(raw + pos, res->body, res->body_len);
	}

        pos += res->body_len;

        raw[pos] = '\0';
        *response_len = pos;
        return raw;

    error:
        free(raw);
        return NULL;
}
