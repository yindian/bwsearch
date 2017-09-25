/*
 * See Copyright Notice in bwslib.h
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bwscommon.h"
#include "csacompat.h"
#include "sawrapper.h"

static void writeint(int k, int64_t x, FILE *fp)
{
    while (k-- > 0)
    {
        fputc(x & 0xff, fp);
        x >>= 8;
    }
    assert(x == 0 || x == -1);
}

static int count1_log2(int64_t x, int *plog2)
{
    int l, n;
    for (l = 64, n = 0; x && l; --l)
    {
        n += x & 1;
        x >>= 1;
    }
    if (plog2)
    {
        *plog2 = 64 - l - 1;
    }
    return n;
}

static int count1(int64_t x)
{
    return count1_log2(x, NULL);
}

static int log2(int64_t x)
{
    int ret;
    count1_log2(x, &ret);
    return ret;
}

int main(int argc, char *argv[])
{
    FILE *fp;
    char *base, *ofname;
    int baselen;
    off_t len;
    sauchar_t *T;
    saidx_t *SA;
    saidx_t *ISA;
    saidx_t last;
    int i, j;
    int m; /* the number of distinct characters in the text  */
    sauchar_t AtoC[CSA_SIGMA];
    if (argc<2|| argc>3|| !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        printf("Usage: %s input_filename [output_file_basename]\n",
               argv[0]);
        printf("Output: basename.idx basename.bw basename.lst\n");
        return argc > 2;
    }
    if (argc > 2)
    {
        base = argv[2];
        baselen = (int) strlen(base);
    }
    else
    {
        GET_BASE_N_LEN(base, baselen, argv[1]);
    }
    CHECK_OPEN_FILE(fp, argv[1], "rb");
#undef CLEAN_UP
#define CLEAN_UP fclose(fp)
    if (fseeko(fp, 0, SEEK_END))
    {
        perror("seek failed");
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
        CLEAN_UP;
        return FAIL_RET;
    }
    rewind(fp);
    T = (sauchar_t *) malloc(len * sizeof(sauchar_t));
    SA = (saidx_t *) malloc(len * sizeof(saidx_t));
    ISA = (saidx_t *) malloc(((len - 1) / CSA_D2 + 1) * sizeof(saidx_t));
    if (!T || !SA || !ISA)
    {
        fprintf(stderr, "malloc failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    assert(count1(-1) == 64);
    assert(count1(CSA_D) == 1);
    assert(count1(CSA_D2) == 1);
    assert(count1(CSA_L) == 1);
    assert(count1(CSA_LB) == 1);
    assert(CSA_L <= CSA_LB);
    assert(CSA_LB <= (1 << 16));
    assert(CSA_LB == (1 << log2(CSA_LB)));
    fprintf(stderr, "Reading %s (%lu bytes) ... ", argv[1], (long) len);
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        fclose(fp);\
        free(ISA);\
        free(SA);\
        free(T);\
    } while (0)
    TICK;
    if (fread(T, sizeof(sauchar_t), len, fp) != len)
    {
        perror("read failed");
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    TOCK;
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        free(ISA);\
        free(SA);\
        free(T);\
    } while (0)
    fprintf(stderr, "Sorting suffix array ... ");
    TICK;
    if (divsufsort(T, SA, len) != 0)
    {
        fprintf(stderr, "sort failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    ofname = (char *) malloc(baselen + 5);
    if (!ofname)
    {
        fprintf(stderr, "malloc failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        free(ofname);\
        free(ISA);\
        free(SA);\
        free(T);\
    } while (0)
#undef FAIL_RET
#define FAIL_RET 2
    sprintf(ofname, "%.*s.idx", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "wb");
    fprintf(stderr, "Writing %s ... ", ofname);
    TICK;
    writeint(4, CSA_VERSION, fp);
    writeint(1, CSA_ID_HEADER, fp);
    writeint(1, CSA_K, fp);
    writeint(CSA_K, len, fp);
    writeint(CSA_K, CSA_D, fp);
    writeint(CSA_K, CSA_D2, fp);
    writeint(CSA_K, CSA_SIGMA, fp);
    {
        off_t C[CSA_SIGMA];
        memset(C, 0, sizeof(C));
        memset(AtoC, 0, sizeof(AtoC));
        for (i = 0; i < len; i++)
        {
            ++C[T[i]];
        }
        m = 0;
        for (i = 0; i < CSA_SIGMA; i++)
        {
            m += !!C[i];
        }
        writeint(CSA_K, m, fp);
        j = 0;
        for (i = 0; i < CSA_SIGMA; i++)
        {
            if (C[i])
            {
                writeint(1, i, fp); /* characters appeared in the text */
                writeint(CSA_K, C[i], fp); /* frequency of characters */
                AtoC[j++] = i;
            }
        }
    }
    writeint(1, CSA_ID_SA, fp);
    writeint(1, CSA_K, fp);
    writeint(CSA_K, CSA_D, fp);
    writeint(CSA_K, len, fp);
    for (i = 0, j = CSA_D - 1; i < len / CSA_D; i++, j += CSA_D)
    {
        writeint(CSA_K, SA[j], fp);
    }
    writeint(1, CSA_ID_ISA, fp);
    writeint(1, CSA_K, fp);
    writeint(CSA_K, CSA_D2, fp);
    for (i = 0; i < len; i++)
    {
        if (SA[i] % CSA_D2 == 0)
        {
            ISA[SA[i] / CSA_D2] = i + 1;
        }
    }
    for (i = 0, j = 0; i <= (len - 1) / CSA_D2; i++, j += CSA_D2)
    {
        writeint(CSA_K, ISA[i], fp);
    }
    fclose(fp);
    TOCK;
    fprintf(stderr, "Constructing BWT ... ");
    TICK;
    if (bw_transform(T, T, SA, len, &last) != 0)
    {
        fprintf(stderr, "BWT failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    sprintf(ofname, "%.*s.bw", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "wb");
    fprintf(stderr, "Writing %s ... ", ofname);
    TICK;
    if (fwrite(T, sizeof(sauchar_t), len, fp) != len)
    {
        perror("write failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    TOCK;
    sprintf(ofname, "%.*s.lst", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "w");
    fprintf(stderr, "Writing %s ... ", ofname);
    TICK;
    if (fprintf(fp, "%lu", (long) last) < 0)
    {
        perror("print failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    TOCK;
    sprintf(ofname, "%.*s.bws", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "wb");
    fprintf(stderr, "Writing %s ... ", ofname);
    TICK;
    writeint(1, CSA_ID_LF, fp);
    writeint(1, CSA_K, fp);
    writeint(CSA_K, len, fp);
    writeint(CSA_K, last, fp);
    writeint(CSA_K, CSA_L, fp);
    writeint(1, CSA_ID_BWS_IDX, fp);
    writeint(1, log2(CSA_LB), fp);
    writeint(CSA_K, m, fp);
    {
        off_t C[CSA_SIGMA];
        off_t D[CSA_SIGMA];
        off_t *buf;
        sauchar_t *bws;
        int l, c;
        buf = (off_t *) malloc(sizeof(off_t) * m * (len / CSA_LB + 1));
        if (!buf)
        {
            fprintf(stderr, "malloc failed\n");
            CLEAN_UP;
            return FAIL_RET;
        }
        bws = (sauchar_t *) malloc(sizeof(sauchar_t) * (len / CSA_L + 1));
        if (!bws)
        {
            fprintf(stderr, "malloc failed\n");
            free(buf);
            CLEAN_UP;
            return FAIL_RET;
        }
        memset(C, 0, sizeof(C));
        l = c = 0;
        for (i = 0; i <= len; i += CSA_LB)
        {
            int k, n;
            memset(D, 0, sizeof(D));
            for (k = 0; k < CSA_LB; k += CSA_L)
            {
                if (i + k > len)
                {
                    break;
                }
                n = i + k + CSA_L;
                if (len + 1 < n)
                {
                    n = len + 1;
                }
                if (n <= last)
                {
                    j = i + k;
                }
                else if (i + k > last)
                {
                    --n;
                    j = i + k - 1;
                }
                else
                {
                    --n;
                    j = i + k;
                }
                bws[c++] = T[j];
                writeint(2, D[T[j]], fp);
                for (; j < n; j++)
                {
                    ++D[T[j]];
                }
            }
            for (j = 0; j < m; j++)
            {
                C[j] += D[AtoC[j]];
                buf[l++] = C[j];
            }
        }
        assert(l == m * (len / CSA_LB + 1));
        assert(c == (len / CSA_L + 1));
        for (j = 0; j < c; j++)
        {
            writeint(1, bws[j], fp);
        }
        for (j = 0; j < l; j++)
        {
            writeint(CSA_K, buf[j], fp);
        }
        free(buf);
    }
    fclose(fp);
    TOCK;
    free(ofname);
    free(ISA);
    free(SA);
    free(T);
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
