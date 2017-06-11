#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bwscommon.h"
#include "csacompat.h"
#include "sawrapper.h"
#ifdef _OPENMP
#include <omp.h>
#endif

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

static saint_t
my_inverse_bw_transform(const sauchar_t *T, sauchar_t *U, saidx_t *A,
                     saidx_t n, saidx_t idx,
                     saidx_t *ISA, saidx_t *C, sauchar_t *D, saidx_t d) {
  saidx_t *B;
  saidx_t i, j, p;
  saint_t c;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (idx < 0) ||
     (n < idx) || ((0 < n) && (idx == 0))) {
    return -1;
  }
  if((ISA == NULL) || (C == NULL) || (D == NULL) || (d < 0) || (d > CSA_SIGMA))
  {
      return -3;
  }
  if(n <= 1) { return 0; }

  if((B = A) == NULL) {
    /* Allocate n*sizeof(saidx_t) bytes of memory. */
    if((B = (saidx_t *)malloc((size_t)n * sizeof(saidx_t))) == NULL) { return -2; }
  }

  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
  /* Inverse BW transform. */
  for(c = 0, i = 0; c < CSA_SIGMA; ++c) {
    p = C[c];
    if(0 < p) {
      C[c] = i;
      i += p;
    }
  }
  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
  TOCK;
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
  TICK;
  /*
   * F[i] = T[SA[i]] => T[i] = F[ISA[i]]
   * LF[i] = ISA[SA[i] - 1] => FL[i] = ISA[SA[i] + 1]
   * T[i + 1] = F[FL[ISA[i]]]
   */
#ifdef _OPENMP
#pragma omp parallel for default(shared) private(i, j, p)
#endif
  for (j = 0; j <= (n - 1) / CSA_D2; ++j)
  {
      int k = (j + 1) * CSA_D2;
      if (n < k)
      {
          k = n;
      }
      for (i = j * CSA_D2, p = ISA[j]; i < k; ++i)
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

int main(int argc, char *argv[])
{
    FILE *fp;
    char *base, *ifname;
    int baselen;
    off_t len;
    sauchar_t *T;
    saidx_t *SA;
    saidx_t *ISA;
    saidx_t last;
    saidx_t C[CSA_SIGMA];
    sauchar_t D[CSA_SIGMA];
    int m;
    int ret;
    if (argc<2|| argc>3|| !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        printf("Usage: %s input_filename[.bw] [output_filename]\n",
               argv[0]);
        printf("Note: file input_filename.lst is needed too\n");
        return argc > 2;
    }
    else
    {
        char *dot;
        base = argv[1];
        if ((dot = strrchr(base, '.')) &&
            !strchr(dot, '/') && !strchr(dot, '\\'))
        {
            baselen = (int) (dot - base);
        }
        else
        {
            baselen = (int) strlen(base);
        }
    }
    ifname = (char *) malloc(baselen + 5);
    if (!ifname)
    {
        fprintf(stderr, "malloc failed\n");
        return FAIL_RET;
    }
#undef CLEAN_UP
#define CLEAN_UP free(ifname)
    sprintf(ifname, "%.*s.lst", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "r");
    last = 0;
    if (fscanf(fp, "%lu", (unsigned long *)&last) != 1)
    {
        perror("scanf failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    sprintf(ifname, "%.*s.bw", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    if (fseeko(fp, 0, SEEK_END))
    {
        perror("seek failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    len = ftello(fp);
    if (len <= 0 || len > MAX_FILE_LEN)
    {
        if (len < 0)
        {
            perror("tell failed");
        }
        else if (len == 0)
        {
            fprintf(stderr, "empty file\n");
        }
        else
        {
            fprintf(stderr, "file too large: %" PRId64 " > %lu bytes\n",
                    (int64_t) len, MAX_FILE_LEN);
        }
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    rewind(fp);
    T = (sauchar_t *) malloc(len * sizeof(sauchar_t));
    SA = (saidx_t *) malloc(len * sizeof(saidx_t));
    if (!T || !SA)
    {
        perror("malloc failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        free(SA);\
        free(T);\
        free(ifname);\
    } while (0)
    fprintf(stderr, "Reading %s (%lu bytes) ... ", ifname, (long) len);
    TICK;
    if (fread(T, sizeof(sauchar_t), len, fp) != len)
    {
        perror("read failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    TOCK;
    sprintf(ifname, "%.*s.idx", baselen, base);
    fp = fopen(ifname, "rb");
    if (fp)
    {
        int err = 0;
        fprintf(stderr, "Loading index ... ");
        TICK;
        ISA = (saidx_t *) malloc(((len - 1) / CSA_D2 + 1) * sizeof(saidx_t));
        if (!ISA)
        {
            perror("malloc failed");
            fclose(fp);
            CLEAN_UP;
            return FAIL_RET;
        }
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        if (ISA) free (ISA);\
        free(SA);\
        free(T);\
        free(ifname);\
    } while (0)
#define CHECK_COND(_cond) \
        {\
            if (!(_cond) || ferror(fp))\
            {\
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
            CHECK_COND(readint(CSA_K, fp) == len);
            CHECK_COND(readint(CSA_K, fp) == CSA_D);
            CHECK_COND(readint(CSA_K, fp) == CSA_D2);
            CHECK_COND(readint(CSA_K, fp) == CSA_SIGMA);
            memset(C, 0, sizeof(C));
            memset(D, 0, sizeof(D));
            CHECK_COND((m = (int) readint(CSA_K, fp)) >= 0);
            for (i = 0; i < m; i++)
            {
                CHECK_COND((D[i] = (sauchar_t) readint(1, fp)) >= 0);
                CHECK_COND((C[D[i]] = (saidx_t) readint(CSA_K, fp)) > 0);
            }
            CHECK_COND(readint(1, fp) == CSA_ID_SA);
            CHECK_COND(readint(1, fp) == CSA_K);
            CHECK_COND(readint(CSA_K, fp) == CSA_D);
            fseeko(fp, (len / CSA_D + 1) * CSA_K, SEEK_CUR);
            CHECK_COND(readint(1, fp) == CSA_ID_ISA);
            CHECK_COND(readint(1, fp) == CSA_K);
            CHECK_COND(readint(CSA_K, fp) == CSA_D2);
            for (i = 0; i <= (len - 1) / CSA_D2; i++)
            {
                CHECK_COND((ISA[i] = (saidx_t) readint(CSA_K, fp)) > 0);
            }
        } while (0);
        fclose(fp);
        if (err)
        {
            fprintf(stderr, "failed %d\n", err);
            CLEAN_UP;
            return FAIL_RET;
        }
        TOCK;
    }
    else
    {
        ISA = NULL;
    }
    fprintf(stderr, "Computing Inverse BWT ... ");
    TICK;
    if ((ISA ? my_inverse_bw_transform(T, T, SA, len, last, ISA, C, D, m)
         : inverse_bw_transform(T, T, SA, len, last)) != 0)
    {
        fprintf(stderr, "IBWT failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    if (argc > 2)
    {
        CHECK_OPEN_FILE(fp, argv[2], "wb");
    }
    else
    {
        fp = stdout;
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
    }
    ret = 0;
    fprintf(stderr, "Writing %s ... ", argc > 2 ? argv[2] : "(stdout)");
    TICK;
    if (fwrite(T, sizeof(sauchar_t), len, fp) != len)
    {
        perror("write failed");
        ret = FAIL_RET;
    }
    if (fp != stdout)
    {
        fclose(fp);
    }
    TOCK;
    CLEAN_UP;
    return ret;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
