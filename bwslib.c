/*
 * See Copyright Notice in bwslib.h
 */

#include "bwslib.h"
#ifndef COMPILE_TEST
#include <inttypes.h>
#else
#include "bwscompat.h"
#endif
#include <stdlib.h>
#include <string.h>

static int64_t readint(int k, FILE *f)
{
    int64_t x;
    int i;
    x = 0;
    for (i = 0; i < k; i++)
    {
        x += (int64_t) fgetc(f) << (8*i);
    }
    return x;
}

/* Binary search for inverse bwt. */
static
saidx_t
binarysearch_lower(const saidx_t *A, saidx_t size, saidx_t value) {
  saidx_t half, i;
  for(i = 0, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    if(A[i + half] < value) {
      i += half + 1;
      half -= (size & 1) ^ 1;
    }
  }
  return i;
}

saint_t
bws_inverse_bw_transform(const sauchar_t *T, sauchar_t *U, saidx_t *A,
                     saidx_t n, saidx_t idx,
                     csaidx_t *pindex) {
  saidx_t *B;
  saidx_t i, j, p;
  saint_t c;
  saidx_t *ISA;
  saidx_t *C;
  sauchar_t *D;
  saidx_t d;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (idx < 0) ||
     (n < idx) || ((0 < n) && (idx == 0))) {
    return -1;
  }
  if (pindex == NULL)
  {
      return -3;
  }
  ISA = pindex->ISA;
  C = pindex->C;
  D = pindex->D;
  d = pindex->m;
  if((ISA == NULL) || (C == NULL) || (D == NULL) || (d < 0) || (d > pindex->s))
  {
      return -3;
  }
  if(n <= 1) { return 0; }

  if((B = A) == NULL) {
    /* Allocate n*sizeof(saidx_t) bytes of memory. */
    if((B = (saidx_t *)malloc((size_t)n * sizeof(saidx_t))) == NULL) { return -2; }
  }

#ifdef DEBUG
  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
#endif
  /* Inverse BW transform. */
  for(c = 0, i = 0; c < pindex->s; ++c) {
    p = C[c];
    if(0 < p) {
      C[c] = i;
      i += p;
    }
  }
#ifdef DEBUG
  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
#endif
  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
#ifdef DEBUG
  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
#endif
  /*
   * F[i] = T[SA[i]] => T[i] = F[ISA[i]]
   * LF[i] = ISA[SA[i] - 1] => FL[i] = ISA[SA[i] + 1]
   * T[i + 1] = F[FL[ISA[i]]]
   */
#ifdef _OPENMP
#pragma omp parallel for default(shared) private(i, j, p)
#endif
  for (j = 0; j <= (n - 1) / pindex->d2; ++j)
  {
      int k = (j + 1) * pindex->d2;
      if (n < k)
      {
          k = n;
      }
      for (i = j * pindex->d2, p = ISA[j]; i < k; ++i)
      {
          U[i] = D[binarysearch_lower(C, d, p)];
          p = B[p - 1];
      }
  }

  if(A == NULL) {
    /* Deallocate memory. */
    free(B);
  }

  return 0;
}

int bws_load_csa_index(csaidx_t *pindex, int flags, FILE *fp)
{
    int ret, err;
    if (!pindex || !fp)
    {
        return BWS_RET_INV_ARG;
    }
    memset(pindex, 0, sizeof(csaidx_t));
    ret = BWS_RET_OK;
    err = 0;
#define CHECK_COND(_cond) \
    {\
        if (!(_cond) || ferror(fp))\
        {\
            ret = BWS_RET_MALFORM;\
            err = __LINE__;\
            break;\
        }\
    }
    do
    {
        int i;
        CHECK_COND(readint(4, fp) == CSA_VERSION);
        CHECK_COND(readint(1, fp) == CSA_ID_HEADER);
        CHECK_COND(readint(1, fp) == CSA_K);
        CHECK_COND((pindex->n = readint(CSA_K, fp)) >= 0);
        CHECK_COND((pindex->d = readint(CSA_K, fp)) >= CSA_D);
        CHECK_COND((pindex->d2 = readint(CSA_K, fp)) >= CSA_D2);
        CHECK_COND((pindex->s = readint(CSA_K, fp)) <= CSA_SIGMA);
        memset(pindex->C, 0, sizeof(pindex->C));
        memset(pindex->D, 0, sizeof(pindex->D));
        CHECK_COND((pindex->m = (int) readint(CSA_K, fp)) >= 0);
        for (i = 0; i < pindex->m; i++)
        {
            CHECK_COND((pindex->D[i] = (sauchar_t) readint(1, fp)) >= 0);
            CHECK_COND((pindex->C[pindex->D[i]]
                        = (saidx_t) readint(CSA_K, fp)) > 0);
        }
        CHECK_COND(readint(1, fp) == CSA_ID_SA);
        CHECK_COND(readint(1, fp) == CSA_K);
        CHECK_COND(readint(CSA_K, fp) == pindex->d);
        if ((flags & BWS_FLAG_LOAD_SA))
        {
            pindex->SA = (saidx_t *) malloc((pindex->n / pindex->d + 1)
                                            * sizeof(saidx_t));
            if (!pindex->SA)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i <= pindex->n / pindex->d; i++)
            {
                CHECK_COND((pindex->SA[i] = (saidx_t) readint(CSA_K, fp)) > 0);
                if (err)
                {
                    break;
                }
            }
        }
        else
        {
            fseeko(fp, (pindex->n / pindex->d + 1) * CSA_K, SEEK_CUR);
        }
        CHECK_COND(readint(1, fp) == CSA_ID_ISA);
        CHECK_COND(readint(1, fp) == CSA_K);
        CHECK_COND(readint(CSA_K, fp) == pindex->d2);
        if ((flags & BWS_FLAG_LOAD_ISA))
        {
            pindex->ISA = (saidx_t *) malloc(((pindex->n - 1) / pindex->d2 + 1)
                                             * sizeof(saidx_t));
            if (!pindex->ISA)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i <= (pindex->n - 1) / pindex->d2; i++)
            {
                CHECK_COND((pindex->ISA[i] = (saidx_t) readint(CSA_K, fp)) > 0);
                if (err)
                {
                    break;
                }
            }
        }
    } while (0);
    if (err)
    {
        fprintf(stderr, "Error in %s:%d\n", __FILE__, err);
    }
    return ret;
}

int bws_free_csa_index(csaidx_t *pindex)
{
    if (!pindex)
    {
        return BWS_RET_INV_ARG;
    }
    if (pindex->SA)
    {
        free(pindex->SA);
    }
    if (pindex->ISA)
    {
        free(pindex->ISA);
    }
    return BWS_RET_OK;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
