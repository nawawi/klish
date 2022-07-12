/** @file hist.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>
#include <faux/file.h>

#include "tinyrl/hist.h"


struct hist_s {
	faux_list_t *list;
	faux_list_node_t *pos;
	size_t stifle;
	char *fname;
	bool_t temp;
};


static int hist_compare(const void *first, const void *second)
{
	const char *f = (const char *)first;
	const char *s = (const char *)second;

	return strcmp(f, s);
}


static int hist_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const char *s = (const char *)list_item;

	return strcmp(f, s);
}


hist_t *hist_new(const char *hist_fname, size_t stifle)
{
	hist_t *hist = faux_zmalloc(sizeof(hist_t));
	if (!hist)
		return NULL;

	// Init
	hist->list = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		hist_compare, hist_kcompare, (void (*)(void *))faux_str_free);
	hist->pos = NULL; // It means position is reset
	hist->stifle = stifle;
	if (hist_fname)
		hist->fname = faux_str_dup(hist_fname);
	hist->temp = BOOL_FALSE;

	return hist;
}


void hist_free(hist_t *hist)
{
	if (!hist)
		return;

	faux_str_free(hist->fname);
	faux_list_free(hist->list);
	faux_free(hist);
}


void hist_pos_reset(hist_t *hist)
{
	if (!hist)
		return;

	// History contain temp entry
	if (hist->temp) {
		faux_list_del(hist->list, faux_list_tail(hist->list));
		hist->temp = BOOL_FALSE;
	}

	hist->pos = NULL;
}


const char *hist_pos(hist_t *hist)
{
	if (!hist)
		return NULL;

	if (!hist->pos)
		return NULL;

	return (const char *)faux_list_data(hist->pos);
}


const char *hist_pos_up(hist_t *hist)
{
	if (!hist)
		return NULL;

	if (!hist->pos) {
		hist->pos = faux_list_tail(hist->list);
	} else {
		faux_list_node_t *new_pos = faux_list_prev_node(hist->pos);
		if (new_pos) // Don't go up over the list
			hist->pos = new_pos;
	}

	if (!hist->pos)
		return NULL;

	return (const char *)faux_list_data(hist->pos);
}


const char *hist_pos_down(hist_t *hist)
{
	if (!hist)
		return NULL;

	if (!hist->pos)
		return NULL;
	hist->pos = faux_list_next_node(hist->pos);

	if (!hist->pos)
		return NULL;

	return (const char *)faux_list_data(hist->pos);
}


void hist_add(hist_t *hist, const char *line, bool_t temp)
{
	if (!hist)
		return;

	hist_pos_reset(hist);
	if (temp) {
		hist->temp = BOOL_TRUE;
	} else {
		// Try to find the same string within history
		faux_list_kdel(hist->list, line);
	}
	// Add line to the end of list.
	// Use (void *)line to make compiler happy about 'const' modifier.
	faux_list_add(hist->list, (void *)faux_str_dup(line));

	// Stifle list. Note we add only one element so list length can be
	// (stifle + 1) but not greater so remove only one element from list.
	// If stifle = 0 then don't stifle at all (special case).
	if ((hist->stifle != 0) && (faux_list_len(hist->list) > hist->stifle))
		faux_list_del(hist->list, faux_list_head(hist->list));
}


void hist_clear(hist_t *hist)
{
	if (!hist)
		return;

	faux_list_del_all(hist->list);
	hist_pos_reset(hist);
}


int hist_save(const hist_t *hist)
{
	faux_file_t *f = NULL;
	faux_list_node_t *node = NULL;
	const char *line = NULL;

	if (!hist)
		return -1;
	if (!hist->fname)
		return 0;

	f = faux_file_open(hist->fname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (!f)
		return -1;
	node = faux_list_head(hist->list);
	while ((line = (const char *)faux_list_each(&node))) {
		faux_file_write(f, line, strlen(line));
		faux_file_write(f, "\n", 1);
	}
	faux_file_close(f);

	return 0;
}


int hist_restore(hist_t *hist)
{
	faux_file_t *f = NULL;
	char *line = NULL;
	size_t count = 0;

	if (!hist)
		return -1;
	if (!hist->fname)
		return 0;

	// Remove old entries from list
	hist_clear(hist);

	f = faux_file_open(hist->fname, O_RDONLY, 0);
	if (!f)
		return -1;

	while (((hist->stifle == 0) || (count < hist->stifle)) &&
		(line = faux_file_getline(f))) {
		faux_list_add(hist->list, line);
		count++;
	}
	faux_file_close(f);

	return 0;
}
