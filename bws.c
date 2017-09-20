/*
 * See Copyright Notice in bwslib.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bwscommon.h"
#include "bwslib.h"

int main(int argc, char *argv[])
{
    FILE *fp;
    char *base, *ifname;
    int baselen;
    csaidx_t csa;
    bwsidx_t bws;
    saidx_t l, r;
    int ret;
    if (argc<3|| argc>4|| !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        printf("Usage: %s input_filename[.idx] pattern [-c]\n",
               argv[0]);
        printf("Note: file input_filename.idx/bws/bw are needed\n");
        return argc > 3;
    }
    else
    {
        GET_BASE_N_LEN(base, baselen, argv[1]);
    }
#include "bws.i"
    fprintf(stderr, "Searching %s ... ", argv[2]);
    TICK;
    ret = bws_search(&csa, &bws,
                     fp,
                     argv[2], strlen(argv[2]),
                     &l, &r);
    fclose(fp);
    TOCK;
    printf("Matched suffix: %s\nl = %d, r = %d, r - l + 1 = %d\n",
           argv[2] + strlen(argv[2]) - ret, l, r, r - l + 1);
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
