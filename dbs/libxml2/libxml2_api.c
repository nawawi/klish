/** @file libxml2_api.c
 */

#include <errno.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <klish/kxml.h>


bool_t kxml_doc_start(void)
{
	return BOOL_TRUE;
}


bool_t kxml_doc_stop(void)
{
	xmlCleanupParser();
	return BOOL_TRUE;
}


kxml_doc_t *kxml_doc_read(const char *filename)
{
	xmlDoc *doc = NULL;

	if (faux_str_is_empty(filename))
		return NULL;

	doc = xmlReadFile(filename, NULL, 0);

	return (kxml_doc_t *)doc;
}


void kxml_doc_release(kxml_doc_t *doc)
{
	if (doc)
		xmlFreeDoc((xmlDoc *)doc);
}


bool_t kxml_doc_is_valid(const kxml_doc_t *doc)
{
	return (bool_t)(doc != NULL);
}


/*
int kxml_doc_error_caps(kxml_doc_t *doc)
{
	doc = doc; // happy compiler
	return kxmlERR_NOCAPS;
}

int kxml_doc_err_line(kxml_doc_t *doc)
{
	doc = doc; // happy compiler
	return -1;
}

int kxml_doc_err_col(kxml_doc_t *doc)
{
	doc = doc; // happy compiler
	return -1;
}

const char *kxml_doc_err_msg(kxml_doc_t *doc)
{
	doc = doc; // happy compiler
	return "";
}
*/

kxml_nodetype_e kxml_node_type(const kxml_node_t *node)
{
	if (!node)
		return KXML_NODE_UNKNOWN;

	switch (((xmlNode *)node)->type) {
	case XML_ELEMENT_NODE:
		return KXML_NODE_ELM;
	case XML_TEXT_NODE:
		return KXML_NODE_TEXT;
	case XML_COMMENT_NODE:
		return KXML_NODE_COMMENT;
	case XML_PI_NODE:
		return KXML_NODE_PI;
	case XML_ATTRIBUTE_NODE:
		return KXML_NODE_ATTR;
	default:
		break;
	}

	return KXML_NODE_UNKNOWN;
}


kxml_node_t *kxml_doc_root(const kxml_doc_t *doc)
{
	if (!doc)
		return NULL;

	return (kxml_node_t *)xmlDocGetRootElement((xmlDoc *)doc);
}


kxml_node_t *kxml_node_parent(const kxml_node_t *node)
{
	xmlNode *root = NULL;
	xmlNode *n = (xmlNode *)node;

	if (!n)
		return NULL;

	root = xmlDocGetRootElement(n->doc);
	if (n != root)
		return (kxml_node_t *)n->parent;

	return NULL;
}


const kxml_node_t *kxml_node_next_child(const kxml_node_t *parent,
	const kxml_node_t *curchild)
{
	xmlNode *child = NULL;

	if (!parent)
		return NULL;

	if (curchild) {
		child = ((xmlNode *)curchild)->next;
	} else {
		child = ((xmlNode *)parent)->children;
	}

	return (kxml_node_t *)child;
}


char *kxml_node_attr(const kxml_node_t *node, const char *attrname)
{
	xmlNode *n = (xmlNode *)node;
	xmlAttr *a = NULL;

	if (!node || !attrname)
		return NULL;

	if (n->type != XML_ELEMENT_NODE)
		return NULL;

	a = n->properties;
	while (a) {
		if (faux_str_casecmp((char *)a->name, attrname) == 0) {
			if (a->children && a->children->content)
				return (char *)a->children->content;
			else
				return NULL;
		}
		a = a->next;
	}

	return NULL;
}


void kxml_node_attr_free(char *str)
{
	str = str; // Happy compiler
	// kxml_node_attr() doesn't allocate any memory
	// so we don't need to free()
}


char *kxml_node_content(const kxml_node_t *node)
{
	xmlNode *n = (xmlNode *)node;
	xmlNode *c = NULL;
	char *content = NULL;

	if (!n)
		return NULL;

	c = n->children;
	while (c) {
		if ((c->type == XML_TEXT_NODE || c->type == XML_CDATA_SECTION_NODE)
			 && !xmlIsBlankNode(c)) {
			faux_str_cat(&content, (char *)c->content);
		}
		c = c->next;
	}

	return content;
}


void kxml_node_content_free(char *str)
{
	if (!str)
		return;

	faux_str_free(str);
}


char *kxml_node_name(const kxml_node_t *node)
{
	xmlNode *n = (xmlNode *)node;

	if (!node)
		return NULL;

	return (char *)n->name;
}


void kxml_node_name_free(char *str)
{
	str = str; // Happy compiler
	// kxml_node_name() doesn't allocate any memory
	// so we don't need to free()
}
