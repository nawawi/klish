#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <klish/ischeme.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>


uint8_t kdb_ischeme_major = KDB_MAJOR;
uint8_t kdb_ischeme_minor = KDB_MINOR;


bool_t kdb_ischeme_deploy_scheme(kdb_t *db, const kscheme_t *scheme)
{
	faux_ini_t *ini = NULL;
	const char *out_path = NULL;
	int f = -1;
	char *out = NULL;
	ssize_t bytes_written = 0;

	assert(db);
	if (!db)
		return BOOL_FALSE;

	out = ischeme_deploy(scheme, 0);
	if (!out)
		return BOOL_FALSE;

	// Get configuration info from kdb object
	ini = kdb_ini(db);
	if (ini)
		out_path = faux_ini_find(ini, "DeployPath");

	if (out_path)
		f = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 00644);
	else
		f = STDOUT_FILENO;
	if (f < 0) {
		faux_str_free(out);
		return BOOL_FALSE;
	}
	bytes_written = faux_write_block(f, out, strlen(out));
	if (out_path)
		close(f);
	faux_str_free(out);
	if (bytes_written < 0)
		return BOOL_FALSE;

	return BOOL_TRUE;
}
