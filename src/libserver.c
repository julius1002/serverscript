#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include "../deps/blocking-queue/blocking_queue.h"
#include "../include/quickjs.h"
#include "../deps/civetweb-1.16/include/civetweb.h"
#include "../include/serverutils.h"
#include "../include/utils.h"
#include <sys/types.h>
#include "../include/libserver.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static volatile int running = 1;

#define MAX_NUM_PATH_TO_CB 100

static struct path_to_cb pathcbs[MAX_NUM_PATH_TO_CB];

static size_t num_path_to_cbs = 0;

static void handle_signal(int sig)
{
	(void)sig;
	running = 0;

	struct event *ev = malloc(sizeof(struct event));
	ev->type = SHUTDOWN;
	enqueue(ev);
}

static JSValue js_init(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
	JSValue js_port, js_www_loc;

	if(argc > 2) {
		return JS_NewInt32(ctx, 1);
	}

	js_port = argv[0];
	const char *port = JS_ToCString(ctx, js_port);

	if(!port) {
		return JS_NewInt32(ctx, -1);
	}

	js_www_loc = argv[1];
	const char *www_loc = JS_ToCString(ctx, js_www_loc);

	if(!www_loc) {
		return JS_NewInt32(ctx, -1);
	}

	struct mg_context *server;

	const char *options[] = {
		"document_root", www_loc,
		"listening_ports", port,
		NULL
	};

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	init_blocking_queue(100);

	server = mg_start(NULL, NULL, options);
	if (server == NULL) {
		fprintf(stderr, "Failed to start CivetWeb\n");
		return JS_NewInt32(ctx, 1);
	}

	JS_SetContextOpaque(ctx, server);

	return JS_NewInt32(ctx, 0);
}

// this is the function to handle http_event in our event loop. 
// It is handled by the runtime, in a way that it calls the callbackand we have no thread safety issues.
void handle_http_event(struct event *ev, JSContext *ctx) {
	struct http_pair *pair;
	struct path_to_cb pathcb;
	int argc;
	JSValue argv[2];
	JSValue result;

	pair = (struct http_pair *) ev->data;

	for(int i = 0; i < MAX_NUM_PATH_TO_CB; i++) {
		pathcb = pathcbs[i];
		if(pathcb.path != NULL && 
		     pair->req.info->local_uri != NULL &&
				!strncmp(pathcb.path, pair->req.info->local_uri, strlen(pathcb.path))) {

			argc = 2;
			argv[0] = req_to_js(ctx, &pair->req);
			argv[1] = res_to_js(ctx, &pair->res);

			result = JS_Call(ctx, pathcb.cb, JS_UNDEFINED, argc, argv);

			res_values_to_c(ctx, argv, pair);

			JS_FreeValue(ctx, result);

			break;
		}
	}
	if(0 > pthread_mutex_lock(&ev->mutex)) {
		perror("pthread_mutex_lock");
		exit(1);
	}

	ev->done = 1;

	if(0 > pthread_cond_signal(&ev->cond)) {
		perror("pthread_cond_signal");
		exit(1);
	}

	if(0 > pthread_mutex_unlock(&ev->mutex)) {
		perror("pthread_mutex_unlock");
		exit(1);
	}
}

void loop_events(JSContext *ctx)
{
	while(running) {
		struct bq_entry *entr = dequeue();
		struct event *ev = (struct event *) entr->data;
		switch(ev->type) {
			case HTTP_REQUEST: {
						   handle_http_event(ev, ctx);
						   break;
					   }
                        case SHUTDOWN: {
					       free(ev);
					       break;
				       }
		}
		free(entr);
	}

	struct mg_context *server = JS_GetContextOpaque(ctx);
	mg_stop(server);

}


static JSValue js_launch(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
	loop_events(ctx);
	return JS_NewInt32(ctx, 0);
}

/*
 * Here is usually the business logic happening. 
 *
 * What we do is, we prepare the request object and pass it along with other data to a queue.
 *
 * We do this to circumvent that we only use a single qjs runtime thread.
 *
 * This runtime thread calls our callback.
 *
 * This handler waits for the callback to be called in order to return a proper result to the client.
 */
static int request_handler(struct mg_connection *conn, void *cbdata)
{
	(void)cbdata;

	const char *hdr = mg_get_header(conn, "Content-Length");
	char *content = NULL;
	int content_len = 0;

	if(hdr) {
		content_len = atoi(hdr);
		content = malloc(sizeof(char *) * content_len + 1);
		mg_read(conn, (void *) content, content_len);
	}

	const struct mg_request_info *req = mg_get_request_info(conn);

	struct event *ev = malloc(sizeof(struct event));
	ev->type = HTTP_REQUEST;
	struct http_pair *pair = malloc(sizeof(struct http_pair));
	pair->req.info = req;
	pair->req.body = content;
	pair->req.body_len = content_len;
	ev->data = pair;

	init_lock_mutex_cond(&ev->mutex, &ev->cond);

	pair->res.status = 200;
	pair->res.reason = "OK";

	enqueue(ev);

	wait_for_cb_invoke(&ev->mutex, &ev->cond, &ev->done);

	size_t outlen;
	struct http_response *res = &pair->res;
	char *out = serialize(res, &outlen);

	mg_printf(conn, "%s\n", out);

	free(pair);
	free(ev);
	free(out);

	return 200;
}

static JSValue js_add_mapping(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
	if(argc < 2) {
		return JS_NewInt32(ctx, 1);
	}

	JSValue path = JS_ToString(ctx, argv[0]);
	const char *pathstr = JS_ToCString(ctx, path);

	if (!pathstr) {
		return JS_NewInt32(ctx, -1);
	}

	if (!JS_IsFunction(ctx, argv[1])) {
		return JS_ThrowTypeError(ctx, "expected a function");
	}

	pathcbs[num_path_to_cbs].path = pathstr;
	pathcbs[num_path_to_cbs++].cb = argv[1];

	struct mg_context *server = JS_GetContextOpaque(ctx);
	mg_set_request_handler(server, pathstr, request_handler, NULL);

	return JS_NewInt32(ctx, 0);
}

static const JSCFunctionListEntry js_server_funcs[] = {
	JS_CFUNC_DEF("init_server", 2, js_init),
	JS_CFUNC_DEF("launch_server", 0, js_launch),
	JS_CFUNC_DEF("add_mapping", 2, js_add_mapping)
};

static int js_server_init(JSContext *ctx, JSModuleDef *m)
{
	return JS_SetModuleExportList(ctx, m, js_server_funcs,
			countof(js_server_funcs));
}

JSModuleDef *js_init_module_server(JSContext *ctx, const char *module_name)
{
	JSModuleDef *m;
	m = JS_NewCModule(ctx, module_name, js_server_init);
	if (!m)
		return NULL;
	JS_AddModuleExportList(ctx, m, js_server_funcs, countof(js_server_funcs));
	return m;
}
