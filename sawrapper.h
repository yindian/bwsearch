#ifndef _SAWRAPPER_H
#define _SAWRAPPER_H

#ifndef COMPILE_TEST
#include "divsufsort.h"
#else
typedef unsigned char sauchar_t;
typedef int saidx_t;
#endif

#ifndef PRId64
#ifndef _MSC_VER
#error "Compiler not supported"
#endif
#define PRId64 "I64d"
typedef __int64 int64_t;
#endif

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
