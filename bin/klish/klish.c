#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <faux/faux.h>
#include <faux/str.h>

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"


int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;

	// Parse command line options
	opts = opts_init();
	if (opts_parse(argc, argv, opts))
		goto err;

	retval = 0;

err:
	opts_free(opts);

	return retval;
}
