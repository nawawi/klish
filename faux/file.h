/** @file file.h
 * @brief Public interface to work with files.
 */

#ifndef _faux_file_h
#define _faux_file_h

#include "faux/faux.h"

typedef struct faux_file_s faux_file_t;

C_DECL_BEGIN

faux_file_t *faux_file_fdopen(int fd);
faux_file_t *faux_file_open(const char *pathname, int flags, mode_t mode);
int faux_file_close(faux_file_t *file);
int faux_file_fileno(faux_file_t *file);
char *faux_file_getline(faux_file_t *file);

C_DECL_END

#endif				/* _faux_file_h */
