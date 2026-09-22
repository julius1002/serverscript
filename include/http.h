#include <unistd.h>
#include <string.h>
#include <pthread.h>

struct header {
	const char *name;
	const char *value;
};

struct http_response {
        int status;
        const char *reason;
        size_t num_headers;
	struct header headers[50];
        const char *body;
        size_t body_len;
};

struct http_request {
	const struct mg_request_info *info;
	char *body;
	size_t body_len;
};

struct http_pair {
	struct http_request req;
	struct http_response res;
};

enum event_type {
	HTTP_REQUEST,
	SHUTDOWN
};

struct event {
	enum event_type type;
	void *data;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int done;
};

char *serialize(struct http_response *res, size_t *raw_len);

void init_default_response(struct http_response *res);
