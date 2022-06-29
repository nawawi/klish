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

#include "tinyrl/hist.h"


struct hist_s {
	faux_list_t *list;
	faux_list_node_t *pos;
	size_t stifle;
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



hist_t *hist_new(size_t stifle)
{
	hist_t *hist = faux_zmalloc(sizeof(hist_t));
	if (!hist)
		return NULL;

	// Init
	hist->list = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		hist_compare, hist_kcompare, (void (*)(void *))faux_str_free);
	hist->pos = NULL; // It means position is reset
	hist->stifle = stifle;

	return hist;
}


void hist_free(hist_t *hist)
{
	if (!hist)
		return;

	faux_list_free(hist->list);
	faux_free(hist);
}


void hist_pos_reset(hist_t *hist)
{
	if (!hist)
		return;

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

	if (!hist->pos)
		hist->pos = faux_list_tail(hist->list);
	else
		hist->pos = faux_list_prev_node(hist->pos);

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


void hist_add(hist_t *hist, const char *line)
{
	if (!hist)
		return;

	hist_pos_reset(hist);
	// Try to find the same string within history
	faux_list_kdel(hist->list, line);
	// Add line to the end of list.
	// Use (void *)line to make compiler happy about 'const' modifier.
	faux_list_add(hist->list, (void *)line);

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


/*
int hist_save(const hist_t *hist, const char *fname)
{
	hist_entry_t *entry;
	hist_iterator_t iter;
	FILE *f;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}
	if (!(f = fopen(fname, "w")))
		return -1;
	for (entry = hist_getfirst(hist, &iter);
		entry; entry = hist_getnext(&iter)) {
		if (fprintf(f, "%s\n", hist_entry__get_line(entry)) < 0)
			return -1;
	}
	fclose(f);

	return 0;
}

int hist_restore(hist_t *hist, const char *fname)
{
	FILE *f;
	char *p;
	int part_len = 300;
	char *buf;
	int buf_len = part_len;
	int res = 0;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}
	if (!(f = fopen(fname, "r")))
		return 0; // Can't find history file

	buf = malloc(buf_len);
	p = buf;
	while (fgets(p, buf_len - (p - buf), f)) {
		char *ptmp = NULL;
		char *el = strchr(buf, '\n');
		if (el) { // The whole line was readed
			*el = '\0';
			hist_add(hist, buf);
			p = buf;
			continue;
		}
		buf_len += part_len;
		ptmp = realloc(buf, buf_len);
		if (!ptmp) {
			res = -1;
			goto end;
		}
		buf = ptmp;
		p = buf + buf_len - part_len - 1;
	}
end:
	free(buf);
	fclose(f);

	return res;
}

*/
