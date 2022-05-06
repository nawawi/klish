#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

#include <klish/kplugin.h>
#include <klish/kcontext.h>
#include <faux/ini.h>
#include <faux/str.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua-compat.h"

#define LUA_CONTEXT "klish_context"
#define LUA_AUTORUN_SW "autostart"
#define LUA_BACKTRACE_SW "backtrace"
#define LUA_PACKAGE_PATH_SW "package.path"


const uint8_t kplugin_lua_major = KPLUGIN_MAJOR;
const uint8_t kplugin_lua_minor = KPLUGIN_MINOR;


struct lua_klish_data {
	lua_State *L;
	kcontext_t *context;
	char *package_path_sw;
	char *autorun_path_sw;
	int backtrace_sw; // show traceback
};

static lua_State *globalL = NULL;

static int luaB_par(lua_State *L);


static const luaL_Reg klish_lib[] = {
        { "par", luaB_par },
        { NULL, NULL }
};

#if LUA_VERSION_NUM >= 502
static int traceback (lua_State *L)
{
	const char *msg = lua_tostring(L, 1);

	if (msg)
		luaL_traceback(L, L, msg, 1);
	else if (!lua_isnoneornil(L, 1)) {  // is there an error object?
		if (!luaL_callmeta(L, 1, "__tostring"))  // try its 'tostring' metamethod
		lua_pushliteral(L, "(no error message)");
	}

	return 1;
}
#else
static int traceback (lua_State *L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  // pass error message
	lua_pushinteger(L, 2);  // skip this function and traceback
	lua_call(L, 2, 1);  // call debug.traceback

	return 1;
}
#endif


static int report (lua_State *L, int status)
{
	if (status && !lua_isnil(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL)
			msg = "(error object is not a string)";
		fprintf(stderr,"Error: %s\n", msg);
		lua_pop(L, 1);
		status = -1;
	}

	return status;
}


static int docall(struct lua_klish_data *ctx, int narg)
{
	int status = 0;
	int base = 0;

	if (ctx->backtrace_sw) {
		base = lua_gettop(ctx->L) - narg;  // function index
		lua_pushcfunction(ctx->L, traceback);  // push traceback function
		lua_insert(ctx->L, base);  // put it under chunk and args
	}
	status = lua_pcall(ctx->L, narg, LUA_MULTRET, base);
	if (ctx->backtrace_sw)
		lua_remove(ctx->L, base);  // remove traceback function
	// force a complete garbage collection in case of errors
	if (status != 0)
		lua_gc(ctx->L, LUA_GCCOLLECT, 0);

	return status;
}


static int clear(lua_State *L)
{
	int N = lua_gettop(L);
	lua_pop(L, N);

	return 0;
}


static int loadscript(struct lua_klish_data *ctx, const char *path)
{
	int status = 0;

	status = luaL_loadfile(ctx->L, path);
	if (!status) {
		status = docall(ctx, 0);
	}
	status = report(ctx->L, status);
	clear(ctx->L);

	return status;
}


static int dostring(struct lua_klish_data *ctx, const char *s)
{
	int status = luaL_loadstring(ctx->L, s) || docall(ctx, 0);

	return report(ctx->L, status);
}


static struct lua_klish_data *lua_context(lua_State *L)
{
	struct lua_klish_data *ctx;

	lua_getglobal(L, LUA_CONTEXT);
	ctx = lua_touserdata(L, -1);
	lua_pop(L, 1);

	return ctx;
}


static int luaB_par(lua_State *L)
{
	unsigned int k = 0;
	kcontext_t *context;
	kpargv_t *pars;
	kpargv_pargs_node_t *par_i;
	kparg_t *p = NULL;
	struct lua_klish_data *ctx;

	const char *name = luaL_optstring(L, 1, NULL);

	if (!name)
		lua_newtable(L);
	ctx = lua_context(L);
	assert(ctx);

	context = ctx->context;
	assert(context);

	pars = kcontext_pargv(context);
	if (!pars)
		return (name == NULL);

	par_i = kpargv_pargs_iter(pars);
	if (kpargv_pargs_len(pars) > 0) {
		while ((p = kpargv_pargs_each(&par_i))) {
			const char *n = kentry_name(kparg_entry(p));
			if (!name) {
				lua_pushnumber(L, ++ k);
				lua_pushstring(L, n);
				lua_rawset(L, -3);
				lua_pushstring(L, n);
				lua_pushstring(L, kparg_value(p));
				lua_rawset(L, -3);
			} else if (!strcmp(n, name)) {
				lua_pushstring(L, kparg_value(p));
				return 1;
			}
		}
	}
	return (name == NULL);
}


static int luaopen_klish(lua_State *L)
{
	luaL_newlib(L, klish_lib);
	return 1;
}


static int clish_env(lua_State *L)
{
	luaL_requiref(L, "klish", luaopen_klish, 1);
	return 0;
}


static int package_path(struct lua_klish_data *ctx)
{
	int rc = 0;
	char *str = NULL;
	char *path = ctx->package_path_sw;

	faux_str_cat(&str, "package.path=\"");
	faux_str_cat(&str, path);
	faux_str_cat(&str, "\"");
	rc = dostring(ctx, str);
	clear(ctx->L);
	faux_str_free(str);

	return rc;
}


static void lstop(lua_State *L, lua_Debug *ar)
{
	lua_sethook(L, NULL, 0, 0);
//	luaL_error(L, "interrupted!");
	ar = ar;  // Unused arg
}


static void laction (int i) {
	if (!globalL)
		return;
	lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
	globalL = NULL; // run only once
}


static void locale_set()
{
	setlocale(LC_NUMERIC, "C"); // to avoid . -> , in numbers
	setlocale(LC_CTYPE, "C"); // to avoid lower/upper problems
}


static void locale_reset()
{
	setlocale(LC_NUMERIC, "");
	setlocale(LC_CTYPE, "");
}


static lua_State *lua_init(struct lua_klish_data *ctx)
{
	lua_State *L = NULL;
	locale_set();

#if LUA_VERSION_NUM >= 502
	L = luaL_newstate();
#else
	L = lua_open();
#endif
	if (!L) {
		fprintf(stderr, "Error: Failed to instantiate Lua interpreter\n");
		locale_reset();
		return NULL;
	}
	ctx->L = L; // lua state

	luaL_openlibs(L);

	if (ctx->package_path_sw && package_path(ctx)) {
		fprintf(stderr, "Error: Failed to define package env.\n");
		goto err;
	}

	if (clish_env(L)) {
		fprintf(stderr, "Error: Failed to define Lua clish env.\n");
		goto err;
	}

	if (ctx->autorun_path_sw) {
		if (loadscript(ctx, ctx->autorun_path_sw)) {
			goto err;
		}
	}
	globalL = L;
	locale_reset();

	return L;
err:
	lua_close(L);
	locale_reset();

	return NULL;
}


static int exec_action(struct lua_klish_data *ctx, const char *script)
{
	int rc = 0;
	lua_State *L = ctx->L;

	assert(L);
	globalL = L;

	lua_pushlightuserdata(L, ctx);
	lua_setglobal(L, LUA_CONTEXT);
	locale_set();
	rc = dostring(ctx, script);
	locale_reset();
	fflush(stdout);
	fflush(stderr);
	clear(L);

	return rc;
}


int klish_plugin_lua_action(kcontext_t *context)
{
	int status = -1;
	const char *script = NULL;

	struct sigaction sig_old_int;
	struct sigaction sig_old_quit;
	struct sigaction sig_new;
	sigset_t sig_set;

	const kplugin_t *plugin;
	struct lua_klish_data *ctx;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	ctx = kplugin_udata(plugin);
	assert(ctx);

	script = kcontext_script(context);

	if (!script) // Nothing to do
		return 0;

	sigemptyset(&sig_set);

	sig_new.sa_flags = 0;
	sig_new.sa_mask = sig_set;
	sig_new.sa_handler = laction;
	sigaction(SIGINT, &sig_new, &sig_old_int);
	sigaction(SIGQUIT, &sig_new, &sig_old_quit);

	status = exec_action(ctx, script);
	while ( wait(NULL) >= 0 || errno != ECHILD);

	// Restore SIGINT and SIGQUIT
	sigaction(SIGINT, &sig_old_int, NULL);
	sigaction(SIGQUIT, &sig_old_quit, NULL);
err:
	return status;
}


static void free_ctx(struct lua_klish_data *ctx)
{
	if (ctx->package_path_sw)
		faux_str_free(ctx->package_path_sw);
	if (ctx->autorun_path_sw)
		faux_str_free(ctx->autorun_path_sw);
	free(ctx);
}


int kplugin_lua_init(kcontext_t *context)
{
	faux_ini_t *ini = NULL;
	kplugin_t *plugin = NULL;
	const char *p = NULL;
	struct lua_klish_data *ctx = NULL;
	const char *conf = NULL;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return -1;

	conf = kplugin_conf(plugin);

	ctx->context = context;
	ctx->backtrace_sw = 1;
	ctx->package_path_sw = NULL;
	ctx->autorun_path_sw = NULL;
	ctx->L = NULL;

	if (conf) {
		ini = faux_ini_new();
		faux_ini_parse_str(ini, conf);
		p = faux_ini_find(ini, LUA_BACKTRACE_SW);
		ctx->backtrace_sw = p ? atoi(p) : 1;
		p = faux_ini_find(ini, LUA_PACKAGE_PATH_SW);
		ctx->package_path_sw = p ? faux_str_dup(p): NULL;
		p = faux_ini_find(ini, LUA_AUTORUN_SW);
		ctx->autorun_path_sw = p ? faux_str_dup(p): NULL;
		faux_ini_free(ini);
	}
	kplugin_set_udata(plugin, ctx);
	if (!lua_init(ctx)) {
		free_ctx(ctx);
		return -1;
	}
	kplugin_add_syms(plugin, ksym_new("lua", klish_plugin_lua_action));

	return 0;
}


int kplugin_lua_fini(kcontext_t *context)
{
	kplugin_t *plugin = NULL;
	lua_State *L = NULL;
	struct lua_klish_data *ctx = NULL;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	ctx = kplugin_udata(plugin);
	if (!ctx)
		return 0;
	if (ctx->L)
		lua_close(ctx->L);
	free_ctx(ctx);

	return 0;
}
