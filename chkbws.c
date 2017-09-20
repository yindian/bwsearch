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
    saidx_t i;
    int c;
    int ret;
    if (argc<2 || argc>3 || (argc == 3 && strcmp(argv[2], "-p"))
        || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        printf("Usage: %s input_filename[.idx] [-p]\n",
               argv[0]);
        printf("Note: file input_filename.idx/bws/bw are needed\n");
        return 1;
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
#define CLEAN_UP do\
    {\
        bws_free_csa_index(&csa);\
        free(ifname);\
    } while (0)
    sprintf(ifname, "%.*s.idx", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    fprintf(stderr, "Loading %s ... ", ifname);
    TICK;
    ret = bws_load_csa_index(&csa, BWS_FLAG_LOAD_ISA, fp);
    fclose(fp);
    if (ret)
    {
        fprintf(stderr, "failed %d\n", ret);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        bws_free_bws_index(&bws);\
        bws_free_csa_index(&csa);\
        free(ifname);\
    } while (0)
    sprintf(ifname, "%.*s.bws", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    fprintf(stderr, "Loading %s ... ", ifname);
    TICK;
    ret = bws_load_bws_index(&bws, BWS_FLAG_LOAD_RANKC, fp);
    fclose(fp);
    if (ret)
    {
        fprintf(stderr, "failed %d\n", ret);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    sprintf(ifname, "%.*s.bw", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    TICK;
    if (argc > 2 && !strcmp(argv[2], "-p"))
    {
        printf("i\\rank");
        for (c = 0; c < csa.m; c++)
        {
            printf("\t%c", csa.AtoC[c]);
        }
        printf("\n");
        for (i = 0; i <= bws.n; i++)
        {
            printf("%d", i);
            for (c = 0; c < csa.m; c++)
            {
                printf("\t%d", bws_rankc(&csa, &bws,
                                         fp,
                                         i, c));
            }
            printf("\n");
        }
    }
    TOCK;
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
