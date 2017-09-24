/*
 * See Copyright Notice in bwslib.h
 */

#include "bwslib.h"
#include <stdlib.h>

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
  D = pindex->AtoC;
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

  /* Inverse BW transform. */
  for(c = 0, i = 0; c < pindex->s; ++c) {
    p = C[c];
    if(0 < p) {
      C[c] = i;
      i += p;
    }
  }
  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
  /*
   * F[i] = T[SA[i]] => T[i] = F[ISA[i]]
   * LF[i] = ISA[SA[i] - 1] => FL[i] = ISA[SA[i] + 1]
   * T[i + 1] = F[FL[ISA[i]]]
   * FL == Psi
   */
#ifdef _OPENMP
#pragma omp parallel for default(shared) private(i, j, p)
#endif
  for (j = 0; j <= pindex->n_sub_1_div_d2; ++j)
  {
      int k = (j + 1) * pindex->d2;
      if (n < k)
      {
          k = n;
      }
      for (i = j * pindex->d2, p = GET_SAIDX(*pindex, ISA, j); i < k; ++i)
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

/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
