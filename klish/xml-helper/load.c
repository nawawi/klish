/** @file load.c
 * @brief Common part for XML parsing.
 *
 * Different XML parsing engines can provide a functions in a form of
 * standardized API. This code uses this API and parses XML to kscheme.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/error.h>
#include <klish/kscheme.h>
#include <klish/ischeme.h>
#include <klish/kxml.h>

#define TAG "XML"


typedef bool_t (kxml_process_fn)(const kxml_node_t *element,
	void *parent, faux_error_t *error);

static kxml_process_fn
	process_action,
	process_param,
	process_command,
	process_view,
	process_ptype,
	process_plugin,
	process_klish,
	process_entry;

// Different TAGs types
typedef enum {
	KTAG_NONE,
	KTAG_ACTION,
	KTAG_PARAM,
	KTAG_SWITCH, // PARAM alias
	KTAG_MULTI, // PARAM alias
	KTAG_COMMAND,
	KTAG_FILTER,
	KTAG_VIEW,
	KTAG_PTYPE,
	KTAG_PLUGIN,
	KTAG_KLISH,
	KTAG_ENTRY,
	KTAG_COND,
	KTAG_COMPL,
	KTAG_HELP,
	KTAG_PROMPT,
	KTAG_MAX,
} ktags_e;

static const char * const kxml_tags[] = {
	NULL,
	"ACTION",
	"PARAM",
	"SWITCH",
	"MULTI",
	"COMMAND",
	"FILTER",
	"VIEW",
	"PTYPE",
	"PLUGIN",
	"KLISH",
	"ENTRY",
	"COND",
	"COMPL",
	"HELP",
	"PROMPT",
};

static kxml_process_fn *kxml_handlers[] = {
	NULL,
	process_action,
	process_param,
	process_param,
	process_param,
	process_command,
	process_command,
	process_view,
	process_ptype,
	process_plugin,
	process_klish,
	process_entry,
	process_command,
	process_command,
	process_command,
	process_command,
};


static const char *kxml_tag_name(ktags_e tag)
{
	if ((KTAG_NONE == tag) || (tag >= KTAG_MAX))
		return "NONE";

	return kxml_tags[tag];
}


static ktags_e kxml_node_tag(const kxml_node_t *node)
{
	ktags_e tag = KTAG_NONE;
	char *name = NULL;

	if (!node)
		return KTAG_NONE;

	if (kxml_node_type(node) != KXML_NODE_ELM)
		return KTAG_NONE;
	name = kxml_node_name(node);
	if (!name)
		return KTAG_NONE; // Strange case
	for (tag = (KTAG_NONE + 1); tag < KTAG_MAX; tag++) {
		if (faux_str_casecmp(name, kxml_tags[tag]) == 0)
			break;
	}
	kxml_node_name_free(name);
	if (tag >= KTAG_MAX)
		return KTAG_NONE;

	return tag;
}


static kxml_process_fn *kxml_node_handler(const kxml_node_t *node)
{
	return kxml_handlers[kxml_node_tag(node)];
}


/** @brief Reads an element from the XML stream and processes it.
 */
static bool_t process_node(const kxml_node_t *node, void *parent, faux_error_t *error)
{
	kxml_process_fn *handler = NULL;

	// Process only KXML_NODE_ELM. Don't process other types like:
	// KXML_NODE_DOC,
	// KXML_NODE_TEXT,
	// KXML_NODE_ATTR,
	// KXML_NODE_COMMENT,
	// KXML_NODE_PI,
	// KXML_NODE_DECL,
	// KXML_NODE_UNKNOWN,
	if (kxml_node_type(node) != KXML_NODE_ELM)
		return BOOL_TRUE;

	handler = kxml_node_handler(node);

	if (!handler) { // Unknown element
		faux_error_sprintf(error,
			TAG": Unknown tag \"%s\"", kxml_node_name(node));
		return BOOL_FALSE;
	}

#ifdef KXML_DEBUG
	printf("kxml: Tag \"%s\"\n", kxml_node_name(node));
#endif

	return handler(node, parent, error);
}


static bool_t kxml_load_file(kscheme_t *scheme, const char *filename,
	faux_error_t *error)
{
	kxml_doc_t *doc = NULL;
	kxml_node_t *root = NULL;
	bool_t r = BOOL_FALSE;

	if (!scheme)
		return BOOL_FALSE;
	if (!filename)
		return BOOL_FALSE;

#ifdef KXML_DEBUG
	printf("kxml: Processing XML file \"%s\"\n", filename);
#endif

	doc = kxml_doc_read(filename);
	if (!kxml_doc_is_valid(doc)) {
/*		int errcaps = kxml_doc_error_caps(doc);
		printf("Unable to open file '%s'", filename);
		if ((errcaps & kxml_ERR_LINE) == kxml_ERR_LINE)
			printf(", at line %d", kxml_doc_err_line(doc));
		if ((errcaps & kxml_ERR_COL) == kxml_ERR_COL)
			printf(", at column %d", kxml_doc_err_col(doc));
		if ((errcaps & kxml_ERR_DESC) == kxml_ERR_DESC)
			printf(", message is %s", kxml_doc_err_msg(doc));
		printf("\n");
*/		kxml_doc_release(doc);
		return BOOL_FALSE;
	}
	root = kxml_doc_root(doc);
	r = process_node(root, scheme, error);
	kxml_doc_release(doc);
	if (!r) {
		faux_error_sprintf(error, TAG": Illegal file %s", filename);
		return BOOL_FALSE;
	}

	return BOOL_TRUE;
}


/** @brief Default path to get XML files from.
 */
static const char *default_path = "/etc/klish;~/.klish";
static const char *path_separators = ":;";


bool_t kxml_load_scheme(kscheme_t *scheme, const char *xml_path,
	faux_error_t *error)
{
	char *path = NULL;
	char *fn = NULL;
	char *saveptr = NULL;
	bool_t ret = BOOL_TRUE;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;

	// Use the default path if xml path is not specified.
	// Dup is needed because sring will be tokenized but
	// the xml_path is must be const.
	if (!xml_path)
		path = faux_str_dup(default_path);
	else
		path = faux_str_dup(xml_path);

#ifdef KXML_DEBUG
	printf("kxml: Loading scheme \"%s\"\n", path);
#endif

	// Loop through each directory
	for (fn = strtok_r(path, path_separators, &saveptr);
		fn; fn = strtok_r(NULL, path_separators, &saveptr)) {
		DIR *dir = NULL;
		struct dirent *entry = NULL;
		char *realpath = NULL;

		// Expand tilde. Tilde must be the first symbol.
		realpath = faux_expand_tilde(fn);

		// Regular file
		if (faux_isfile(realpath)) {
			if (!kxml_load_file(scheme, realpath, error))
				ret = BOOL_FALSE;
			faux_str_free(realpath);
			continue;
		}

		// Search this directory for any XML files
#ifdef KXML_DEBUG
		printf("kxml: Processing XML dir \"%s\"\n", realpath);
#endif
		dir = opendir(realpath);
		if (!dir) {
			faux_str_free(realpath);
			continue;
		}
		for (entry = readdir(dir); entry; entry = readdir(dir)) {
			const char *extension = strrchr(entry->d_name, '.');
			char *filename = NULL;

			// Check the filename
			if (!extension || strcmp(".xml", extension))
				continue;
			filename = faux_str_sprintf("%s/%s", realpath, entry->d_name);
			if (!kxml_load_file(scheme, filename, error))
				ret = BOOL_FALSE;
			faux_str_free(filename);
		}
		closedir(dir);
		faux_str_free(realpath);
	}

	faux_str_free(path);

	return ret;
}


/** @brief Iterate through element's children.
 */
static bool_t process_children(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	const kxml_node_t *node = NULL;

	while ((node = kxml_node_next_child(element, node)) != NULL) {
		bool_t res = BOOL_FALSE;
		res = process_node(node, parent, error);
		if (!res)
			return res;
	}

	return BOOL_TRUE;
}


static bool_t process_klish(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	return process_children(element, parent, error);
}


static bool_t process_plugin(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	iplugin_t iplugin = {};
	kplugin_t *plugin = NULL;
	bool_t res = BOOL_FALSE;
	ktags_e parent_tag = kxml_node_tag(kxml_node_parent(element));

	if (parent_tag != KTAG_KLISH) {
		faux_error_sprintf(error,
			TAG": Tag \"%s\" can't contain PLUGIN tag",
			kxml_tag_name(parent_tag));
		return BOOL_FALSE;
	}

	iplugin.name = kxml_node_attr(element, "name");
	iplugin.id = kxml_node_attr(element, "id");
	iplugin.file = kxml_node_attr(element, "file");
	iplugin.conf = kxml_node_content(element);

	plugin = iplugin_load(&iplugin, error);
	if (!plugin)
		goto err;

	if (!kscheme_add_plugins((kscheme_t *)parent, plugin)) {
		faux_error_sprintf(error, TAG": Can't add PLUGIN \"%s\". "
			"Probably duplication",
			kplugin_name(plugin));
		kplugin_free(plugin);
		goto err;
	}

	if (!process_children(element, plugin, error))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(iplugin.name);
	kxml_node_attr_free(iplugin.id);
	kxml_node_attr_free(iplugin.file);
	kxml_node_content_free(iplugin.conf);

	return res;
}


static bool_t process_action(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	iaction_t iaction = {};
	kaction_t *action = NULL;
	bool_t res = BOOL_FALSE;
	ktags_e parent_tag = kxml_node_tag(kxml_node_parent(element));
	kentry_t *parent_entry = (kentry_t *)parent;

	iaction.sym = kxml_node_attr(element, "sym");
	iaction.lock = kxml_node_attr(element, "lock");
	iaction.interrupt = kxml_node_attr(element, "interrupt");
	iaction.interactive = kxml_node_attr(element, "interactive");
	iaction.exec_on = kxml_node_attr(element, "exec_on");
	iaction.update_retcode = kxml_node_attr(element, "update_retcode");
	iaction.permanent = kxml_node_attr(element, "permanent");
	iaction.sync = kxml_node_attr(element, "sync");
	iaction.script = kxml_node_content(element);

	action = iaction_load(&iaction, error);
	if (!action)
		goto err;

	if (	(KTAG_ENTRY != parent_tag) &&
		(KTAG_COMMAND != parent_tag) &&
		(KTAG_FILTER != parent_tag) &&
		(KTAG_COND != parent_tag) &&
		(KTAG_COMPL != parent_tag) &&
		(KTAG_HELP != parent_tag) &&
		(KTAG_PROMPT != parent_tag) &&
		(KTAG_PTYPE != parent_tag)) {
		faux_error_sprintf(error,
			TAG": Tag \"%s\" can't contain ACTION tag",
			kxml_tag_name(parent_tag));
		kaction_free(action);
		goto err;
	}
	if (!kentry_add_actions(parent_entry, action)) {
		faux_error_sprintf(error,
			TAG": Can't add ACTION #%d to ENTRY \"%s\". "
			"Probably duplication",
			kentry_actions_len(parent_entry) + 1,
			kentry_name(parent_entry));
		kaction_free(action);
		goto err;
	}

	if (!process_children(element, action, error))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(iaction.sym);
	kxml_node_attr_free(iaction.lock);
	kxml_node_attr_free(iaction.interrupt);
	kxml_node_attr_free(iaction.interactive);
	kxml_node_attr_free(iaction.exec_on);
	kxml_node_attr_free(iaction.update_retcode);
	kxml_node_attr_free(iaction.permanent);
	kxml_node_attr_free(iaction.sync);
	kxml_node_content_free(iaction.script);

	return res;
}


static kentry_t *add_entry_to_hierarchy(const kxml_node_t *element, void *parent,
	ientry_t *ientry, faux_error_t *error)
{
	kentry_t *entry = NULL;
	ktags_e tag = kxml_node_tag(element);
	ktags_e parent_tag = kxml_node_tag(kxml_node_parent(element));

	assert(ientry);

	// Parent is mandatory field
	if (!parent) {
		faux_error_sprintf(error,
			TAG": Broken parent object for entry \"%s\"",
			ientry->name);
		return NULL;
	}

	if ((parent_tag == KTAG_ACTION) ||
//		(parent_tag == KTAG_HOTKEY) ||
		(parent_tag == KTAG_PLUGIN)) {
		faux_error_sprintf(error,
			TAG": Tag \"%s\" can't contain %s tag \"%s\"",
			kxml_tag_name(parent_tag),
			kxml_tag_name(tag), ientry->name);
		return NULL;
	}

	// High level ENTRY
	if (KTAG_KLISH == parent_tag) {
		kscheme_t *scheme = (kscheme_t *)parent;

		// Does such ENTRY already exist
		entry = kscheme_find_entry(scheme, ientry->name);
		if (entry) {
			if (!ientry_parse(ientry, entry, error))
				return NULL;

		} else { // New entry object
			entry = ientry_load(ientry, error);
			if (!entry)
				return NULL;
			if (!kscheme_add_entrys(scheme, entry)) {
				faux_error_sprintf(error, TAG": Can't add entry \"%s\". "
					"Probably duplication",
					kentry_name(entry));
				kentry_free(entry);
				return NULL;
			}
		}

	// ENTRY within ENTRY
	} else {
		kentry_t *parent_entry = (kentry_t *)parent;

		// Does such ENTRY already exist
		entry = kentry_find_entry(parent_entry, ientry->name);
		if (entry) {
			if (!ientry_parse(ientry, entry, error))
				return NULL;
		} else { // New entry object
			entry = ientry_load(ientry, error);
			if (!entry)
				return NULL;
			kentry_set_parent(entry, parent_entry);
			if (!kentry_add_entrys(parent_entry, entry)) {
				faux_error_sprintf(error, TAG": Can't add entry \"%s\". "
					"Probably duplication",
					kentry_name(entry));
				kentry_free(entry);
				return NULL;
			}
		}
	}

	return entry;
}


static bool_t process_entry(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	ientry_t ientry = {};
	kentry_t *entry = NULL;
	bool_t res = BOOL_FALSE;

	// Mandatory entry name
	ientry.name = kxml_node_attr(element, "name");
	if (!ientry.name) {
		faux_error_sprintf(error, TAG": entry without name");
		return BOOL_FALSE;
	}
	ientry.help = kxml_node_attr(element, "help");
	ientry.container = kxml_node_attr(element, "container");
	ientry.mode = kxml_node_attr(element, "mode");
	ientry.purpose = kxml_node_attr(element, "purpose");
	ientry.min = kxml_node_attr(element, "min");
	ientry.max = kxml_node_attr(element, "max");
	ientry.ref = kxml_node_attr(element, "ref");
	ientry.value = kxml_node_attr(element, "value");
	ientry.restore = kxml_node_attr(element, "restore");
	ientry.order = kxml_node_attr(element, "order");
	ientry.filter = kxml_node_attr(element, "filter");

	if (!(entry = add_entry_to_hierarchy(element, parent, &ientry, error)))
		goto err;

	if (!process_children(element, entry, error))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(ientry.name);
	kxml_node_attr_free(ientry.help);
	kxml_node_attr_free(ientry.container);
	kxml_node_attr_free(ientry.mode);
	kxml_node_attr_free(ientry.purpose);
	kxml_node_attr_free(ientry.min);
	kxml_node_attr_free(ientry.max);
	kxml_node_attr_free(ientry.ref);
	kxml_node_attr_free(ientry.value);
	kxml_node_attr_free(ientry.restore);
	kxml_node_attr_free(ientry.order);
	kxml_node_attr_free(ientry.filter);

	return res;
}


static bool_t process_view(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	ientry_t ientry = {};
	kentry_t *entry = NULL;
	bool_t res = BOOL_FALSE;

	// Mandatory VIEW name
	ientry.name = kxml_node_attr(element, "name");
	if (!ientry.name) {
		faux_error_sprintf(error, TAG": VIEW without name");
		return BOOL_FALSE;
	}
	ientry.help = kxml_node_attr(element, "help");
	ientry.container = "true";
	ientry.mode = "switch";
	ientry.purpose = "common";
	ientry.min = "1";
	ientry.max = "1";
	ientry.ref = kxml_node_attr(element, "ref");
	ientry.value = NULL;
	ientry.restore = "false";
	ientry.order = "false";
	ientry.filter = "false";

	if (!(entry = add_entry_to_hierarchy(element, parent, &ientry, error)))
		goto err;

	if (!process_children(element, entry, error))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(ientry.name);
	kxml_node_attr_free(ientry.help);
	kxml_node_attr_free(ientry.ref);

	return res;
}


static bool_t process_ptype(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	ientry_t ientry = {};
	kentry_t *entry = NULL;
	bool_t res = BOOL_FALSE;
	bool_t is_name = BOOL_FALSE;

	// Mandatory PTYPE name or reference
	ientry.name = kxml_node_attr(element, "name");
	ientry.ref = kxml_node_attr(element, "ref");
	if (ientry.name) {
		is_name = BOOL_TRUE;
	} else {
		if (!ientry.ref) {
			faux_error_sprintf(error,
				TAG": PTYPE without name or reference");
			return BOOL_FALSE;
		}
		ientry.name = "__ptype";
	}
	ientry.help = kxml_node_attr(element, "help");
	ientry.container = "true";
	ientry.mode = "sequence";
	ientry.purpose = "ptype";
	ientry.min = "1";
	ientry.max = "1";
	ientry.value = kxml_node_attr(element, "value");
	ientry.restore = "false";
	ientry.order = "true";
	ientry.filter = "false";

	if (!(entry = add_entry_to_hierarchy(element, parent, &ientry, error)))
		goto err;

	if (!process_children(element, entry, error))
		goto err;

	res = BOOL_TRUE;
err:
	if (is_name)
		kxml_node_attr_free(ientry.name);
	kxml_node_attr_free(ientry.help);
	kxml_node_attr_free(ientry.ref);
	kxml_node_attr_free(ientry.value);

	return res;
}


static bool_t process_param(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	ientry_t ientry = {};
	kentry_t *entry = NULL;
	bool_t res = BOOL_FALSE;
	ktags_e parent_tag = kxml_node_tag(kxml_node_parent(element));
	kentry_t *parent_entry = (kentry_t *)parent;
	kentry_t *entry_add_to = parent_entry;

	// Mandatory PARAM name
	ientry.name = kxml_node_attr(element, "name");
	if (!ientry.name) {
		faux_error_sprintf(error, TAG": PARAM without name");
		return BOOL_FALSE;
	}
	ientry.help = kxml_node_attr(element, "help");
	ientry.container = kxml_node_attr(element, "container");
	ientry.mode = kxml_node_attr(element, "mode");
	ientry.purpose = "common";
	ientry.min = kxml_node_attr(element, "min");
	ientry.max = kxml_node_attr(element, "max");
	ientry.ref = kxml_node_attr(element, "ref");
	ientry.value = kxml_node_attr(element, "value");
	ientry.restore = "false";
	ientry.order = kxml_node_attr(element, "order");
	ientry.filter = "false";

	entry = ientry_load(&ientry, error);
	if (!entry)
		goto err;

	if ((KTAG_COMMAND != parent_tag) &&
		(KTAG_PARAM != parent_tag) &&
		(KTAG_ENTRY != parent_tag) &&
		(KTAG_SWITCH != parent_tag) &&
		(KTAG_MULTI != parent_tag) &&
		(KTAG_COND != parent_tag) &&
		(KTAG_COMPL != parent_tag) &&
		(KTAG_HELP != parent_tag) &&
		(KTAG_PTYPE != parent_tag)) {
		faux_error_sprintf(error,
			TAG": Tag \"%s\" can't contain PARAM tag",
			kxml_tag_name(parent_tag));
		kentry_free(entry);
		goto err;
	}

	// Add newly created entry to special container in 'sequence' mode if
	// parent entry can has 'switch' mode.
	if (kentry_mode(parent_entry) == KENTRY_MODE_SWITCH) {
		const char *seq_entry_name = "__sequence";
		kentry_t *seq_entry = kentry_find_entry(parent_entry, seq_entry_name);
		if (!seq_entry) {
			seq_entry = kentry_new(seq_entry_name);
			assert(seq_entry);
			kentry_set_container(seq_entry, BOOL_TRUE);
			kentry_set_mode(seq_entry, KENTRY_MODE_SEQUENCE);
			kentry_add_entrys(parent_entry, seq_entry);
		}
		entry_add_to = seq_entry;
	}
	if (!kentry_add_entrys(entry_add_to, entry)) {
		faux_error_sprintf(error,
			TAG": Can't add PARAM \"%s\" to ENTRY \"%s\". "
			"Probably duplication",
			kentry_name(entry_add_to), kentry_name(entry_add_to));
		kentry_free(entry);
		goto err;
	}

	if (!process_children(element, entry, error))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(ientry.name);
	kxml_node_attr_free(ientry.help);
	kxml_node_attr_free(ientry.container);
	kxml_node_attr_free(ientry.mode);
	kxml_node_attr_free(ientry.min);
	kxml_node_attr_free(ientry.max);
	kxml_node_attr_free(ientry.ref);
	kxml_node_attr_free(ientry.value);
	kxml_node_attr_free(ientry.order);

	return res;
}


// COMMAND, FILTER, COND, COMPL, HELP, PROMPT
static bool_t process_command(const kxml_node_t *element, void *parent,
	faux_error_t *error)
{
	ientry_t ientry = {};
	kentry_t *entry = NULL;
	bool_t res = BOOL_FALSE;
	ktags_e tag = kxml_node_tag(element);
	bool_t is_name = BOOL_FALSE;

	// Mandatory COMMAND name
	ientry.name = kxml_node_attr(element, "name");
	if (ientry.name) {
		is_name = BOOL_TRUE;
	} else {
		switch (tag) {
		case KTAG_COMMAND:
		case KTAG_FILTER:
			faux_error_sprintf(error, TAG": COMMAND without name");
			return BOOL_FALSE;
		case KTAG_COND:
			ientry.name = "__cond";
			break;
		case KTAG_COMPL:
			ientry.name = "__compl";
			break;
		case KTAG_HELP:
			ientry.name = "__help";
			break;
		case KTAG_PROMPT:
			ientry.name = "__prompt";
			break;
		default:
			faux_error_sprintf(error, TAG": Unknown tag");
			return BOOL_FALSE;
		}
	}
	ientry.help = kxml_node_attr(element, "help");
	ientry.container = "false";
	ientry.mode = "sequence";
	// Purpose
	switch (tag) {
	case KTAG_COND:
		ientry.purpose = "cond";
		break;
	case KTAG_COMPL:
		ientry.purpose = "completion";
		break;
	case KTAG_HELP:
		ientry.purpose = "help";
		break;
	case KTAG_PROMPT:
		ientry.purpose = "prompt";
		break;
	default:
		ientry.purpose = "common";
		break;
	}
	ientry.min = "1";
	ientry.max = "1";
	ientry.ref = kxml_node_attr(element, "ref");
	if ((KTAG_FILTER == tag) || (KTAG_COMMAND == tag)) {
		ientry.value = kxml_node_attr(element, "value");
		ientry.restore = kxml_node_attr(element, "restore");
	} else {
		ientry.value = NULL;
		ientry.restore = "false";
	}
	ientry.order = "false";
	// Filter
	if (KTAG_FILTER == tag)
		ientry.filter = "true";
	else
		ientry.filter = "false";

	if (!(entry = add_entry_to_hierarchy(element, parent, &ientry, error)))
		goto err;

	if (!process_children(element, entry, error))
		goto err;

	res = BOOL_TRUE;
err:
	if (is_name)
		kxml_node_attr_free(ientry.name);
	kxml_node_attr_free(ientry.help);
	kxml_node_attr_free(ientry.ref);
	if ((KTAG_FILTER == tag) || (KTAG_COMMAND == tag)) {
		kxml_node_attr_free(ientry.value);
		kxml_node_attr_free(ientry.restore);
	}

	return res;
}




