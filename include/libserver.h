#include "quickjs.h"
#include <pthread.h>

struct path_to_cb {
	const char *path;
	JSValue cb;
};

JSModuleDef *js_init_module_server(JSContext *ctx, const char *module_name);
