#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>

#include <klish/ktp_session.h>

#include "private.h"


static int ktpd_session_compare(const void *first, const void *second)
{
	const ktpd_session_t *f = (const ktpd_session_t *)first;
	const ktpd_session_t *s = (const ktpd_session_t *)second;

	return (ktpd_session_fd(f) - ktpd_session_fd(s));
}


static int ktpd_session_kcompare(const void *key, const void *list_item)
{
	const int *f = (const int *)key;
	const ktpd_session_t *s = (const ktpd_session_t *)list_item;

	return (*f - ktpd_session_fd(s));
}


ktpd_clients_t *ktpd_clients_new(void)
{
	ktpd_clients_t *db = NULL;

	db = faux_zmalloc(sizeof(*db));
	assert(db);
	if (!db)
		return NULL;

	// Init
	db->list = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		ktpd_session_compare, ktpd_session_kcompare,
		(void (*)(void *))ktpd_session_free);

	return db;
}


/** @brief Frees.
 */
void ktpd_clients_free(ktpd_clients_t *db)
{
	if (!db)
		return;

	faux_list_free(db->list);
	faux_free(db);
}


ktpd_session_t *ktpd_clients_find(const ktpd_clients_t *db, int fd)
{
	assert(db);
	if (!db)
		return NULL;
	assert(fd >= 0);
	if (fd < 0)
		return NULL;

	return (ktpd_session_t *)faux_list_kfind(db->list, &fd);
}


ktpd_session_t *ktpd_clients_add(ktpd_clients_t *db, int fd)
{
	ktpd_session_t *session = NULL;

	assert(db);
	if (!db)
		return NULL;
	assert(fd >= 0);
	if (fd < 0)
		return NULL;

	// Already exists
	if (ktpd_clients_find(db, fd))
		return NULL;

	session = ktpd_session_new(fd);
	if (!session)
		return NULL;

	if (!faux_list_add(db->list, session)) {
		ktpd_session_free(session);
		return NULL;
	}

	return session;
}


int ktpd_clients_del(ktpd_clients_t *db, int fd)
{
	assert(db);
	if (!db)
		return -1;
	assert(fd >= 0);
	if (fd < 0)
		return -1;

	if (!faux_list_kdel(db->list, &fd))
		return -1;

	return 0;
}


faux_list_node_t *ktpd_clients_init_iter(const ktpd_clients_t *db)
{
	assert(db);
	if (!db || !db->list)
		return NULL;

	return faux_list_head(db->list);
}


ktpd_session_t *ktpd_clients_each(faux_list_node_t **iter)
{
	return (ktpd_session_t *)faux_list_each(iter);
}


void ktpd_clients_debug(ktpd_clients_t *db)
{
	faux_list_node_t *iter = NULL;
	ktpd_session_t *session = NULL;

	assert(db);
	if (!db)
		return;

	iter = faux_list_head(db->list);
	if (!iter)
		return;

	while ((session = (ktpd_session_t *)faux_list_each(&iter))) {
		printf("clients: %d\n", ktpd_session_fd(session));
	}

}
