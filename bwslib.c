/*
 * See Copyright Notice in bwslib.h
 */

#define _POSIX_SOURCE
#include "bwslib.h"
#ifndef COMPILE_TEST
#include <inttypes.h>
#else
#include "bwscompat.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include "mman.h"
#else
#include <sys/mman.h>
#endif

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

static int64_t getint(int k, sauchar_t *base, int idx)
{
    int64_t x;
    int i;
    x = 0;
    base += idx * k;
    for (i = 0; i < k; i++)
    {
        x += (int64_t) *base++ << (8*i);
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

static saidx_t get_saidx_direct(saidx_t *base, int idx, unsigned int k)
{
    (void)k;
    return base[idx];
}

static unsigned short get_ushort_direct(unsigned short *base, int idx)
{
    return base[idx];
}

#if !defined(le32toh) || !defined(le16toh)
#if BYTE_ORDER == LITTLE_ENDIAN
#define le32toh(_x) (_x)
#define le16toh(_x) (_x)
#else
#define le32toh(_x) ({\
                     uint32_t _v = (_x);\
                     ((_v & 0xFF) << 24) | (((_v >> 8) & 0xFF) << 16) |\
                     (((_v >> 16) & 0xFF) << 8) | (_v >> 24);})
#define le16toh(_x) ({\
                     uint16_t _v = (_x);\
                     ((_v & 0xFF) << 8) | (_v >> 8);})
#endif
#endif

static saidx_t get_saidx_le(saidx_t *base, int idx, unsigned int k)
{
    (void)k;
    return (saidx_t) le32toh((uint32_t) base[idx]);
}

static unsigned short get_ushort_le(unsigned short *base, int idx)
{
    return le16toh(base[idx]);
}

static saidx_t get_saidx_nonstd(saidx_t *base, int idx, unsigned int k)
{
    return getint(k, (sauchar_t *) base, idx);
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
        if ((flags & BWS_FLAG_MMAP))
        {
            off_t pos = ftello(fp);
            if (fseeko(fp, 0, SEEK_END))
            {
                perror("seek failed");
                return BWS_RET_MEM_ERR;
            }
            pindex->len = (long) ftello(fp);
            if (fseeko(fp, pos, SEEK_SET))
            {
                perror("seek failed");
                return BWS_RET_MEM_ERR;
            }
            CHECK_COND((pindex->fd = dup(fileno(fp))) > 0);
            pindex->map = mmap(NULL, pindex->len,
                              PROT_READ, MAP_SHARED, pindex->fd, 0);
            if (!pindex->map || pindex->map == MAP_FAILED)
            {
                perror("mmap failed");
                return BWS_RET_MEM_ERR;
            }
            if (sizeof(saidx_t) == pindex->k)
            {
                pindex->get = get_saidx_le;
            }
            else
            {
                pindex->get = get_saidx_nonstd;
            }
        }
        else
        {
            pindex->get = get_saidx_direct;
        }
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
        if ((flags & BWS_FLAG_LOAD_SA) && (flags & BWS_FLAG_MMAP)
            )
        {
            pindex->SA = (saidx_t *) ((char *) pindex->map + ftello(fp));
            fseeko(fp, (pindex->n / pindex->d + 1) * pindex->k, SEEK_CUR);
        }
        else
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
        pindex->n_sub_1_div_d2 = (pindex->n - 1) / pindex->d2;
        if ((flags & BWS_FLAG_LOAD_ISA))
        {
            pindex->cache.isa_cache = (saidx_t *) malloc(pindex->d2
                                                         * sizeof(saidx_t));
            if (!pindex->cache.isa_cache)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
        }
        if ((flags & BWS_FLAG_LOAD_ISA) && (flags & BWS_FLAG_MMAP)
            )
        {
            pindex->ISA = (saidx_t *) ((char *) pindex->map + ftello(fp));
            fseeko(fp, (pindex->n_sub_1_div_d2 + 1) * pindex->k, SEEK_CUR);
        }
        else
        if ((flags & BWS_FLAG_LOAD_ISA))
        {
            pindex->ISA = (saidx_t *) malloc((pindex->n_sub_1_div_d2 + 1)
                                             * sizeof(saidx_t));
            if (!pindex->ISA)
            {
                perror("malloc failed");
                return BWS_RET_MEM_ERR;
            }
            for (i = 0; i <= pindex->n_sub_1_div_d2; i++)
            {
                CHECK_COND((pindex->ISA[i]
                            = (saidx_t) readint(pindex->k, fp)) > 0);
                if (err)
                {
                    break;
                }
            }
        }
        else
        {
            fseeko(fp, (pindex->n_sub_1_div_d2 + 1) * pindex->k, SEEK_CUR);
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
    if (pindex->cache.isa_cache)
    {
        free(pindex->cache.isa_cache);
        pindex->cache.isa_cache = NULL;
    }
    pindex->get = NULL;
    if (pindex->map)
    {
        if (munmap(pindex->map, pindex->len))
        {
            perror("munmap failed");
        }
        pindex->len = 0;
        pindex->map = NULL;
    }
    if (pindex->fd)
    {
        close(pindex->fd);
        pindex->fd = 0;
        return BWS_RET_OK;
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
        if ((flags & BWS_FLAG_MMAP))
        {
            off_t pos = ftello(fp);
            if (fseeko(fp, 0, SEEK_END))
            {
                perror("seek failed");
                return BWS_RET_MEM_ERR;
            }
            pindex->len = (long) ftello(fp);
            if (fseeko(fp, pos, SEEK_SET))
            {
                perror("seek failed");
                return BWS_RET_MEM_ERR;
            }
            CHECK_COND((pindex->fd = dup(fileno(fp))) > 0);
            pindex->map = mmap(NULL, pindex->len,
                              PROT_READ, MAP_SHARED, pindex->fd, 0);
            if (!pindex->map || pindex->map == MAP_FAILED)
            {
                perror("mmap failed");
                return BWS_RET_MEM_ERR;
            }
            if (sizeof(saidx_t) == pindex->k)
            {
                pindex->get = get_saidx_le;
            }
            else
            {
                pindex->get = get_saidx_nonstd;
            }
            pindex->get16 = get_ushort_le;
        }
        else
        {
            pindex->get = get_saidx_direct;
            pindex->get16 = get_ushort_direct;
        }
        CHECK_COND((pindex->n = readint(pindex->k, fp)) >= 0);
        CHECK_COND((pindex->last = readint(pindex->k, fp)) >= 0);
        CHECK_COND(pindex->last <= pindex->n);
        CHECK_COND((pindex->l = readint(pindex->k, fp)) > 0);
        CHECK_COND(count1(pindex->l) == 1);
        CHECK_COND(readint(1, fp) == CSA_ID_BWS_IDX);
        CHECK_COND((pindex->logLB = readint(1, fp)) >= 0);
        CHECK_COND(pindex->logLB <= 16);
        pindex->lb = 1 << pindex->logLB;
        CHECK_COND((pindex->m = (int) readint(pindex->k, fp)) >= 0);
        l = pindex->m * (pindex->n / pindex->lb + 1);
        c = pindex->m * (pindex->n / pindex->l + 1);
        if ((flags & BWS_FLAG_LOAD_RANKC) && (flags & BWS_FLAG_MMAP)
            )
        {
            pindex->lRankC = (unsigned short *)((char *)pindex->map+ftello(fp));
            fseeko(fp, c * 2, SEEK_CUR);
            pindex->lbRankC = (saidx_t *) ((char *)pindex->map+ftello(fp));
            fseeko(fp, l * pindex->k, SEEK_CUR);
        }
        else
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
    pindex->get = NULL;
    pindex->get16 = NULL;
    if (pindex->map)
    {
        if (munmap(pindex->map, pindex->len))
        {
            perror("munmap failed");
        }
        pindex->len = 0;
        pindex->map = NULL;
    }
    if (pindex->fd)
    {
        close(pindex->fd);
        pindex->fd = 0;
        return BWS_RET_OK;
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

typedef struct _bw_mmap_t {
    sauchar_t *map;
    off_t   base;
    off_t   pos;
    off_t   len;
} bw_mmap_t;

static bw_mmap_t *bw_file_new_mmap(FILE *fp)
{
    bw_mmap_t *bwm = (bw_mmap_t *) malloc(sizeof(bw_mmap_t));
    if (bwm)
    {
        bwm->pos = ftello(fp);
        do
        {
            if (fseeko(fp, 0, SEEK_END))
            {
                perror("seek failed");
                break;
            }
            bwm->len = ftello(fp);
            if (fseeko(fp, bwm->pos, SEEK_SET))
            {
                perror("seek failed");
                break;
            }
            bwm->map = (sauchar_t *) mmap(NULL, bwm->len,
                                          PROT_READ, MAP_SHARED,
                                          fileno(fp), 0);
            if (!bwm->map || bwm->map == MAP_FAILED)
            {
                perror("mmap failed");
                break;
            }
            bwm->base = bwm->pos;
            bwm->pos = 0;
            bwm->len -= bwm->base;
            return bwm;
        } while (0);
        free(bwm);
    }
    return NULL;
}

static void bw_file_seek_set_from_mmap(bw_file_t *bwfp, saidx_t pos)
{
    bw_mmap_t *bwm = (bw_mmap_t *) bwfp->tag;
    if (pos >= 0 && pos <= bwm->len)
    {
        bwm->pos = pos;
    }
    else
    {
        fprintf(stderr, "seek beyond range\n");
    }
}

static int bw_file_get_char_from_mmap(bw_file_t *bwfp)
{
    bw_mmap_t *bwm = (bw_mmap_t *) bwfp->tag;
    if (bwm->pos >= 0 && bwm->pos < bwm->len)
    {
        return bwm->map[bwm->base + bwm->pos++];
    }
    return -1;
}

static void bw_file_close_from_mmap(bw_file_t *bwfp)
{
    bw_mmap_t *bwm = (bw_mmap_t *) bwfp->tag;
    if (munmap(bwm->map, bwm->base + bwm->len))
    {
        perror("munmap failed");
    }
    free(bwm);
    free(bwfp);
}

static void bw_file_seek_set_from_fp(bw_file_t *bwfp, saidx_t pos)
{
    FILE *fp = (FILE *) bwfp->tag;
    if (fseeko(fp, pos, SEEK_SET))
    {
        perror("seek failed");
    }
}

static int bw_file_get_char_from_fp(bw_file_t *bwfp)
{
    FILE *fp = (FILE *) bwfp->tag;
    return fgetc(fp);
}

static void bw_file_close_from_fp(bw_file_t *bwfp)
{
    free(bwfp);
}

bw_file_t *bw_file_new_from_fp(FILE *fp, int flags)
{
    bw_file_t *bwfp;
    if (!fp)
    {
        return NULL;
    }
    bwfp = (bw_file_t *) malloc(sizeof(bw_file_t));
    if (!bwfp)
    {
        return NULL;
    }
    if ((flags & BWS_FLAG_MMAP))
    {
        bwfp->tag = bw_file_new_mmap(fp);
        bwfp->seek = bw_file_seek_set_from_mmap;
        bwfp->getc = bw_file_get_char_from_mmap;
        bwfp->close = bw_file_close_from_mmap;
    }
    else
    {
        bwfp->tag = fp;
        bwfp->seek = bw_file_seek_set_from_fp;
        bwfp->getc = bw_file_get_char_from_fp;
        bwfp->close = bw_file_close_from_fp;
    }
    return bwfp;
}

saidx_t bws_rankc(csaidx_t *pcsa, bwsidx_t *pbws,
                  bw_file_t *fpbw,
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
        fpbw->seek(fpbw, 0);
        while (i-- >= 0)
        {
            rank += fpbw->getc(fpbw) == c;
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
        saidx_t k;
        if (i >= pbws->lb)
        {
            rank = pbws->lbRankC[((i >> pbws->logLB) - 1) * pcsa->m + c];
            pos = (i >> pbws->logLB) << pbws->logLB; /* i & ~(pbws->lb - 1); */
        }
        i -= pbws->l;
        for (k = pos/pbws->l * pcsa->m; pos <= i; pos += pbws->l, k += pcsa->m)
        {
            rank += pbws->lRankC[k + c];
        }
        i += pbws->l;
        if (pos <= i)
        {
            c = pcsa->AtoC[c];
            pos -= pos > pbws->last;
            fpbw->seek(fpbw, pos);
            if (i >= pbws->last)
            {
                i--;
            }
            i -= pos;
            while (i-- >= 0)
            {
                rank += fpbw->getc(fpbw) == c;
            }
        }
    }
#endif
#ifdef DEBUG_BWS
    printf("rank_%c(%d) = %d\n", c, j, rank);
#endif
    return rank;
}

saidx_t bws_lf(csaidx_t *pcsa, bwsidx_t *pbws,
               bw_file_t *fpbw,
               saidx_t i)
{
    int c;
    if (i == pbws->last)
    {
        return 0;
    }
    fpbw->seek(fpbw, i - (i > pbws->last));
    c = fpbw->getc(fpbw);
    c = pcsa->CtoA[c];
    /* c = BW[i], LF[i] = C[c] + rank_c(BW, i) */
    return pcsa->K[c + 1] - 1 + bws_rankc(pcsa, pbws,
                                          fpbw,
                                          i, c);
}

saidx_t bws_sa(csaidx_t *pcsa, bwsidx_t *pbws,
               bw_file_t *fpbw,
               saidx_t i)
{
    saidx_t v;
    /*
     * LF[i] = ISA[SA[i] - 1]
     * SA[i] = SA[LF[i]] + 1
     *       = SA[LF[LF[i]] + 2
     *       = ...
     */
    for (v = 0; i % pcsa->d; v++)
    {
        i = bws_lf(pcsa, pbws,
                   fpbw,
                   i);
    }
    i /= pcsa->d;
    v += GET_SAIDX(*pcsa, SA, i);
    if (v > pcsa->n)
    {
        v -= pcsa->n + 1;
    }
    return v;
}

saidx_t bws_isa(csaidx_t *pcsa, bwsidx_t *pbws,
                bw_file_t *fpbw,
                saidx_t i)
{
    saidx_t j, v;
    /*
     * LF[j] = ISA[SA[j] - 1], i = SA[j] - 1
     * ISA[i] = LF[ISA[i + 1]]
     *        = LF[LF[ISA[i + 2]]]
     *        = ...
     * SA[0] = n => ISA[n] = 0
     */
    if (i == 0)
    {
        return pbws->last;
    }
    j = (i - 1) / pcsa->d2;
    if (j + 1 == pcsa->cache.isa_blkid)
    {
        if (j == pcsa->n_sub_1_div_d2)
        {
            if (pcsa->n - i < pcsa->cache.isa_count)
            {
                return pcsa->cache.isa_cache[pcsa->n - i];
            }
            v = pcsa->cache.isa_cache[pcsa->cache.isa_count - 1];
            j = pcsa->n - pcsa->cache.isa_count + 1;
        }
        else
        {
            j = (j + 1) * pcsa->d2;
            if (j - i < pcsa->cache.isa_count)
            {
                return pcsa->cache.isa_cache[j - i];
            }
            v = pcsa->cache.isa_cache[pcsa->cache.isa_count - 1];
            j -= pcsa->cache.isa_count - 1;
        }
    }
    else
    {
        pcsa->cache.isa_blkid = j + 1;
        pcsa->cache.isa_count = 0;
    }
    if (pcsa->cache.isa_count)
    {
        pcsa->cache.isa_count--;
    }
    else
    if (j == pcsa->n_sub_1_div_d2)
    {
        v = 0;
        j = pcsa->n;
    }
    else
    {
        v = GET_SAIDX(*pcsa, ISA, j + 1);
        j = (j + 1) * pcsa->d2;
    }
    for (; j > i; j--)
    {
        pcsa->cache.isa_cache[pcsa->cache.isa_count++] = v;
        v = bws_lf(pcsa, pbws,
                   fpbw,
                   v);
    }
    pcsa->cache.isa_cache[pcsa->cache.isa_count++] = v;
    return v;
}

saidx_t bws_isa_r(csaidx_t *pcsa, bwsidx_t *pbws,
                  bw_file_t *fpbw,
                  saidx_t i)
{
    saidx_t j, v;
    /*
     * LF[j] = ISA[SA[j] - 1], i = SA[j] - 1
     * ISA[i] = LF[ISA[i + 1]]
     *        = LF[LF[ISA[i + 2]]]
     *        = ...
     * SA[0] = n => ISA[n] = 0
     */
    if (i == 0)
    {
        return pbws->last;
    }
    j = (i - 1) / pcsa->d2;
    if (j == pcsa->n_sub_1_div_d2)
    {
        v = 0;
        j = pcsa->n;
    }
    else
    {
        v = GET_SAIDX(*pcsa, ISA, j + 1);
        j = (j + 1) * pcsa->d2;
    }
    for (; j > i; j--)
    {
        v = bws_lf(pcsa, pbws,
                   fpbw,
                   v);
    }
    return v;
}

int bws_t(csaidx_t *pcsa, bwsidx_t *pbws,
          bw_file_t *fpbw,
          saidx_t i)
{
    saidx_t j, l, size, half;
    int c;
    /*
     * F[i] = T[SA[i]] => T[i] = F[ISA[i]]
     */
    j = bws_isa(pcsa, pbws,
                fpbw,
                i);
    if (j == 0)
    {
        return -1;
    }
    size = pcsa->m;
    for (l = 0, half = size >> 1; size; size = half, half >>= 1)
    {
        if (pcsa->K[l + half + 2] - 1 < j)
        {
            l += half + 1;
            half -= (size & 1) ^ 1;
        }
    }
    c = pcsa->AtoC[l];
    return c;
}

int bws_bw(csaidx_t *pcsa, bwsidx_t *pbws,
           bw_file_t *fpbw,
           saidx_t i)
{
    int c;
    if (i == pbws->last)
    {
        return -1;
    }
    fpbw->seek(fpbw, i - (i > pbws->last));
    c = fpbw->getc(fpbw);
    return c;
}

sauchar_t *bws_substr(csaidx_t *pcsa, bwsidx_t *pbws,
                      bw_file_t *fpbw,
                      saidx_t i, int len)
{
    sauchar_t *s;
    if (!pcsa || !pbws || !fpbw || i >= pbws->n || len <= 0)
    {
        return NULL;
    }
    if (i + len > pbws->n)
    {
        len = pbws->n - i;
    }
    s = (sauchar_t *) malloc(len + 1);
    if (s)
    {
        saidx_t l, size, half;
        saidx_t *C;
        sauchar_t *p = s + len;
        *p-- = 0;
        i += len;
        i = bws_isa(pcsa, pbws,
                    fpbw,
                    i - 1) + 1;
        C = pcsa->K + 2;
        do
        {
            size = pcsa->m;
            for (l = 0, half = size >> 1; size; size = half, half >>= 1)
            {
                if (C[l + half] < i)
                {
                    l += half + 1;
                    half -= (size & 1) ^ 1;
                }
            }
            *p-- = pcsa->AtoC[l];
            i = bws_lf(pcsa, pbws,
                       fpbw,
                       i - 1) + 1;
        } while (--len);
    }
    return s;
}

int bws_search(csaidx_t *pcsa, bwsidx_t *pbws,
               bw_file_t *fpbw,
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
