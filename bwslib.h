/******************************************************************************
 * Copyright (C) 2017 Yin Dian
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef _BWSLIB_H
#define _BWSLIB_H

#include "csacompat.h"
#include <stdio.h>

#define BWS_RET_OK          0
#define BWS_RET_INV_ARG     -1
#define BWS_RET_MEM_ERR     -2
#define BWS_RET_MALFORM     -3

#define BWS_FLAG_LOAD_SA    1
#define BWS_FLAG_LOAD_ISA   2

typedef struct _csaidx_t csaidx_t;

/*
 * {======================================================================
 * for divsufsort compatibility
 * =======================================================================
 */

/*- Datatypes -*/
#ifndef SAUCHAR_T
#define SAUCHAR_T
typedef unsigned char sauchar_t;
#endif /* SAUCHAR_T */
#ifndef SAINT_T
#define SAINT_T
typedef int saint_t;
#endif /* SAINT_T */
#ifndef SAIDX_T
#define SAIDX_T
typedef int saidx_t;
#endif /* SAIDX_T */

extern saint_t
bws_inverse_bw_transform(const sauchar_t *T, sauchar_t *U, saidx_t *A,
                     saidx_t n, saidx_t idx,
                     csaidx_t *pindex);

/* }====================================================================== */

struct _csaidx_t {
    saidx_t C[CSA_SIGMA];
    sauchar_t D[CSA_SIGMA];
    saidx_t *SA;
    saidx_t *ISA;
    saidx_t d;
    saidx_t d2;
    saidx_t n;
    unsigned int k;
    unsigned int s;
    int m;
};

extern int bws_load_csa_index(csaidx_t *pindex, int flags, FILE *fp);
extern int bws_free_csa_index(csaidx_t *pindex);

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
