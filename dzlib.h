/*
 * See Copyright Notice in bwslib.h
 */

#ifndef _DZLIB_H
#define _DZLIB_H

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <stdio.h>
#include "bwslib.h"

#define DZ_RET_EXISTING    1
#define DZ_RET_OK          0
#define DZ_RET_INV_ARG     -1
#define DZ_RET_MEM_ERR     -2
#define DZ_RET_MALFORM     -3
#define DZ_RET_IO_ERR      -4
#define DZ_RET_TOO_LARGE   -5
#define DZ_RET_ZLIB_ERR    -10

#define DZ_CHUNK_SIZE       58315
#define DZ_DEFLATE_BUF_SIZE 65535

extern int dzip_compress(const char *fname, int force);
extern int dzip_decompress(const char *fname, int force);

extern bw_file_t *bw_file_new_from_dzip_fp(FILE *fp, int *pret, int *perr);

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
