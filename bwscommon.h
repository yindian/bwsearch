#ifndef _BWSCOMMON_H
#define _BWSCOMMON_H

#include <stdio.h>
#include <time.h>

static clock_t start, finish;
#define TICK do\
    {\
    start = clock();\
    } while (0)
#define TOCK do\
    {\
    finish = clock(); \
    fprintf(stderr, "%.4f sec\n", (double)(finish - start) / CLOCKS_PER_SEC);\
    } while (0)

#define CLEAN_UP
#define FAIL_RET 1
#define CHECK_OPEN_FILE(_fp, _name, _mode) \
    if (!(_fp = fopen(_name, _mode)))\
    {\
        fprintf(stderr, "Failed to open %s\n", _name);\
        CLEAN_UP;\
        return FAIL_RET;\
    }

#define MAX_FILE_LEN (1UL << 31)

#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
