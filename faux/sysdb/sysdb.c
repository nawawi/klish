/** @file sysdb.c
 * @brief Wrappers for system database functions like getpwnam(), getgrnam().
 */

// It must be here to include config.h before another headers
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include "faux/faux.h"
#include "faux/sysdb.h"

#define DEFAULT_GETPW_R_SIZE_MAX 1024

/** @brief Wrapper for ugly getpwnam_r() function.
 *
 * Gets passwd structure by user name. Easy to use.
 *
 * @param [in] name User name.
 * @return Pointer to allocated passwd structure.
 * @warning The resulting pointer (return value) must be freed by faux_free().
 */
struct passwd *faux_sysdb_getpwnam(const char *name) {

	long int size = 0;
	char *buf = NULL;
	struct passwd *pwbuf = NULL;
	struct passwd *pw = NULL;
	int res = 0;

#ifdef _SC_GETPW_R_SIZE_MAX
	if ((size = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	pwbuf = faux_zmalloc(sizeof(*pwbuf) + size);
	if (!pwbuf)
		return NULL;
	buf = (char *)pwbuf + sizeof(*pwbuf);

	res = getpwnam_r(name, pwbuf, buf, size, &pw);
	if (res || !pw) {
		faux_free(pwbuf);
		if (res)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return pwbuf;
}

/** @brief Wrapper for ugly getpwuid_r() function.
 *
 * Gets passwd structure by UID. Easy to use.
 *
 * @param [in] uid UID.
 * @return Pointer to allocated passwd structure.
 * @warning The resulting pointer (return value) must be freed by faux_free().
 */
struct passwd *faux_sysdb_getpwuid(uid_t uid) {

	long int size = 0;
	char *buf = NULL;
	struct passwd *pwbuf = NULL;
	struct passwd *pw = NULL;
	int res = 0;

#ifdef _SC_GETPW_R_SIZE_MAX
	if ((size = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	pwbuf = faux_zmalloc(sizeof(*pwbuf) + size);
	if (!pwbuf)
		return NULL;
	buf = (char *)pwbuf + sizeof(*pwbuf);

	res = getpwuid_r(uid, pwbuf, buf, size, &pw);
	if (!pw) {
		faux_free(pwbuf);
		if (res)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return pwbuf;
}

/** @brief Wrapper for ugly getgrnam_r() function.
 *
 * Gets group structure by group name. Easy to use.
 *
 * @param [in] name Group name.
 * @return Pointer to allocated group structure.
 * @warning The resulting pointer (return value) must be freed by faux_free().
 */
struct group *faux_sysdb_getgrnam(const char *name) {

	long int size;
	char *buf;
	struct group *grbuf; 
	struct group *gr = NULL;
	int res = 0;

#ifdef _SC_GETGR_R_SIZE_MAX
	if ((size = sysconf(_SC_GETGR_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	grbuf = faux_zmalloc(sizeof(*grbuf) + size);
	if (!grbuf)
		return NULL;
	buf = (char *)grbuf + sizeof(*grbuf);

	res = getgrnam_r(name, grbuf, buf, size, &gr);
	if (!gr) {
		faux_free(grbuf);
		if (res)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return grbuf;
}

/** @brief Wrapper for ugly getgrgid_r() function.
 *
 * Gets group structure by GID. Easy to use.
 *
 * @param [in] gid GID.
 * @return Pointer to allocated group structure.
 * @warning The resulting pointer (return value) must be freed by faux_free().
 */
struct group *faux_sysdb_getgrgid(gid_t gid) {

	long int size;
	char *buf;
	struct group *grbuf;
	struct group *gr = NULL;
	int res = 0;

#ifdef _SC_GETGR_R_SIZE_MAX
	if ((size = sysconf(_SC_GETGR_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	grbuf = faux_zmalloc(sizeof(struct group) + size);
	if (!grbuf)
		return NULL;
	buf = (char *)grbuf + sizeof(struct group);

	res = getgrgid_r(gid, grbuf, buf, size, &gr);
	if (!gr) {
		faux_free(grbuf);
		if (res)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return grbuf;
}
