#ifndef _lub_conv_h
#define _lub_conv_h

#include "lub/c_decl.h"

_BEGIN_C_DECL

int lub_conv_atol(const char *str, long int *val, int base);
int lub_conv_atoul(const char *str, unsigned long int *val, int base);
int lub_conv_atos(const char *str, short *val, int base);
int lub_conv_atous(const char *str, unsigned short *val, int base);
int lub_conv_atoi(const char *str, int *val, int base);
int lub_conv_atoui(const char *str, unsigned int *val, int base);

_END_C_DECL

#endif

