#include "faux/ini.h"

const unsigned char testc_version_major = 1;
const unsigned char testc_version_minor = 0;

const char *testc_module[][2] = {
	{"testc_faux_ini_good", "INI subsystem good"},
	{"testc_faux_ini_bad", "INI bad"},
	{"testc_faux_ini_signal", "Interrupted by signal"},
	{NULL, NULL}
	};
