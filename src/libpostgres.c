#include <libpq-fe.h>
#include "../include/quickjs.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static const char *db_info;

JSValue js_init(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
	JSValue dbinfo = JS_ToString(ctx, argv[0]);
	const char *dbinfo_str = JS_ToCString(ctx, dbinfo);
	db_info = dbinfo_str;
	return JS_NewInt32(ctx, -1);
}

JSValue js_query(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
	JSValue query = JS_ToString(ctx, argv[0]);
	const char *query_str = JS_ToCString(ctx, query);

	PGconn *conn = PQconnectdb(db_info);

	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "Connection failed: %s", PQerrorMessage(conn));
		PQfinish(conn);
		return JS_NewInt32(ctx, -1);
	}

        JSValue array = JS_NewArray(ctx);

	PGresult *res = PQprepare(conn, "", query_str, argc - 1, NULL);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "Prepare failed: %s\n",
			PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(conn);
		return JS_NewInt32(ctx, -1);
	}

	const char *params[argc - 1];
	for(int i = 1; i < argc; i++) {
		JSValue query = JS_ToString(ctx, argv[i]);
		const char *arg_str = JS_ToCString(ctx, query);
		params[i-1] = arg_str;
	}

	printf("prep statement ezpz");

	res = PQexecPrepared(conn, "", argc - 1, params, NULL, NULL, 0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(conn);
		return JS_NewInt32(ctx, -1);
        }

	if (PQresultStatus(res) == PGRES_TUPLES_OK) {
	        int rows = PQntuples(res);
	        int cols = PQnfields(res);

	        for (int row = 0; row < rows; row++) {
			JSValue record = JS_NewObject(ctx);
	                for (int col = 0; col < cols; col++) {
				const char *name = PQfname(res, col);
				JS_SetPropertyStr(ctx, record, name, JS_NewString(ctx, PQgetvalue(res, row, col)));
	                }
		        JS_SetPropertyUint32(ctx, array, row, record);
	        }
	}

	PQclear(res);
        PQfinish(conn);
	return array;
}

static const JSCFunctionListEntry js_server_funcs[] = {
    JS_CFUNC_DEF("init_postgres", 1, js_init),
    JS_CFUNC_DEF("query", 1, js_query)
};

static int js_server_init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, js_server_funcs,
                                  countof(js_server_funcs));
}

JSModuleDef *js_init_module_postgres(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_server_init);
    if (!m)
        return NULL;
    JS_AddModuleExportList(ctx, m, js_server_funcs, countof(js_server_funcs));
    return m;
}
