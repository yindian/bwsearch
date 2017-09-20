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
    sauchar_t *BW;
    saidx_t C[CSA_SIGMA];
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
    if (argc > 2 && !strcmp(argv[2], "-p"))
    {
        TICK;
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
        TOCK;
        return 0;
    }
    BW = (sauchar_t *) malloc(bws.n);
    if (!BW)
    {
        fprintf(stderr, "malloc failed\n");
        return FAIL_RET;
    }
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        free(BW);\
        bws_free_bws_index(&bws);\
        bws_free_csa_index(&csa);\
        free(ifname);\
    } while (0)
    fprintf(stderr, "Reading %s (%d bytes) ... ", ifname, bws.n);
    TICK;
    if (fread(BW, sizeof(sauchar_t), bws.n, fp) != bws.n)
    {
        perror("read failed");
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    fprintf(stderr, "Checking rank_[1 .. %d](BW, 0 .. %d) ... ", csa.m, bws.n);
    TICK;
    memset(C, 0, sizeof(C));
    for (i = 0; i <= bws.n; i++)
    {
        if (i < bws.last)
        {
            C[csa.CtoA[BW[i]]]++;
        }
        else if (i > bws.last)
        {
            C[csa.CtoA[BW[i - 1]]]++;
        }
        for (c = 0; c < csa.m; c++)
        {
            saidx_t rank = bws_rankc(&csa, &bws,
                                     fp,
                                     i, c);
            if (C[c] != rank)
            {
                printf("Mismatched rank_%c{%d}(%d) %d != %d\n",
                       csa.AtoC[c], c, i,
                       rank, C[c]);
            }
        }
    }
    fclose(fp);
    TOCK;
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
