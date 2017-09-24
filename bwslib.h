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
#define BWS_FLAG_LOAD_RANKC 4
#define BWS_FLAG_MMAP       8

typedef struct _csaidx_t csaidx_t;
typedef struct _bwsidx_t bwsidx_t;

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

typedef saidx_t (*get_saidx_f)(saidx_t *base, int idx, unsigned int k);
#define GET_SAIDX(_struct, _field, _idx) ((_struct).get ?\
                                          (_struct).get((_struct)._field,\
                                                        _idx, (_struct).k)\
                                          : (_struct)._field[_idx])
typedef unsigned short (*get_ushort_f)(unsigned short *base, int idx);
#define GET_U16(_struct, _field, _idx) ((_struct).get16 ?\
                                        (_struct).get16((_struct)._field,\
                                                        _idx), (_struct).k)\
                                         : (_struct)._field[_idx])

struct _csaidx_t {
    saidx_t C[CSA_SIGMA];
    saidx_t K[CSA_SIGMA + 2];
    sauchar_t AtoC[CSA_SIGMA];
    int CtoA[CSA_SIGMA];
    saidx_t *SA;
    saidx_t *ISA;
    saidx_t d;
    saidx_t d2;
    saidx_t n;
    saidx_t n_sub_1_div_d2;
    unsigned int k;
    unsigned int s;
    int m;
    int fd;
    long len;
    void *map;
    get_saidx_f get;
    struct {
        saidx_t isa_blkid;
        saidx_t isa_count;
        saidx_t *isa_cache;
    } cache;
};

extern int bws_load_csa_index(csaidx_t *pindex, int flags, FILE *fp);
extern int bws_free_csa_index(csaidx_t *pindex);

struct _bwsidx_t {
    unsigned short *lRankC;
    saidx_t *lbRankC;
    saidx_t n;
    saidx_t last;
    saidx_t l;
    saidx_t lb;
    saidx_t logLB;
    unsigned int k;
    int m;
    int fd;
    long len;
    void *map;
    get_saidx_f get;
    get_ushort_f get16;
};

extern int bws_load_bws_index(bwsidx_t *pindex, int flags, FILE *fp);
extern int bws_free_bws_index(bwsidx_t *pindex);

typedef struct _bw_file_t bw_file_t;
typedef void (*bw_file_seek_set_f)(bw_file_t *bwfp, saidx_t pos);
typedef int (*bw_file_get_char_f)(bw_file_t *bwfp);
typedef void (*bw_file_close_f)(bw_file_t *bwfp);

struct _bw_file_t {
    void *tag;
    bw_file_seek_set_f  seek;
    bw_file_get_char_f  getc;
    bw_file_close_f     close;
};

extern bw_file_t *bw_file_new_from_fp(FILE *fp, int flags);

extern saidx_t bws_rankc(csaidx_t *pcsa, bwsidx_t *pbws,
                         bw_file_t *fpbw,
                         saidx_t i, int c);

extern saidx_t bws_lf(csaidx_t *pcsa, bwsidx_t *pbws,
                      bw_file_t *fpbw,
                      saidx_t i);

extern saidx_t bws_sa(csaidx_t *pcsa, bwsidx_t *pbws,
                      bw_file_t *fpbw,
                      saidx_t i);

extern saidx_t bws_isa(csaidx_t *pcsa, bwsidx_t *pbws,
                       bw_file_t *fpbw,
                       saidx_t i);

extern saidx_t bws_isa_r(csaidx_t *pcsa, bwsidx_t *pbws,
                         bw_file_t *fpbw,
                         saidx_t i);

extern int bws_t(csaidx_t *pcsa, bwsidx_t *pbws,
                 bw_file_t *fpbw,
                 saidx_t i);

extern int bws_bw(csaidx_t *pcsa, bwsidx_t *pbws,
                  bw_file_t *fpbw,
                  saidx_t i);

extern sauchar_t *bws_substr(csaidx_t *pcsa, bwsidx_t *pbws,
                             bw_file_t *fpbw,
                             saidx_t i, int len);

extern int bws_search(csaidx_t *pcsa, bwsidx_t *pbws,
                      bw_file_t *fpbw,
                      const char *key, int klen,
                      saidx_t *pleft, saidx_t *pright);
#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
