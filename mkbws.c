#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "csacompat.h"
#include "sawrapper.h"
#define MAX_FILE_LEN (1UL << 31)

static void writeint(int k, int64_t x, FILE *fp)
{
    while (k-- > 0)
    {
        fputc(x & 0xff, fp);
        x >>= 8;
    }
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
    clock_t start, finish;
    int i, j;
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
#define CLEAN_UP
#define FAIL_RET 1
#define CHECK_OPEN_FILE(_fp, _name, _mode) \
    if (!(_fp = fopen(_name, _mode)))\
    {\
        fprintf(stderr, "Failed to open %s\n", _name);\
        CLEAN_UP;\
        return FAIL_RET;\
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
    fprintf(stderr, "Reading %s (%lu bytes) ... ", argv[1], (long) len);
#define TICK do\
    {\
    start = clock();\
    } while (0)
#define TOCK do\
    {\
    finish = clock(); \
    fprintf(stderr, "%.4f sec\n", (double)(finish - start) / CLOCKS_PER_SEC);\
    } while (0)
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
        int m; /* the number of distinct characters in the text  */
        memset(C, 0, sizeof(C));
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
        for (i = 0; i < CSA_SIGMA; i++)
        {
            if (C[i])
            {
                writeint(1, i, fp); /* characters appeared in the text */
                writeint(CSA_K, C[i], fp); /* frequency of characters */
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
    free(ofname);
    free(ISA);
    free(SA);
    free(T);
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
