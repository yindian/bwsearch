/*
 * See Copyright Notice in bwslib.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "bwscommon.h"
#include "bwslib.h"

int main(int argc, char *argv[])
{
    FILE *fp;
    bw_file_t *bwfp;
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
        GET_BASE_N_LEN(base, baselen, argv[1]);
    }
#include "bws.i"
    if (argc > 2 && !strcmp(argv[2], "-p"))
    {
        TICK;
        printf("SA\tISA\tLF\t");
        printf("T\tBW\t");
        printf("i\\rank");
        for (c = 0; c < csa.m; c++)
        {
            printf("\t%c", csa.AtoC[c]);
        }
        printf("\n");
#define PRINT_C_TAB(_c) do\
        {\
            if (_c == -1)\
            {\
                printf("$\t");\
            }\
            else if (isalnum(_c) || (ispunct(_c) && _c != '$'))\
            {\
                printf("%c\t", _c);\
            }\
            else\
            {\
                printf("\\x%02X\t", _c);\
            }\
        } while (0)
        for (i = 0; i <= bws.n; i++)
        {
            printf("%d\t%d\t%d\t",
                   bws_sa(&csa, &bws,
                          bwfp,
                          i),
                   bws_isa(&csa, &bws,
                           bwfp,
                           i),
                   bws_lf(&csa, &bws,
                          bwfp,
                          i));
            c = bws_t(&csa, &bws,
                      bwfp,
                      i);
            PRINT_C_TAB(c);
            c = bws_bw(&csa, &bws,
                       bwfp,
                       i);
            PRINT_C_TAB(c);
            printf("%d", i);
            for (c = 0; c < csa.m; c++)
            {
                printf("\t%d", bws_rankc(&csa, &bws,
                                         bwfp,
                                         i, c));
            }
            printf("\n");
        }
        bwfp->close(bwfp);
        fclose(fp);
        TOCK;
        return 0;
    }
    BW = (sauchar_t *) malloc(bws.n);
    if (!BW)
    {
        fprintf(stderr, "malloc failed\n");
        CLEAN_UP;
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
        bwfp->close(bwfp);
        fclose(fp);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
#if 0
    fprintf(stderr, "Checking rank_[1 .. %d](BW, 0 .. %d) .", csa.m, bws.n);
    TICK;
    memset(C, 0, sizeof(C));
    for (i = 0; i <= bws.n; i++)
    {
#if 1
        if (bws_isa(&csa, &bws,
                    bwfp,
                    i) !=
            bws_isa_r(&csa, &bws,
                      bwfp,
                      i))
        {
            printf("Mismatched ISA[%d] %d != %d\n", i,
                   bws_isa(&csa, &bws,
                           bwfp,
                           i),
                   bws_isa_r(&csa, &bws,
                             bwfp,
                             i));
        }
#endif
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
                                     bwfp,
                                     i, c);
            if (C[c] != rank)
            {
                printf("Mismatched rank_%c{%d}(%d) %d != %d\n",
                       csa.AtoC[c], c + 1, i,
                       rank, C[c]);
            }
        }
        if (!(i & (bws.lb - 1)))
        {
            fprintf(stderr, ".");
        }
    }
    fprintf(stderr, " ");
    TOCK;
#else
    (void) C;
#endif
    fprintf(stderr, "Computing T[0 .. %d] ... ", bws.n);
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    TICK;
    fwrite(bws_substr(&csa, &bws,
                      bwfp,
                      0, bws.n),
           1, bws.n, stdout);
    bwfp->close(bwfp);
    fclose(fp);
    TOCK;
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
