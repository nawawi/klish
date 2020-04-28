#include <stdlib.h>
#include <stdio.h>

int testc_faux_ini_good(void) {

	char *path = NULL;

	path = getenv("FAUX_INI_PATH");
	if (path)
		printf("Env var is [%s]\n", path);
	return 0;
}


int testc_faux_ini_bad(void) {

	return -1;
}

