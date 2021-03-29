/*
 * ------------------------------------------------------
 * shell_roxml.c
 *
 * This file implements the means to read an XML encoded file 
 * and populate the CLI tree based on the contents. It implements
 * the kxml_ API using roxml
 * ------------------------------------------------------
 */

#include <errno.h>
#include <roxml.h>
#include <string.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <klish/kxml.h>


bool_t kxml_doc_start(void)
{
	return BOOL_TRUE;
}


bool_t kxml_doc_stop(void)
{
	return BOOL_TRUE;
}


kxml_doc_t *kxml_doc_read(const char *filename)
{
	node_t *doc = roxml_load_doc((char *)filename);

	return (kxml_doc_t *)doc;
}


void kxml_doc_release(kxml_doc_t *doc)
{
	if (!doc)
		return;

	roxml_release(RELEASE_ALL);
	roxml_close((node_t *)doc);
}


bool_t kxml_doc_is_valid(const kxml_doc_t *doc)
{
	return (bool_t)(doc != NULL);
}

/*
int kxml_doc_error_caps(kxml_doc_t *doc)
{
	return kxml_ERR_NOCAPS;
}

int kxml_doc_get_err_line(kxml_doc_t *doc)
{
	return -1;
}

int kxml_doc_get_err_col(kxml_doc_t *doc)
{
	return -1;
}

const char *kxml_doc_get_err_msg(kxml_doc_t *doc)
{
	return "";
}
*/

kxml_nodetype_e kxml_node_get_type(const kxml_node_t *node)
{
	int type = 0;

	if (!node)
		return KXML_NODE_UNKNOWN;

	type = roxml_get_type((node_t *)node);
	switch (type) {
	case ROXML_ELM_NODE:
		return KXML_NODE_ELM;
	case ROXML_TXT_NODE:
		return KXML_NODE_TEXT;
	case ROXML_CMT_NODE:
		return KXML_NODE_COMMENT;
	case ROXML_PI_NODE:
		return KXML_NODE_PI;
	case ROXML_ATTR_NODE:
		return KXML_NODE_ATTR;
	default:
		break;
	}

	return KXML_NODE_UNKNOWN;
}


kxml_node_t *kxml_doc_root(const kxml_doc_t *doc)
{
	node_t *root = NULL;
	char *name = NULL;

	if (!doc)
		return NULL;
	root = roxml_get_root((node_t *)doc);
	if (!root)
		return NULL;
	/* The root node is always documentRoot since libroxml-2.2.2. */
	/* It's good but not compatible with another XML parsers. */
	name = roxml_get_name(root, NULL, 0);
	if (0 == strcmp(name, "documentRoot"))
		root = roxml_get_chld(root, NULL, 0);
	roxml_release(name);

	return (kxml_node_t *)root;
}


kxml_node_t *kxml_node_parent(const kxml_node_t *node)
{
	node_t *roxn = (node_t *)node;
	node_t *root = NULL;

	if (!node)
		return NULL;

	root = roxml_get_root(roxn);
	if (roxn != root)
		return (kxml_node_t *)roxml_get_parent(roxn);

	return NULL;
}


const kxml_node_t *kxml_node_next_child(const kxml_node_t *parent,
	const kxml_node_t *curchild)
{
	node_t *roxc = (node_t *)curchild;
	node_t *roxp = (node_t *)parent;
	node_t *child = NULL;
	int count = 0;

	if (!parent)
		return NULL;

	if (roxc)
		return (kxml_node_t *)
			roxml_get_next_sibling(roxc);


	count = roxml_get_chld_nb(roxp);
	if (count)
		child = roxml_get_chld(roxp, NULL, 0);

	return (kxml_node_t *)child;
}


static int i_is_needle(char *src, const char *needle)
{
	int nlen = strlen(needle);
	int slen = strlen(src);

	if (slen >= nlen) {
		if (strncmp(src, needle, nlen) == 0)
			return 1;
	}
	return 0;
}


/* warning: dst == src is valid */
static void i_decode_and_copy(char *dst, char *src)
{
	while (*src) {
		if (*src == '&') {
			if (i_is_needle(src, "&lt;")) {
				*dst++ = '<';
				src += 4;
			} else if (i_is_needle(src, "&gt;")) {
				*dst++ = '>';
				src += 4;
			} else if (i_is_needle(src, "&amp;")) {
				*dst++ = '&';
				src += 5;
			} else {
				*dst++ = *src++;
			}
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = 0;
}


char *kxml_node_attr(const kxml_node_t *node,
	const char *attrname)
{
	node_t *roxn = (node_t *)node;
	node_t *attr = NULL;
	char *content = NULL;

	if (!node || !attrname)
		return NULL;

	attr = roxml_get_attr(roxn, (char *)attrname, 0);
	content = roxml_get_content(attr, NULL, 0, NULL);
	if (content)
		i_decode_and_copy(content, content);

	return content;
}


void kxml_node_attr_free(char *str)
{
	if (!str)
		return;
	roxml_release(str);
}


char *kxml_node_content(const kxml_node_t *node)
{
	char *content = NULL;

	if (!node)
		return NULL;

	content = roxml_get_content((node_t *)node, NULL, 0, NULL);
	if (content)
		i_decode_and_copy(content, content);

	return content;
}


void kxml_node_content_free(char *str)
{
	if (!str)
		return;
	roxml_release(str);
}


char *kxml_node_get_name(const kxml_node_t *node)
{
	if (!node)
		return NULL;

	return roxml_get_name((node_t *)node, NULL, 0);
}


void kxml_node_name_free(char *str)
{
	if (!str)
		return;
	roxml_release(str);
}
