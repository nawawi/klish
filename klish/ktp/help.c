#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/ktp_session.h>


int help_compare(const void *first, const void *second)
{
	const help_t *f = (const help_t *)first;
	const help_t *s = (const help_t *)second;

	return strcmp(f->prefix, s->prefix);
}


int help_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const help_t *s = (const help_t *)list_item;

	return strcmp(f, s->prefix);
}


help_t *help_new(char *prefix, char *line)
{
	help_t *help = NULL;

	help = faux_zmalloc(sizeof(*help));
	help->prefix = prefix;
	help->line = line;

	return help;
}


void help_free(void *ptr)
{
	help_t *help = (help_t *)ptr;

	faux_free(help->prefix);
	faux_free(help->line);

	faux_free(help);
}
