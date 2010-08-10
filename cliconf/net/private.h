#ifndef net_private_h
#define net_private_h

#include "cliconf/net.h"

struct conf_client_s {
	int sock;
	char *path;
};

#endif
