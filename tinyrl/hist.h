#ifndef _tinyrl_hist_h
#define _tinyrl_hist_h


#include <faux/faux.h>
#include <faux/list.h>


typedef struct hist_s hist_t;
typedef faux_list_node_t hist_node_t;


C_DECL_BEGIN

hist_t *hist_new(size_t stifle);
void hist_free(hist_t *hist);

void hist_add(hist_t *hist, const char *line);
void hist_clear(hist_t *hist);

void hist_pos_reset(hist_t *hist);
const char *hist_pos(hist_t *hist);
const char *hist_pos_up(hist_t *hist);
const char *hist_pos_down(hist_t *hist);

extern int hist_save(const hist_t *hist, const char *fname);
extern int hist_restore(hist_t *hist, const char *fname);

C_DECL_END

#endif // _tinyrl_hist_h
