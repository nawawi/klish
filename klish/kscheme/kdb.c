#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include <faux/str.h>
#include <faux/ini.h>
#include <klish/khelper.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>


struct kdb_s {
	char *name; // Arbitrary name (can be used to generate SO file name)
	char *file; // SO file name
	faux_ini_t *ini;
	uint8_t major;
	uint8_t minor;
	void *dlhan; // dlopen() handler
	kdb_init_fn init_fn;
	kdb_fini_fn fini_fn;
	kdb_load_fn load_fn;
	kdb_load_fn deploy_fn;
	void *udata; // User data
};


// Simple methods

// Name
KGET_STR(db, name);

// Shared object file
KGET_STR(db, file);

// INI
KGET(db, faux_ini_t *, ini);
KSET(db, faux_ini_t *, ini);

// Version major number
KGET(db, uint8_t, major);
KSET(db, uint8_t, major);

// Version minor number
KGET(db, uint8_t, minor);
KSET(db, uint8_t, minor);

// User data
KGET(db, void *, udata);
KSET(db, void *, udata);


kdb_t *kdb_new(const char *name, const char *file)
{
	kdb_t *db = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	db = faux_zmalloc(sizeof(*db));
	assert(db);
	if (!db)
		return NULL;

	// Initialize
	db->name = faux_str_dup(name);
	db->file = faux_str_dup(file);
	db->ini = NULL;
	db->major = 0;
	db->minor = 0;
	db->dlhan = NULL;
	db->init_fn = NULL;
	db->fini_fn = NULL;
	db->load_fn = NULL;
	db->deploy_fn = NULL;
	db->udata = NULL;

	return db;
}


void kdb_free(kdb_t *db)
{
	if (!db)
		return;

	faux_str_free(db->name);
	faux_str_free(db->file);
	faux_ini_free(db->ini);

	if (db->dlhan)
		dlclose(db->dlhan);

	faux_free(db);
}


bool_t kdb_load(kdb_t *db)
{
	const char *name = NULL;
	char *file_name = NULL;
	char *init_name = NULL;
	char *fini_name = NULL;
	char *load_name = NULL;
	char *deploy_name = NULL;
	char *major_name = NULL;
	char *minor_name = NULL;
	int flag = RTLD_NOW | RTLD_LOCAL;
	bool_t retcode = BOOL_FALSE;
	uint8_t *ver = NULL;

	assert(db);
	if (!db)
		return BOOL_FALSE;

	// Shared object file name
	name = kdb_name(db);
	if (kdb_file(db))
		file_name = faux_str_dup(kdb_file(db));
	else
		file_name = faux_str_sprintf(KDB_SONAME_FMT, name);

	// Symbol names
	major_name = faux_str_sprintf(KDB_MAJOR_FMT, name);
	minor_name = faux_str_sprintf(KDB_MINOR_FMT, name);
	init_name = faux_str_sprintf(KDB_INIT_FMT, name);
	fini_name = faux_str_sprintf(KDB_FINI_FMT, name);
	load_name = faux_str_sprintf(KDB_LOAD_FMT, name);
	deploy_name = faux_str_sprintf(KDB_DEPLOY_FMT, name);

	// Open shared object
	db->dlhan = dlopen(file_name, flag);
	if (!db->dlhan) {
//		fprintf(stderr, "Error: Can't open db \"%s\": %s\n",
//			this->name, dlerror());
		goto err;
	}

	// Get db version
	ver = (uint8_t *)dlsym(db->dlhan, major_name);
	if (!ver)
		goto err;
	kdb_set_major(db, *ver);
	ver = (uint8_t *)dlsym(db->dlhan, minor_name);
	if (!ver)
		goto err;
	kdb_set_minor(db, *ver);

	// Get db init function
	db->init_fn = dlsym(db->dlhan, init_name);
	db->fini_fn = dlsym(db->dlhan, fini_name);
	db->load_fn = dlsym(db->dlhan, load_name);
	db->deploy_fn = dlsym(db->dlhan, deploy_name);
	if (!db->load_fn && !db->deploy_fn) { // Strange DB plugin
//		fprintf(stderr, "Error: DB plugin \"%s\" has no deploy and load functions\n",
//			this->name);
		goto err;
	}

	retcode = BOOL_TRUE;
err:
	faux_str_free(file_name);
	faux_str_free(major_name);
	faux_str_free(minor_name);
	faux_str_free(init_name);
	faux_str_free(fini_name);
	faux_str_free(load_name);
	faux_str_free(deploy_name);

	if (!retcode && db->dlhan)
		dlclose(db->dlhan);

	return retcode;
}
