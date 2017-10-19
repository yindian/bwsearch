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
    bw_file_t *bwfp;
    char *base, *ifname;
    int baselen;
    csaidx_t csa;
    bwsidx_t bws;
    saidx_t l, r;
    int keylen;
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
    keylen = (int) strlen(argv[2]);
    ret = bws_search(&csa, &bws,
                     bwfp,
                     argv[2], keylen,
                     &l, &r);
#if 0
    bwfp->close(bwfp);
    fclose(fp);
    TOCK;
    printf("Matched suffix: %s\nl = %d, r = %d, r - l + 1 = %d\n",
           argv[2] + strlen(argv[2]) - ret, l, r, r - l + 1);
#else
    TOCK;
    if (ret == keylen)
    {
        if (argc > 3 && !strcmp(argv[3], "-c"))
        {
            printf("%d\n", r - l + 1);
            return 0;
        }
        else
        {
#define MAX(_x, _y) ((_x) > (_y) ? (_x) : (_y))
#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#define BUFLEN 256
#define MAXRESULT 5000
            char buf[BUFLEN];
            char *p;
            saidx_t i, j, k, s, t;

            if (r - l + 1 > MAXRESULT)
            {
                r = l + MAXRESULT - 1;
            }

            TICK;
            for (i = l; i <= r; i++)
            {
                j = bws_sa(&csa, &bws,
                           bwfp,
                           i);
                p = buf + BUFLEN;
                *--p = '\0';
                for (k = j - 1, s = MAX(0, j - BUFLEN); k > s; --k)
                {
                    *--p = bws_t(&csa, &bws,
                                 bwfp,
                                 k);
                    if (*p == '\n' || *p == '\0')
                    {
                        ++p;
                        break;
                    }
                }
                if (k == s)
                {
                    printf("...");
                }
                printf("%s%s", p, argv[2]);
                p = buf - 1;
                for (k = j + keylen, t = MIN(bws.n + 1, j + keylen + BUFLEN);
                     k < t; ++k)
                {
                    *++p = bws_t(&csa, &bws,
                                 bwfp,
                                 k);
                    if (*p == '\n' || *p == '\0')
                    {
                        --p;
                        break;
                    }
                }
                *++p = '\0';
                printf("%s", buf);
                if (k == t)
                {
                    printf("...");
                }
                printf("\n");
            }
            TOCK;
        }
    }
    else
    {
        fprintf(stderr, "Matched suffix: %s\n",
                argv[2] + keylen - ret);
        return ret;
    }
#endif
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
