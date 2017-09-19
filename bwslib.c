/*
 * See Copyright Notice in bwslib.h
 */

#include "bwslib.h"
#ifndef COMPILE_TEST
#include <inttypes.h>
#else
#include "bwscompat.h"
#endif
#include <stdlib.h>
#include <string.h>

#if 0
#define DEBUG_BWS
#endif

static int64_t readint(int k, FILE *f)
{
    int64_t x;
    int i;
    x = 0;
    for (i = 0; i < k; i++)
    {
        x += (int64_t) fgetc(f) << (8*i);
    }
    return x;
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

int bws_load_csa_index(csaidx_t *pindex, int flags, FILE *fp)
{
    int ret, err;
    if (!pindex || !fp)
    {
        return BWS_RET_INV_ARG;
    }
    memset(pindex, 0, sizeof(csaidx_t));
    ret = BWS_RET_OK;
    err = 0;
#define CHECK_COND(_cond) \
    {\
        if (!(_cond) || ferror(fp))\
        {\
            ret = BWS_RET_MALFORM;\
            err = __LINE__;\
            break;\
        }\
    }
    do
    {
        int i;
        CHECK_COND(readint(4, fp) == CSA_VERSION);
        CHECK_COND(readint(1, fp) == CSA_ID_HEADER);
        CHECK_COND((pindex->k = readint(1, fp)) <= CSA_K);
        CHECK_COND((pindex->n = readint(pindex->k, fp)) >= 0);
        CHECK_COND((pindex->d = readint(pindex->k, fp)) >= CSA_D);
        CHECK_COND((pindex->d2 = readint(pindex->k, fp)) >= CSA_D2);
        CHECK_COND((pindex->s = readint(pindex->k, fp)) <= CSA_SIGMA);
        memset(pindex->C, 0, sizeof(pindex->C));
        memset(pindex->K, 0, sizeof(pindex->C));
        memset(pindex->AtoC, 0, sizeof(pindex->AtoC));
        for (i = 0; i < CSA_SIGMA; i++)
        {
            pindex->CtoA[i] = -1;
        }
        pindex->K[0] = pindex->K[1] = 1;
        CHECK_COND((pindex->m = (int) readint(pindex->k, fp)) >= 0);
        for (i = 0; i < pindex->m; i++)
        {
            CHECK_COND((pindex->AtoC[i] = (sauchar_t) readint(1, fp)) >= 0);
            CHECK_COND((pindex->C[pindex->AtoC[i]]
                        = (saidx_t) readint(pindex->k, fp)) > 0);
            pindex->CtoA[pindex->AtoC[i]] = i;
            pindex->K[i + 2] = pindex->K[i + 1] + pindex->C[pindex->AtoC[i]];
        }
        CHECK_COND(readint(1, fp) == CSA_ID_SA);
        CHECK_COND(readint(1, fp) == pindex->k);
        CHECK_COND(readint(pindex->k, fp) == pindex->d);
        if ((flags & BWS_FLAG_LOAD_SA))
        {
            pindex->SA = (saidx_t *) malloc((pindex->n / pindex->d + 1)
                                            * sizeof(saidx_t));
            if (!pindex->SA)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i <= pindex->n / pindex->d; i++)
            {
                CHECK_COND((pindex->SA[i]
                            = (saidx_t) readint(pindex->k, fp)) > 0);
                if (err)
                {
                    break;
                }
            }
        }
        else
        {
            fseeko(fp, (pindex->n / pindex->d + 1) * pindex->k, SEEK_CUR);
        }
        CHECK_COND(readint(1, fp) == CSA_ID_ISA);
        CHECK_COND(readint(1, fp) == pindex->k);
        CHECK_COND(readint(pindex->k, fp) == pindex->d2);
        if ((flags & BWS_FLAG_LOAD_ISA))
        {
            pindex->ISA = (saidx_t *) malloc(((pindex->n - 1) / pindex->d2 + 1)
                                             * sizeof(saidx_t));
            if (!pindex->ISA)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i <= (pindex->n - 1) / pindex->d2; i++)
            {
                CHECK_COND((pindex->ISA[i]
                            = (saidx_t) readint(pindex->k, fp)) > 0);
                if (err)
                {
                    break;
                }
            }
        }
    } while (0);
    if (err)
    {
        fprintf(stderr, "Error in %s:%d\n", __FILE__, err);
    }
    return ret;
}

int bws_free_csa_index(csaidx_t *pindex)
{
    if (!pindex)
    {
        return BWS_RET_INV_ARG;
    }
    if (pindex->SA)
    {
        free(pindex->SA);
        pindex->SA = NULL;
    }
    if (pindex->ISA)
    {
        free(pindex->ISA);
        pindex->ISA = NULL;
    }
    return BWS_RET_OK;
}

int bws_load_bws_index(bwsidx_t *pindex, int flags, FILE *fp)
{
    int ret, err;
    if (!pindex || !fp)
    {
        return BWS_RET_INV_ARG;
    }
    memset(pindex, 0, sizeof(bwsidx_t));
    ret = BWS_RET_OK;
    err = 0;
#define CHECK_COND(_cond) \
    {\
        if (!(_cond) || ferror(fp))\
        {\
            ret = BWS_RET_MALFORM;\
            err = __LINE__;\
            break;\
        }\
    }
    do
    {
        int i;
        int l, c;
        CHECK_COND(readint(1, fp) == CSA_ID_LF);
        CHECK_COND((pindex->k = readint(1, fp)) <= CSA_K);
        CHECK_COND((pindex->n = readint(pindex->k, fp)) >= 0);
        CHECK_COND((pindex->last = readint(pindex->k, fp)) >= 0);
        CHECK_COND(pindex->last <= pindex->n);
        CHECK_COND((pindex->l = readint(pindex->k, fp)) >= CSA_L);
        CHECK_COND(count1(pindex->l) == 1);
        CHECK_COND(readint(1, fp) == CSA_ID_BWS_IDX);
        CHECK_COND((pindex->logLB = readint(1, fp)) >= 0);
        CHECK_COND(pindex->logLB <= 16);
        pindex->lb = 1 << pindex->logLB;
        CHECK_COND((pindex->m = (int) readint(pindex->k, fp)) >= 0);
        l = pindex->m * (pindex->n / CSA_LB + 1);
        c = pindex->m * (pindex->n / CSA_L + 1);
        if ((flags & BWS_FLAG_LOAD_RANKC))
        {
            pindex->lRankC = (unsigned short *) malloc(c * sizeof(saidx_t));
            if (!pindex->lRankC)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            pindex->lbRankC = (saidx_t *) malloc(l * sizeof(saidx_t));
            if (!pindex->lbRankC)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i < c; i++)
            {
                CHECK_COND((pindex->lRankC[i]
                            = (unsigned short) readint(2, fp)) >= 0);
                if (err)
                {
                    break;
                }
            }
            for (i = 0; i < l; i++)
            {
                CHECK_COND((pindex->lbRankC[i]
                            = (saidx_t) readint(pindex->k, fp)) >= 0);
                if (err)
                {
                    break;
                }
            }
        }
        else
        {
            fseeko(fp, c * 2, SEEK_CUR);
            fseeko(fp, l * pindex->k, SEEK_CUR);
        }
    } while (0);
    if (err)
    {
        fprintf(stderr, "Error in %s:%d\n", __FILE__, err);
    }
    return ret;
}

int bws_free_bws_index(bwsidx_t *pindex)
{
    if (!pindex)
    {
        return BWS_RET_INV_ARG;
    }
    if (pindex->lRankC)
    {
        free(pindex->lRankC);
        pindex->lRankC = NULL;
    }
    if (pindex->lbRankC)
    {
        free(pindex->lbRankC);
        pindex->lbRankC = NULL;
    }
    return BWS_RET_OK;
}

static saidx_t bws_rankc(csaidx_t *pcsa, bwsidx_t *pbws,
                         FILE *fpbw,
                         saidx_t i, int c)
    /* return num of occurrences of c in bw[0 .. i] */
{
    saidx_t rank = 0;
#ifdef DEBUG_BWS
    saidx_t j = i;
#endif
#if 0
    if (i >= pbws->last)
    {
        rank = c < 0;
        i--;
    }
    if (c >= 0)
    {
        c = pcsa->AtoC[c];
        fseeko(fpbw, 0, SEEK_SET);
        while (i-- >= 0)
        {
            rank += fgetc(fpbw) == c;
        }
    }
#else
    if (c < 0)
    {
        rank = i >= pbws->last;
    }
    else
    {
        saidx_t pos = 0;
        if (i >= pbws->lb)
        {
            rank = pbws->lbRankC[((i >> pbws->logLB) - 1) * pcsa->m + c];
            pos = (i >> pbws->logLB) << pbws->logLB; /* i & ~(pbws->lb - 1); */
        }
        i -= pbws->l;
        for (; pos <= i; pos += pbws->l)
        {
            rank += pbws->lRankC[(pos / pbws->l) * pcsa->m + c];
        }
        i += pbws->l;
        if (pos <= i)
        {
            c = pcsa->AtoC[c];
            fseeko(fpbw, pos, SEEK_SET);
            if (i >= pbws->last)
            {
                i--;
            }
            i -= pos;
            while (i-- >= 0)
            {
                rank += fgetc(fpbw) == c;
            }
        }
    }
#endif
#ifdef DEBUG_BWS
    printf("rank_%c(%d) = %d\n", c, j, rank);
#endif
    return rank;
}

int bws_search(csaidx_t *pcsa, bwsidx_t *pbws,
               FILE *fpbw,
               const char *key, int klen,
               saidx_t *pleft, saidx_t *pright)
{
    int i;
    saidx_t l, r, lastl, lastr;
    if (!pcsa || !pbws || !fpbw || !key)
    {
        return BWS_RET_INV_ARG;
    }
    if (pcsa->n != pbws->n)
    {
        return BWS_RET_MALFORM;
    }
    l = 0;
    r = pbws->n;
#ifdef DEBUG_BWS
    for (i = 0; i < pcsa->m + 2; i++)
    {
        printf("%s%d", i == 0 ? "K[] = {" : ", ", pcsa->K[i]);
    }
    printf("}\n");
#endif
    for (i = klen - 1; i >= 0; i--)
    {
        int c = pcsa->CtoA[(unsigned char) key[i]];
        if (c < 0)
        {
            break;
        }
        if (i == klen - 1)
        {
            l = pcsa->K[c + 1];
            r = pcsa->K[c + 2] - 1;
        }
        else
        {
            lastl = l;
            lastr = r;
            l = pcsa->K[c + 1] + bws_rankc(pcsa, pbws,
                                           fpbw,
                                           l - 1, c);
            r = pcsa->K[c + 1] + bws_rankc(pcsa, pbws,
                                           fpbw,
                                           r, c) - 1;
            if (l > r)
            {
                l = lastl;
                r = lastr;
                break;
            }
        }
#ifdef DEBUG_BWS
        printf("Partial: %s l = %d, r = %d\n", key + i, l, r);
#endif
    }
    if (pleft) { *pleft = l; }
    if (pright) { *pright = r; }
    return klen - i - 1;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
