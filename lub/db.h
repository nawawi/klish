#ifndef _lub_passwd_h
#define _lub_passwd_h
#include <stddef.h>

/* wrappers for ugly getpwnam_r()-like functions */
struct passwd *lub_db_getpwnam(const char *name);
struct passwd *lub_db_getpwuid(uid_t uid);
struct group *lub_db_getgrnam(const char *name);
struct group *lub_db_getgrgid(gid_t gid);

#endif

