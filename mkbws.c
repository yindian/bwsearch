#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "sawrapper.h"
#define MAX_FILE_LEN (1UL << 31)
int main(int argc, char *argv[])
{
    FILE *fp;
    char *base, *ofname;
    int baselen;
    off_t len;
    sauchar_t *T;
    saidx_t *SA;
    clock_t start, finish;
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
        baselen = strlen(base);
    }
    else
    {
        char *dot;
        base = argv[1];
        if ((dot = strrchr(base, '.')) &&
            !strchr(dot, '/') && !strchr(dot, '\\'))
        {
            baselen = dot - base;
        }
        else
        {
            baselen = strlen(base);
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
    if (!T || !SA)
    {
        fprintf(stderr, "malloc failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    fprintf(stderr, "Reading %s (%ld bytes) ...\n", argv[1], (long) len);
    if (fread(T, sizeof(sauchar_t), len, fp) != len)
    {
        perror("read failed");
        CLEAN_UP;
        return FAIL_RET;
    }
    fclose(fp);
    fprintf(stderr, "Sorting suffix array ... ");
    start = clock();
    if (divsufsort(T, SA, len) != 0)
    {
        fprintf(stderr, "sort failed\n");
        CLEAN_UP;
        return FAIL_RET;
    }
    finish = clock();
    fprintf(stderr, "%.4f sec\n", (double)(finish - start) / CLOCKS_PER_SEC);
    ofname = (char *) malloc(baselen + 5);
#undef CLEAN_UP
#define CLEAN_UP free(ofname)
#undef FAIL_RET
#define FAIL_RET 2
    sprintf(ofname, "%.*s.idx", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "wb");
    fclose(fp);
    sprintf(ofname, "%.*s.bw", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "wb");
    fclose(fp);
    sprintf(ofname, "%.*s.lst", baselen, base);
    CHECK_OPEN_FILE(fp, ofname, "w");
    fclose(fp);
    free(ofname);
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
