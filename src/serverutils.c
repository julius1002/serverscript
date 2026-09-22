#include "../include/serverutils.h"
#include <stdlib.h>
#include "../deps/civetweb-1.16/include/civetweb.h"
#include "../deps/blocking-queue/blocking_queue.h"


JSClassID res_class_id;

JSClassID req_class_id;

static JSValue req_headers_to_js(JSContext *ctx, struct http_request *req) {
    JSValue array = JS_NewArray(ctx);
    JSValue header_obj;

    if(req->info->num_headers > 0) {
	    for(int i = 0; i < req->info->num_headers; i++) {
		    header_obj = JS_NewObject(ctx);
		    JS_SetPropertyStr(ctx, header_obj, "name", JS_NewString(ctx, req->info->http_headers[i].name));

		    JS_SetPropertyStr(ctx, header_obj, "value", JS_NewString(ctx, req->info->http_headers[i].value));

		    JS_SetPropertyUint32(ctx, array, i, header_obj);
	    }
    }
    return array;
}


JSValue req_to_js(JSContext *ctx, struct http_request *req)
{
        JSValue obj = JS_NewObjectClass(ctx, req_class_id);
    
        if (JS_IsException(obj)) {
            return obj;
        }
    
        if(req->body != NULL) {
    	    JS_SetPropertyStr(ctx, obj, "body", JS_NewString(ctx, req->body));
        }
    
        if(req->info->request_method != NULL) {
		JS_SetPropertyStr(ctx, obj, "method", JS_NewString(ctx, req->info->request_method));
        }
    
        if(req->info->local_uri != NULL) {
		JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, req->info->local_uri));
        }
    
        JSValue array = req_headers_to_js(ctx, req);
    
        JS_SetPropertyStr(ctx, obj, "headers", array);
        JS_SetOpaque(obj, req);
    
        return obj;
}

JSValue res_to_js(JSContext *ctx, struct http_response *res)
{
    JSValue obj = JS_NewObjectClass(ctx, res_class_id);

    if (JS_IsException(obj)) {
        return obj;
    }

    JS_SetOpaque(obj, res);

    return obj;
}

void res_headers_to_c(JSContext *ctx, JSValue *argv, struct http_pair *pair) {
	JSValue headers = JS_GetPropertyStr(ctx, argv[1], "headers");
	if (JS_IsArray(headers)) {
		    int64_t len;

		    JS_GetLength(ctx, headers, &len);
		    pair->res.num_headers = len;

		    for (uint32_t i = 0; i < len; i++) {
			    JSValue item = JS_GetPropertyUint32(ctx, headers, i);
			    JSValue name = JS_GetPropertyStr(ctx, item, "name");
			    const char *name_str = JS_ToCString(ctx, name);

			    JSValue value = JS_GetPropertyStr(ctx, item, "value");
			    const char *value_str = JS_ToCString(ctx, value);

			    printf("header %d: name: %s, value: %s\n", i, name_str, value_str);
			    pair->res.headers[i].name = name_str;
			    pair->res.headers[i].value = value_str;
	            }
	}
}

void res_values_to_c(JSContext *ctx, JSValue argv[2], struct http_pair *pair) {
	// body
	JSValue js_body = JS_GetPropertyStr(ctx, argv[1], "body");
	const char *body_str = JS_ToCString(ctx, js_body);

	//JS_FreeValue(ctx, body);

	// setting the values at the response before the server sends it
	pair->res.body = body_str;

	// status
	int status;
	JSValue js_status = JS_GetPropertyStr(ctx, argv[1], "status");
        if (!JS_ToInt32(ctx, &status, js_status)) {
		pair->res.status = (status <= 0 ? 200 : status);
	}

	// status
	const char *reason = NULL;
	JSValue js_reason = JS_GetPropertyStr(ctx, argv[1], "reason");
	reason = JS_ToCString(ctx, js_reason);

	pair->res.reason = reason;

	// headers
	res_headers_to_c(ctx, argv, pair);
}


void request_routine(struct http_request *req, struct http_response *res) {
	struct event *ev = malloc(sizeof(struct event));
	ev->type = HTTP_REQUEST;
	struct http_pair *pair = malloc(sizeof(struct http_pair));
	pair->req = *req;
	pair->res = *res;
	ev->data = pair;
	ev->done = 0;

        if(0 > pthread_cond_init(&ev->cond, NULL) ) {
		perror("pthread_cond_init");
		exit(1);
	}
        if(0 > pthread_mutex_init(&ev->mutex, NULL) ) {
		perror("pthread_mutex_init");
		exit(1);
	}

	if(0 > pthread_mutex_lock(&ev->mutex)) {
		perror("pthread_cond_wait");
		exit(1);
	}

	enqueue(ev);

	while(!ev->done) {
		if(0 > pthread_cond_wait(&ev->cond, &ev->mutex)) {
			perror("pthread_cond_wait");
			exit(1);
		}
	}

	if(0 > pthread_mutex_unlock(&ev->mutex)) {
		perror("pthread_cond_wait");
		exit(1);
	}

	pthread_mutex_destroy(&ev->mutex);
	pthread_cond_destroy(&ev->cond);

	free(ev);
}

