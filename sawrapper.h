#ifndef _SAWRAPPER_H
#define _SAWRAPPER_H

#ifndef COMPILE_TEST
#include "divsufsort.h"
#else
typedef unsigned char sauchar_t;
typedef int saidx_t;
typedef int saint_t;
#define divsufsort(_T, _SA, _n) ((saint_t) -1)
#endif

#include "bwscompat.h"

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
