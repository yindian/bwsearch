#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "csacompat.h"
#include "sawrapper.h"
#define MAX_FILE_LEN (1UL << 31)

int main(int argc, char *argv[])
{
    FILE *fp;
    char *base, *ifname;
    int baselen;
    off_t len;
    sauchar_t *T;
    saidx_t *SA;
    saidx_t last;
    clock_t start, finish;
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
#define FAIL_RET 1
        return FAIL_RET;
    }
#define CLEAN_UP free(ifname)
#define CHECK_OPEN_FILE(_fp, _name, _mode) \
    if (!(_fp = fopen(_name, _mode)))\
    {\
        fprintf(stderr, "Failed to open %s\n", _name);\
        CLEAN_UP;\
        return FAIL_RET;\
    }
#define TICK do\
    {\
    start = clock();\
    } while (0)
#define TOCK do\
    {\
    finish = clock(); \
    fprintf(stderr, "%.4f sec\n", (double)(finish - start) / CLOCKS_PER_SEC);\
    } while (0)
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
    fprintf(stderr, "Computing Inverse BWT ... ");
    TICK;
    if (inverse_bw_transform(T, T, SA, len, last) != 0)
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
