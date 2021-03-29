#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>


struct kdb_s {
	char *name;
	char *sofile;
	char *options;
	uint8_t major;
	uint8_t minor;
	void *dlhan; // dlopen() handler
	void *init_fn;
	void *fini_fn;
	void *load_fn;
	void *deploy_fn;
	void *udata; // User data
};


// Simple methods

// Name
KGET_STR(db, name);

// ID
KGET_STR(db, id);
KSET_STR(db, id);

// File
KGET_STR(db, file);
KSET_STR(db, file);

// Conf
KGET_STR(db, conf);
KSET_STR(db, conf);

// Version major number
KGET(db, uint8_t, major);
KSET(db, uint8_t, major);

// Version minor number
KGET(db, uint8_t, minor);
KSET(db, uint8_t, minor);

// User data
KGET(db, void *, udata);
KSET(db, void *, udata);

// COMMAND list
static KCMP_NESTED(db, sym, name);
static KCMP_NESTED_BY_KEY(db, sym, name);
KADD_NESTED(db, sym);
KFIND_NESTED(db, sym);
KNESTED_LEN(db, sym);
KNESTED_ITER(db, sym);
KNESTED_EACH(db, sym);


kdb_t *kdb_new(const char *name)
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
	db->id = NULL;
	db->file = NULL;
	db->conf = NULL;
	db->major = 0;
	db->minor = 0;
	db->dlhan = NULL;
	db->init_fn = NULL;
	db->fini_fn = NULL;
	db->udata = NULL;

	// SYM list
	db->syms = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kdb_sym_compare, kdb_sym_kcompare,
		(void (*)(void *))ksym_free);
	assert(db->syms);

	return db;
}


void kdb_free(kdb_t *db)
{
	if (!db)
		return;

	faux_str_free(db->name);
	faux_str_free(db->id);
	faux_str_free(db->file);
	faux_str_free(db->conf);
	faux_list_free(db->syms);

	if (db->dlhan)
		dlclose(db->dlhan);

	faux_free(db);
}


bool_t kdb_load(kdb_t *db)
{
	char *file_name = NULL;
	char *init_name = NULL;
	char *fini_name = NULL;
	char *major_name = NULL;
	char *minor_name = NULL;
	int flag = RTLD_NOW | RTLD_LOCAL;
	const char *id = NULL;
	bool_t retcode = BOOL_FALSE;
	uint8_t *ver = NULL;

	assert(db);
	if (!db)
		return BOOL_FALSE;

	if (kdb_id(db))
		id = kdb_id(db);
	else
		id = kdb_name(db);

	// Shared object file name
	if (kdb_file(db))
		file_name = faux_str_dup(kdb_file(db));
	else
		file_name = faux_str_sprintf(Kdb_SONAME_FMT, id);

	// Symbol names
	major_name = faux_str_sprintf(Kdb_MAJOR_FMT, id);
	minor_name = faux_str_sprintf(Kdb_MINOR_FMT, id);
	init_name = faux_str_sprintf(Kdb_INIT_FMT, id);
	fini_name = faux_str_sprintf(Kdb_FINI_FMT, id);

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
	if (!db->init_fn) {
//		fprintf(stderr, "Error: Can't get db \"%s\" init function: %s\n",
//			this->name, dlerror());
		goto err;
	}

	// Get db fini function
	db->fini_fn = dlsym(db->dlhan, fini_name);

	retcode = BOOL_TRUE;
err:
	faux_str_free(file_name);
	faux_str_free(major_name);
	faux_str_free(minor_name);
	faux_str_free(init_name);
	faux_str_free(fini_name);

	if (!retcode && db->dlhan)
		dlclose(db->dlhan);

	return retcode;
}
