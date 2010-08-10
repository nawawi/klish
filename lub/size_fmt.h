/*
 * size_fmt.h
 *
 * used to format size_t values within a printf-like format string
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#  if (SIZEOF_LONG != SIZEOF_INT)
#    define SIZE_FMT "lu"
#  else
#    define SIZE_FMT "u"
#  endif
#else /* not HAVE_CONFIG_H */
#  define SIZE_FMT "u"
#endif /* not HAVE_CONFIG_H */
