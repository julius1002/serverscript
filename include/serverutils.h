#include "quickjs.h"
#include "http.h"

void res_headers_to_c(JSContext *ctx, JSValue *argv, struct http_pair *pair);

void res_values_to_c(JSContext *ctx, JSValue argv[2], struct http_pair *pair);

void handle_http_event(struct event *ev, JSContext *ctx);

void request_routine(struct http_request *req, struct http_response *res);

JSValue res_to_js(JSContext *ctx, struct http_response *res);

JSValue req_to_js(JSContext *ctx, struct http_request *req);
