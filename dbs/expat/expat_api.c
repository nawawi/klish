/*
 * ------------------------------------------------------
 * shell_expat.c
 *
 * This file implements the means to read an XML encoded file
 * and populate the CLI tree based on the contents. It implements
 * the kxml_ API using the expat XML parser
 *
 * expat is not your typical XML parser. It does not work
 * by creating a full in-memory XML tree, but by calling specific
 * callbacks (element handlers) regularly while parsing. It's up
 * to the user to create the corresponding XML tree if needed
 * (obviously, this is what we're doing, as we really need the XML
 * tree in klish).
 *
 * The code below do that. It transforms the output of expat
 * to a DOM representation of the underlying XML file. This is
 * a bit overkill, and maybe a later implementation will help to
 * cut the work to something simpler, but the current klish
 * implementation requires this.
 * ------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* FreeBSD have verbatim version of expat named bsdxml */
#ifdef HAVE_LIB_BSDXML
#include <bsdxml.h>
#else
#include <expat.h>
#endif

#include <faux/faux.h>
#include <faux/str.h>
#include <klish/kxml.h>


/** DOM_like XML node
 *
 * @struct kxml_node_s
 */
struct kxml_node_s {
	char *name;
	kxml_node_t *parent; /**< parent node */
	kxml_node_t *children; /**< list of children */
	kxml_node_t *next; /**< next sibling */
	kxml_node_t *attributes; /**< attributes are nodes too */
	char *content; /**< !NULL for text and attributes nodes */
	kxml_nodetype_e type; /**< node type */
	int depth; /**< node depth */
	kxml_doc_t *doc;
};


/** DOM-like XML document
 *
 * @struct kxml_doc_s
 */
struct kxml_doc_s {
	kxml_node_t *root; /**< list of root elements */
	kxml_node_t *current; /**< current element */
	char *filename; /**< current filename */
};


/*
 * Expat need these functions to be able to build a DOM-like tree that
 * will be usable by klish.
 */
/** Put a element at the and of an element list
 *
 * @param first first element of the list
 * @param node element to add
 * @return new first element of the list
 */
static kxml_node_t *kexpat_list_push_back(kxml_node_t *first, kxml_node_t *node)
{
	kxml_node_t *cur = first;
	kxml_node_t *prev = NULL;

	while (cur) {
		prev = cur;
		cur = cur->next;
	}
	if (prev) {
		prev->next = node;
		return first;
	}
	return node;
}

/** Generic add_attr() function
 *
 * @param first first attribute in the attribute list
 * @param n attribute name
 * @param v attribute value
 * @return the new first attribute in the attribute list
 */
static kxml_node_t *kexpat_add_attr(kxml_node_t *first, const char *n, const char *v)
{
	kxml_node_t *node = NULL;

	node = malloc(sizeof(kxml_node_t));
	if (!node)
		return first;

	node->name = strdup(n);
	node->content = strdup(v);
	node->children = NULL;
	node->attributes = NULL;
	node->next = NULL;
	node->type = KXML_NODE_ATTR;
	node->depth = 0;

	return kexpat_list_push_back(first, node);
}


/** Run through an expat attribute list, and create a DOM-like attribute list
 *
 * @param node parent node
 * @param attr NULL-terminated attribute liste
 *
 * Each attribute uses two slots in the expat attribute list. The first one is
 * used to store the name, the second one is used to store the value.
 */
static void kexpat_add_attrlist(kxml_node_t *node, const char **attr)
{
	int i = 0;

	for (i = 0; attr[i]; i += 2) {
		node->attributes = kexpat_add_attr(node->attributes,
			attr[i], attr[i+1]);
	}
}


/** Generic make_node() function
 *
 * @param parent XML parent node
 * @param type XML node type
 * @param n node name (can be NULL, strdup'ed)
 * @param v node content (can be NULL, strdup'ed)
 * @param attr attribute list
 * @return a new node or NULL on error
 */
static kxml_node_t *kexpat_make_node(kxml_node_t *parent,
	kxml_nodetype_e type,
	const char *n,
	const char *v,
	const char **attr)
{
	kxml_node_t *node = NULL;

	node = malloc(sizeof(kxml_node_t));
	if (!node)
		return NULL;
	node->name = n ? strdup(n) : NULL;
	node->content = v ? strdup(v) : NULL;
	node->children = NULL;
	node->attributes = NULL;
	node->next = NULL;
	node->parent = parent;
	node->doc = parent ? parent->doc : NULL;
	node->depth = parent ? parent->depth + 1 : 0;
	node->type = type;

	if (attr)
		kexpat_add_attrlist(node, attr);

	if (parent)
		parent->children = kexpat_list_push_back(parent->children, node);

	return node;
}


/** Add a new XML root
 *
 * @param doc XML document
 * @param el root node name
 * @param attr expat attribute list
 * @return a new root element
 */
static kxml_node_t *kexpat_add_root(kxml_doc_t *doc, const char *el, const char **attr)
{
	kxml_node_t *node = NULL;

	node = kexpat_make_node(NULL, KXML_NODE_ELM, el, NULL, attr);
	if (!node)
		return doc->root;

	doc->root = kexpat_list_push_back(doc->root, node);

	return node;
}


/** Add a new XML element as a child
 *
 * @param cur parent XML element
 * @param el element name
 * @param attr expat attribute list
 * @return a new XMl element
 */
static kxml_node_t *kexpat_add_child(kxml_node_t *cur, const char *el, const char **attr)
{
	kxml_node_t *node;

	node = kexpat_make_node(cur, KXML_NODE_ELM, el, NULL, attr);
	if (!node)
		return cur;

	return node;
}


/** Expat handler: element content
 *
 * @param data user data
 * @param s content (not nul-termainated)
 * @param len content length
 */
static void kexpat_chardata_handler(void *data, const char *s, int len)
{
	kxml_doc_t *doc = data;

	if (doc->current) {
		char *content = malloc(len + 1);
		strncpy(content, s, len);
		content[len] = '\0';

		kexpat_make_node(doc->current, KXML_NODE_TEXT, NULL, content, NULL);
		/*
		 * the previous call is a bit too generic, and strdup() content
		 * so we need to free out own version of content.
		 */
		free(content);
	}
}


/** Expat handler: start XML element
 *
 * @param data user data
 * @param el element name (nul-terminated)
 * @param attr expat attribute list
 */
static void kexpat_element_start(void *data, const char *el, const char **attr)
{
	kxml_doc_t *doc = data;

	if (!doc->current) {
		doc->current = kexpat_add_root(doc, el, attr);
	} else {
		doc->current = kexpat_add_child(doc->current, el, attr);
	}
}


/** Expat handler: end XML element
 *
 * @param data user data
 * @param el element name
 */
static void kexpat_element_end(void *data, const char *el)
{
	kxml_doc_t *doc = data;

	if (doc->current) {
		doc->current = doc->current->parent;
	}

	el = el; /* Happy compiler */
}


/** Free a node, its children and its attributes
 *
 * @param node node to free
 */
static void kexpat_free_node(kxml_node_t *cur)
{
	kxml_node_t *node;
	kxml_node_t *first;

	if (cur->attributes) {
		first = cur->attributes;
		while (first) {
			node = first;
			first = first->next;
			kexpat_free_node(node);
		}
	}
	if (cur->children) {
		first = cur->children;
		while (first) {
			node = first;
			first = first->next;
			kexpat_free_node(node);
		}
	}
	if (cur->name)
		free(cur->name);
	if (cur->content)
		free(cur->content);
	free(cur);
}

/*
 * Public interface
 */

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
	kxml_doc_t *doc = NULL;
	struct stat sb = {};
	int fd = -1;
	char *buffer = NULL;
	XML_Parser parser;
	int rb = 0;

	doc = malloc(sizeof(kxml_doc_t));
	if (!doc)
		return NULL;
	memset(doc, 0, sizeof(kxml_doc_t));
	doc->filename = strdup(filename);
	parser = XML_ParserCreate(NULL);
	if (!parser)
		goto error_parser_create;
	XML_SetUserData(parser, doc);
	XML_SetCharacterDataHandler(parser, kexpat_chardata_handler);
	XML_SetElementHandler(parser,
		kexpat_element_start,
		kexpat_element_end);

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		goto error_open;
	fstat(fd, &sb);
	buffer = malloc(sb.st_size+1);
	rb = read(fd, buffer, sb.st_size);
	if (rb < 0) {
		close(fd);
		goto error_parse;
	}
	buffer[sb.st_size] = 0;
	close(fd);

	if (!XML_Parse(parser, buffer, sb.st_size, 1))
		goto error_parse;

	XML_ParserFree(parser);
	free(buffer);

	return doc;

error_parse:
	free(buffer);

error_open:
	XML_ParserFree(parser);

error_parser_create:
	kxml_doc_release(doc);

	return NULL;
}


void kxml_doc_release(kxml_doc_t *doc)
{
	if (doc) {
		kxml_node_t *node;
		while (doc->root) {
			node = doc->root;
			doc->root = node->next;
			kexpat_free_node(node);
		}
		if (doc->filename)
			free(doc->filename);
		free(doc);
	}
}


bool_t kxml_doc_is_valid(const kxml_doc_t *doc)
{
	return (bool_t)(doc && doc->root);
}

/*
int kxml_doc_error_caps(kxml_doc_t *doc)
{
	doc = doc; // Happy compiler

	return kxml_ERR_NOCAPS;
}

int kxml_doc_get_err_line(kxml_doc_t *doc)
{
	doc = doc; // Happy compiler

	return -1;
}

int kxml_doc_get_err_col(kxml_doc_t *doc)
{
	doc = doc; // Happy compiler

	return -1;
}

const char *kxml_doc_get_err_msg(kxml_doc_t *doc)
{
	doc = doc; // Happy compiler

	return "";
}
*/

kxml_nodetype_e kxml_node_type(const kxml_node_t *node)
{
	if (node)
		return node->type;

	return KXML_NODE_UNKNOWN;
}


kxml_node_t *kxml_doc_root(const kxml_doc_t *doc)
{
	if (doc)
		return doc->root;

	return NULL;
}


kxml_node_t *kxml_node_parent(const kxml_node_t *node)
{
	if (node)
		return node->parent;

	return NULL;
}


const kxml_node_t *kxml_node_next_child(const kxml_node_t *parent,
	const kxml_node_t *curchild)
{
	if (curchild)
		return curchild->next;
	if (parent)
		return parent->children;

	return NULL;
}


char *kxml_node_attr(const kxml_node_t *node,
	const char *attrname)
{
	kxml_node_t *n = NULL;

	if (!node)
		return NULL;

	n = node->attributes;
	while (n) {
		if (strcmp(n->name, attrname) == 0)
			return n->content;
		n = n->next;
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
	char *content = NULL;
	kxml_node_t *children = NULL;

	if (!node)
		return NULL;

	children = node->children;
	while (children) {
		if (children->type == KXML_NODE_TEXT && children->content)
			faux_str_cat(&content, children->content);
		children = children->next;
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
	if (!node)
		return NULL;

	return node->name;
}


void kxml_node_name_free(char *str)
{
	str = str; // Happy compiler
	// kxml_node_attr() doesn't allocate any memory
	// so we don't need to free()
}
