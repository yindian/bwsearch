/*
 * See Copyright Notice in bwslib.h
 */

#ifndef _BWSCOMPAT_H
#define _BWSCOMPAT_H

#ifndef PRId64
#ifndef _MSC_VER
#error "Compiler not supported"
#endif
#define PRId64 "I64d"
typedef __int64 int64_t;
#endif

#if defined(_MSC_VER) && (!defined(ftello) || !defined(fseeko))
#define ftello(_fp) ((off_t) _ftelli64(_fp))
#define fseeko _fseeki64
#endif

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
