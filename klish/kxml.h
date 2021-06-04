/** @file kxml.h
 * @brief XML API for XML engines to provide.
 *
 * Don't use libfaux features here to be maximally independent from
 * specific libraries. It's just an interface API.
 */

#ifndef _klish_kxml_h
#define _klish_kxml_h

#include <faux/faux.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>

/** @brief XML document (opaque type).
 *
 * The real type is defined by the selected external API.
 */
typedef struct kxml_doc_s kxml_doc_t;


/* @brief XML node (opaque type).
 *
 * The real type is defined by the selected external API.
 */
typedef struct kxml_node_s kxml_node_t;


/** @brief Start and Stop XML parser engine.
 *
 * Some parsers need a global cleanup at the end of the programm.
 */
bool_t kxml_doc_start(void);
bool_t kxml_doc_stop(void);


/** @brief Read an XML document.
 */
kxml_doc_t *kxml_doc_read(const char *filename);


/** @brief Release a previously opened XML document.
 */
void kxml_doc_release(kxml_doc_t *doc);


/* @brief Validate a doc.
 *
 * Checks if a doc is valid (i.e. it loaded successfully).
 */
bool_t kxml_doc_is_valid(const kxml_doc_t *doc);


/** @brief Gets the document root.
 */
kxml_node_t *kxml_doc_root(const kxml_doc_t *doc);


/** @brief Gets error description, when available.
 */
int kxml_doc_err_line(kxml_doc_t *doc);
int kxml_doc_err_col(kxml_doc_t *doc);
const char *kxml_doc_err_msg(kxml_doc_t *doc);


/** @brief Node types.
 */
typedef enum {
	KXML_NODE_DOC,
	KXML_NODE_ELM,
	KXML_NODE_TEXT,
	KXML_NODE_ATTR,
	KXML_NODE_COMMENT,
	KXML_NODE_PI,
	KXML_NODE_DECL,
	KXML_NODE_UNKNOWN,
} kxml_nodetype_e;


/** @brief Gets the node type.
 */
kxml_nodetype_e kxml_node_type(const kxml_node_t *node);


/** @brief Gets the next child or NULL.
 *
 * If curchild is NULL, then the function returns the first child.
 */
const kxml_node_t *kxml_node_next_child(const kxml_node_t *parent,
	const kxml_node_t *curchild);


/** @brief Gets the parent node.
 *
 * Returns NULL if node is the document root node.
 */
kxml_node_t *kxml_node_parent(const kxml_node_t *node);


/** @brief Gets the node name.
 */
char *kxml_node_name(const kxml_node_t *node);
void kxml_node_name_free(char *str);


/** @brief Gets the node content.
 */
char *kxml_node_content(const kxml_node_t *node);
void kxml_node_content_free(char *str);


/** @brief Gets an attribute by name.
 *
 * May return NULL if the attribute is not found
 */
char *kxml_node_attr(const kxml_node_t *node, const char *attrname);
void kxml_node_attr_free(char *str);


/** @brief XML-helper
 */
bool_t kxml_load_scheme(kscheme_t *scheme, const char *xml_path,
	faux_error_t *error);


/** @brief Typical XML parser functions
 */
bool_t kxml_plugin_init(kdb_t *db);
bool_t kxml_plugin_fini(kdb_t *db);
bool_t kxml_plugin_load_scheme(kdb_t *db, kscheme_t *scheme);


#endif // _klish_kxml_h
