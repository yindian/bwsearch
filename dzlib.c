/*
 * See Copyright Notice in bwslib.h
 */
#include "dzlib.h"
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <ctype.h>
#if !defined(htole32) || !defined(htole16)
#if BYTE_ORDER == LITTLE_ENDIAN
#define htole32(_x) (_x)
#define htole16(_x) (_x)
#define le32toh(_x) (_x)
#define le16toh(_x) (_x)
#else
#define htole32(_x) ((((_x) & 0xFF) << 24) | ((((_x) >> 8) & 0xFF) << 16) |\
                     ((((_x) >> 16) & 0xFF) << 8) | ((_x) >> 24))
#define htole16(_x) ((((_x) & 0xFF) << 8) | ((_x) >> 8))
#define le32toh(_x) htole32(_x)
#define le16toh(_x) htole16(_x)
#endif
#endif
#include <zlib.h>
#define ZSTRM_SET(_zs, _field, _val) (_zs)->_field = (_val)
#define ZSTRM_GET(_zs, _field) (_zs)->_field
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#include "mman.h"
#else
#include <sys/mman.h>
#endif
#ifdef getc
#undef getc
#endif

int dzip_compress(const char *fname, int force)
{
    int ret, err;
    FILE *fp, *gp;
    char *ofname;
    unsigned char *header;
#define CHECK_COND(_cond, _ret) \
    {\
        if (!(_cond))\
        {\
            ret = _ret;\
            err = __LINE__;\
            break;\
        }\
    }
    ret = DZ_RET_OK;
    err = 0;
    fp = gp = NULL;
    ofname = NULL;
    header = NULL;
    do
    {
        struct stat st;
        int fnlen;
        char *base;
        unsigned chunks, hdrlen;
        unsigned char *p;
        uint32_t u32;
        uint16_t u16;
        int jobs;
        z_stream *zs;
        CHECK_COND(fname, DZ_RET_INV_ARG);
        fp = fopen(fname, "rb");
        CHECK_COND(fp, DZ_RET_IO_ERR);
        CHECK_COND(!fstat(fileno(fp), &st), DZ_RET_IO_ERR);
        CHECK_COND(st.st_size <= (65535-10)/2*DZ_CHUNK_SIZE, DZ_RET_TOO_LARGE);
        fnlen = strlen(fname);
        ofname = (char *) malloc(fnlen + 4);
        CHECK_COND(ofname, DZ_RET_MEM_ERR);
        sprintf(ofname, "%s.dz", fname);
        if (!force)
        {
            gp = fopen(ofname, "rb");
            CHECK_COND(!gp, DZ_RET_EXISTING);
        }
        gp = fopen(ofname, "wb");
        CHECK_COND(gp, DZ_RET_IO_ERR);
        ofname[fnlen] = '\0';
        if ((base = strrchr(ofname, '/')))
        {
            ++base;
        }
        else if ((base = strrchr(ofname, '\\')))
        {
            struct stat dirst;
            *base = '\0';
            if (stat(ofname, &dirst))
            {
                *base = '\\';
                base = ofname;
            }
            else
            {
                ++base;
            }
        }
        else
        {
            base = ofname;
        }
        fnlen = strlen(base);
        chunks = (st.st_size + DZ_CHUNK_SIZE - 1) / DZ_CHUNK_SIZE;
        hdrlen = 10 /* gzip header */ + 2 /* xlen */ + (
                10 + 2 * chunks) /* dictzip subfield */;
        header = p = (unsigned char *) malloc(hdrlen);
        CHECK_COND(header, DZ_RET_MEM_ERR);
        memset(header, 0, hdrlen);
        *p++ = 0x1f; *p++ = 0x8b; /* gzip magic */
        *p++ = Z_DEFLATED; /* compression id */
        *p++ = 4 | 8; /* flags: fextra | fname */
        u32 = htole32(st.st_mtime); memcpy(p, &u32, 4); p += 4;
        *p++ = 2; /* compression flags: max */
        *p++ = 3; /* OS id: unix */
        assert(p == header + 10);
        u16 = 10 + 2 * chunks;
        u16 = htole16(u16); memcpy(p, &u16, 2); p += 2;
        *p++ = 'R'; *p++ = 'A'; /* dictzip magic */
        u16 = 6 + 2 * chunks; /* subfield length */
        u16 = htole16(u16); memcpy(p, &u16, 2); p += 2;
        u16 = 1; /* dictzip version */
        u16 = htole16(u16); memcpy(p, &u16, 2); p += 2;
        u16 = DZ_CHUNK_SIZE; /* chunk length */
        u16 = htole16(u16); memcpy(p, &u16, 2); p += 2;
        u16 = chunks; /* chunk count */
        u16 = htole16(u16); memcpy(p, &u16, 2); p += 2;
        assert(p == header + 22);
        CHECK_COND(fwrite(header, 1, hdrlen, gp) == hdrlen, DZ_RET_IO_ERR);
        CHECK_COND(fwrite(base, 1, fnlen + 1, gp) == fnlen + 1, DZ_RET_IO_ERR);
#ifdef _OPENMP
        jobs = omp_get_max_threads();
#else
        jobs = 1;
#endif
        jobs <<= 6;
        zs = (z_stream *) malloc(sizeof(z_stream) * jobs);
        CHECK_COND(zs, DZ_RET_MEM_ERR);
        memset(zs, 0, sizeof(z_stream) * jobs);
        do
        {
            Bytef *buf;
            uLong crc;
            int i;
            int r;
            int idx;
            size_t size;
            assert(DZ_CHUNK_SIZE <= (1 << 16) - sizeof(uLong));
            assert(DZ_DEFLATE_BUF_SIZE <= (1 << 16));
            buf = (Bytef *) malloc((jobs * 2) << 16);
            CHECK_COND(buf, DZ_RET_MEM_ERR);
            memset(buf, 0, (jobs * 2) << 16);
#define IN_BUF(_i)  (buf + ((_i) * DZ_CHUNK_SIZE))
#define OUT_BUF(_i) (buf + ((jobs + (_i)) << 16))
#define CRC_BUF(_i) (buf + (jobs << 16) - ((_i) + 1) * sizeof(uLong))
            for (i = 0; i < jobs; i++)
            {
                r = deflateInit2(
                        &zs[i],
                        Z_BEST_COMPRESSION,
                        Z_DEFLATED,
                        -15,
                        9,
                        Z_DEFAULT_STRATEGY);
                CHECK_COND(r == Z_OK, DZ_RET_ZLIB_ERR + r);
            }
            if (err)
            {
                free(buf);
                break;
            }
            crc = crc32(0, NULL, 0);
            idx = 22;
            while (!feof(fp))
            {
                int cnt;
                size = fread(buf, 1, DZ_CHUNK_SIZE * jobs, fp);
                if (!size)
                {
                    break;
                }
                cnt = (size + DZ_CHUNK_SIZE - 1) / DZ_CHUNK_SIZE;
#ifdef _OPENMP
#pragma omp parallel for default(shared) private(i, u16)
#endif
                for (i = 0; i < cnt; i++)
                {
                    uLong *pcrc = (uLong *) CRC_BUF(i);
                    ZSTRM_SET(&zs[i], next_in, IN_BUF(i));
                    if (i == cnt - 1)
                    {
                        ZSTRM_SET(&zs[i], avail_in, size - DZ_CHUNK_SIZE * i);
                    }
                    else
                    {
                        ZSTRM_SET(&zs[i], avail_in, DZ_CHUNK_SIZE);
                    }
                    *pcrc = crc32(0, zs[i].next_in, zs[i].avail_in);
                    ZSTRM_SET(&zs[i], next_out, OUT_BUF(i));
                    ZSTRM_SET(&zs[i], avail_out, DZ_DEFLATE_BUF_SIZE);
                    r = deflate(&zs[i], Z_FULL_FLUSH);
                    assert(r == Z_OK);
                    u16 = DZ_DEFLATE_BUF_SIZE - zs[i].avail_out;
                    u16 = htole16(u16);
                    memcpy(header + idx + i * 2, &u16, 2);
                }
                for (i = 0; i < cnt; i++)
                {
                    uLong *pcrc = (uLong *) CRC_BUF(i);
                    CHECK_COND(fwrite(OUT_BUF(i), 1,
                                      DZ_DEFLATE_BUF_SIZE - zs[i].avail_out, gp)
                               == DZ_DEFLATE_BUF_SIZE - zs[i].avail_out,
                               DZ_RET_IO_ERR);
                    crc = crc32_combine(
                            crc, *pcrc, i == cnt - 1 ?
                            size - DZ_CHUNK_SIZE * i : DZ_CHUNK_SIZE);
                }
                if (err)
                {
                    break;
                }
                idx += cnt * 2;
            }
            assert(idx == 22 + chunks * 2);
            for (i = 0; i < jobs; i++)
            {
                ZSTRM_SET(&zs[i], next_in, IN_BUF(i));
                ZSTRM_SET(&zs[i], avail_in, 0);
                ZSTRM_SET(&zs[i], next_out, OUT_BUF(i));
                ZSTRM_SET(&zs[i], avail_out, DZ_DEFLATE_BUF_SIZE);
                r = deflate(&zs[i], Z_FINISH);
                CHECK_COND(r == Z_STREAM_END, DZ_RET_ZLIB_ERR + r);
                CHECK_COND(zs[i].avail_in == 0, DZ_RET_ZLIB_ERR);
                r = deflateEnd(&zs[i]);
                CHECK_COND(r == Z_OK, DZ_RET_ZLIB_ERR + r);
            }
            assert(i == jobs);
            for (i = 0; i < jobs - 1; i++)
            {
                CHECK_COND(zs[i].avail_out==zs[i+1].avail_out, DZ_RET_ZLIB_ERR);
                CHECK_COND(!memcmp(OUT_BUF(i), OUT_BUF(i+1),
                                   DZ_DEFLATE_BUF_SIZE - zs[i].avail_out),
                           DZ_RET_ZLIB_ERR);
            }
            assert(i + 1 == jobs);
            assert(zs[i].avail_out + 2 == DZ_DEFLATE_BUF_SIZE);
            do
            {
                CHECK_COND(fwrite(OUT_BUF(0), 1,
                                  DZ_DEFLATE_BUF_SIZE - zs[0].avail_out, gp)
                           == DZ_DEFLATE_BUF_SIZE - zs[0].avail_out,
                           DZ_RET_IO_ERR);
            } while (0);
            free(buf);
            if (err)
            {
                break;
            }
            u32 = htole32(crc);
            CHECK_COND(fwrite(&u32, 1, 4, gp) == 4, DZ_RET_IO_ERR);
            u32 = st.st_size;
            u32 = htole32(u32);
            CHECK_COND(fwrite(&u32, 1, 4, gp) == 4, DZ_RET_IO_ERR);
        } while (0);
        free(zs);
        if (err)
        {
            break;
        }
        CHECK_COND(!fseek(gp, 22, SEEK_SET), DZ_RET_IO_ERR);
        CHECK_COND(fwrite(header + 22, 2, chunks, gp) == chunks, DZ_RET_IO_ERR);
    } while (0);
    if (fp)
    {
        fclose(fp);
    }
    if (gp)
    {
        fclose(gp);
    }
    if (ofname)
    {
        free(ofname);
    }
    if (header)
    {
        free(header);
    }
    if (err)
    {
        fprintf(stderr, "Error in %s:%d ret %d\n", __FILE__, err, ret);
    }
    return ret;
}

int dzip_decompress(const char *fname, int force)
{
    int ret, err;
    FILE *fp, *gp;
    char *ofname;
    bw_file_t *bwfp;
    ret = DZ_RET_OK;
    err = 0;
    fp = gp = NULL;
    ofname = NULL;
    do
    {
        int fnlen;
        char *p;
        saidx_t len;
        CHECK_COND(fname, DZ_RET_INV_ARG);
        CHECK_COND(strrchr(fname, '.'), DZ_RET_INV_ARG);
        fp = fopen(fname, "rb");
        CHECK_COND(fp, DZ_RET_IO_ERR);
        fnlen = strlen(fname);
        ofname = (char *) malloc(fnlen + 1);
        CHECK_COND(ofname, DZ_RET_MEM_ERR);
        strcpy(ofname, fname);
        p = strrchr(ofname, '.');
        *p = '\0';
        if (!force)
        {
            gp = fopen(ofname, "rb");
            CHECK_COND(!gp, DZ_RET_EXISTING);
        }
        gp = fopen(ofname, "wb");
        CHECK_COND(gp, DZ_RET_IO_ERR);
        bwfp = bw_file_new_from_dzip_fp(fp, &ret, &err);
        if (!bwfp)
        {
            break;
        }
        len = bwfp->size(bwfp);
#if 0
        while (len--)
        {
#if 1
            putc(bwfp->getc(bwfp), gp);
#else
            bwfp->getc(bwfp);
#endif
        }
#else
        {
#define BUF_SIZE            (1 << 16)
#define DZ_READ_SMALL_LEN   3
#define SANITY_CHECK        0
            sauchar_t buf[BUF_SIZE];
            saidx_t n;
            for (; len; len -= n)
            {
#if SANITY_CHECK
                assert(len > 0);
#endif
                n = bwfp->read(bwfp, buf, BUF_SIZE);
#if SANITY_CHECK
                CHECK_COND(n, DZ_RET_IO_ERR);
#endif
#if 1
                fwrite(buf, 1, n, gp);
#endif
            }
        }
#endif
        bwfp->close(bwfp);
    } while (0);
    if (fp)
    {
        fclose(fp);
    }
    if (gp)
    {
        fclose(gp);
    }
    if (ofname)
    {
        free(ofname);
    }
    if (err)
    {
        fprintf(stderr, "Error in %s:%d ret %d\n", __FILE__, err, ret);
    }
    return ret;
}

#ifndef DZ_CHUNK_BUF_CNT
#define DZ_CHUNK_BUF_CNT 2
#endif

typedef struct _bw_dzip_fp_t {
    sauchar_t *map;
    off_t   base;
    off_t   len;
    off_t   rawlen;
    off_t   rawbase;
    uint32_t mtime;
    off_t   chunk_base;
    int     chunk_len;
    int     chunks;
    off_t   *offsets;
    int     duped;
    int     pos_chunk;
    int     pos_in_chunk;
    sauchar_t *cur_pos;
    sauchar_t *cur_chunk_head;
    sauchar_t *cur_chunk_tail;
    int     cache[DZ_CHUNK_BUF_CNT];
    sauchar_t*buf[DZ_CHUNK_BUF_CNT];
    z_stream zs;
} bw_dzip_fp_t;

static bw_dzip_fp_t *bw_file_new_dzip_fp(FILE *fp, int *pret, int *perr)
{
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) malloc(sizeof(bw_dzip_fp_t));
#undef CHECK_COND
#define CHECK_COND(_cond, _ret) \
    {\
        if (!(_cond))\
        {\
            if (pret) *pret = _ret;\
            if (perr) *perr = __LINE__;\
            break;\
        }\
    }
#define PERROR_IF(_cond, _msg) \
    {\
        if (_cond)\
        {\
            if (pret) *pret = DZ_RET_IO_ERR;\
            if (perr) *perr = __LINE__;\
            perror(_msg);\
            break;\
        }\
    }
    do
    {
        int i;
        CHECK_COND(bwdz, DZ_RET_MEM_ERR);
        memset(bwdz, 0, sizeof(bw_dzip_fp_t));
        bwdz->rawbase = ftello(fp);
        do
        {
            off_t pos;
            uint32_t u32;
            uint16_t u16;
            int ch;
            int flags;
            int xlen, len;
            PERROR_IF(fseeko(fp, 0, SEEK_END), "seek failed");
            bwdz->rawlen = ftello(fp);
            PERROR_IF(fseeko(fp, bwdz->rawbase, SEEK_SET), "seek failed");
            CHECK_COND(fgetc(fp) == 0x1f, DZ_RET_MALFORM);
            CHECK_COND(fgetc(fp) == 0x8b, DZ_RET_MALFORM); /* gzip magic */
            CHECK_COND(fgetc(fp) == Z_DEFLATED, DZ_RET_MALFORM); /*compression*/
            CHECK_COND((flags = fgetc(fp)) != EOF, DZ_RET_MALFORM); /* flags */
            CHECK_COND((flags & 4), DZ_RET_MALFORM); /* fextra */
            CHECK_COND(fread(&u32, 4, 1, fp) == 1, DZ_RET_MALFORM); /* mtime */
            bwdz->mtime = le32toh(u32);
            CHECK_COND((ch = fgetc(fp)) != EOF, DZ_RET_MALFORM); /* flags 2 */
            CHECK_COND((ch = fgetc(fp)) != EOF, DZ_RET_MALFORM); /* os */
            CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM); /* xlen */
            xlen = le16toh(u16);
            while (xlen > 0)
            {
                int si1, si2;
                CHECK_COND((si1 = fgetc(fp)) != EOF, DZ_RET_MALFORM); /* si1 */
                CHECK_COND((si2 = fgetc(fp)) != EOF, DZ_RET_MALFORM); /* si2 */
                CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM); /*len*/
                len = le16toh(u16);
                if (si1 == 'R' && si2 == 'A')
                {
                    CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM);
                    CHECK_COND(le16toh(u16) == 1, DZ_RET_MALFORM); /* version */
                    CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM);
                    bwdz->chunk_len = le16toh(u16);
                    CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM);
                    bwdz->chunks = le16toh(u16);
                    CHECK_COND(len == bwdz->chunks * 2 + 6, DZ_RET_MALFORM);
                    bwdz->chunk_base = ftello(fp);
                    PERROR_IF(fseeko(fp, len - 6, SEEK_CUR), "seek failed");
                }
                else
                {
                    PERROR_IF(fseeko(fp, len, SEEK_CUR), "seek failed");
                }
                xlen -= 4 + len;
            }
            if (xlen > 0)
            {
                break;
            }
            CHECK_COND(xlen == 0, DZ_RET_MALFORM);
            CHECK_COND(bwdz->chunk_base, DZ_RET_MALFORM);
            if (flags & 8) /* fname */
            {
                for (ch = fgetc(fp); ch && ch != EOF; ch = fgetc(fp))
                {}
                CHECK_COND(ch == '\0', DZ_RET_MALFORM);
            }
            if (flags & 0x10) /* comment */
            {
                for (ch = fgetc(fp); ch && ch != EOF; ch = fgetc(fp))
                {}
                CHECK_COND(ch == '\0', DZ_RET_MALFORM);
            }
            if (flags & 2) /* fhcrc */
            {
                CHECK_COND((ch = fgetc(fp)) != EOF, DZ_RET_MALFORM);
                CHECK_COND((ch = fgetc(fp)) != EOF, DZ_RET_MALFORM);
            }
            bwdz->base = ftello(fp);
            pos = bwdz->base;
            PERROR_IF(fseeko(fp, bwdz->chunk_base, SEEK_SET), "seek failed");
            bwdz->offsets = (off_t *) malloc(sizeof(off_t) * (bwdz->chunks+1));
            CHECK_COND(bwdz->offsets, DZ_RET_MEM_ERR);
            for (i = 0; i < bwdz->chunks; i++)
            {
                CHECK_COND(fread(&u16, 2, 1, fp) == 1, DZ_RET_MALFORM);
                bwdz->offsets[i] = pos;
                pos += le16toh(u16);
            }
            CHECK_COND(i == bwdz->chunks, DZ_RET_MALFORM);
            bwdz->offsets[i] = pos;
            pos += 2; /* Z_FINISH */
            PERROR_IF(fseeko(fp, pos, SEEK_SET), "seek failed");
            CHECK_COND(fread(&u32, 4, 1, fp) == 1, DZ_RET_MALFORM); /* crc */
            CHECK_COND(fread(&u32, 4, 1, fp) == 1, DZ_RET_MALFORM); /* size */
            bwdz->len = le32toh(u32);
            PERROR_IF(fseeko(fp, bwdz->rawbase, SEEK_SET), "seek failed");
            bwdz->map = (sauchar_t *) mmap(NULL, bwdz->rawlen,
                                          PROT_READ, MAP_SHARED,
                                          fileno(fp), 0);
            PERROR_IF(!bwdz->map || bwdz->map == MAP_FAILED, "mmap failed");
            bwdz->rawlen -= bwdz->rawbase;
            bwdz->duped = 0;
            for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
            {
                bwdz->cache[i] = -1;
                bwdz->buf[i] = (sauchar_t *) malloc(bwdz->chunk_len);
                CHECK_COND(bwdz->buf[i], DZ_RET_MEM_ERR);
            }
            if (i < DZ_CHUNK_BUF_CNT)
            {
                break;
            }
            i = inflateInit2(&bwdz->zs, -15);
            CHECK_COND(i == Z_OK, DZ_RET_ZLIB_ERR + i);
            return bwdz;
        } while (0);
        for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
        {
            if (bwdz->buf[i])
            {
                free(bwdz->buf[i]);
            }
        }
        if (bwdz->offsets)
        {
            free(bwdz->offsets);
        }
        free(bwdz);
    } while (0);
    return NULL;
}

static saidx_t bw_file_size_from_dzip_fp(bw_file_t *bwfp)
{
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    return bwdz->len;
}

static void bw_file_seek_set_from_dzip_fp(bw_file_t *bwfp, saidx_t pos)
{
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    if (pos >= 0 && pos <= bwdz->len)
    {
        bwdz->pos_chunk = pos / bwdz->chunk_len;
        bwdz->pos_in_chunk = pos % bwdz->chunk_len;
        bwdz->cur_pos = bwdz->cur_chunk_tail = NULL;
        bwdz->cur_chunk_head = NULL;
    }
    else
    {
        fprintf(stderr, "seek beyond range\n");
    }
}

static saidx_t bw_file_tell_from_dzip_fp(bw_file_t *bwfp)
{
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    if (!bwdz->cur_pos)
    {
        return bwdz->pos_chunk * bwdz->chunk_len + bwdz->pos_in_chunk;
    }
    return bwdz->pos_chunk * bwdz->chunk_len + (
            bwdz->cur_pos - bwdz->cur_chunk_head);
}

static void bw_dzip_shift_cache(bw_dzip_fp_t *bwdz, int till)
{
    int i;
    for (i = 0; i < till; i++)
    {
        bwdz->cache[i + 1] = bwdz->cache[i];
        bwdz->buf[i + 1] = bwdz->buf[i];
    }
}

#ifndef unlikely
#ifdef __GNUC__
#define unlikely(_x)     __builtin_expect((_x),0)
#else
#define unlikely(_x)     (_x)
#endif
#endif
static int bw_file_get_char_from_dzip_fp(bw_file_t *bwfp)
{
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    if (unlikely(bwdz->cur_pos == bwdz->cur_chunk_tail))
    {
        int i;
        sauchar_t *buf;
        int chunk = bwdz->pos_chunk;
        if (bwdz->cur_pos)
        {
            if (++chunk == bwdz->chunks)
            {
                return -1;
            }
            bwdz->pos_chunk = chunk;
            bwdz->pos_in_chunk = 0;
        }
        for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
        {
            if (bwdz->cache[i] == chunk)
            {
                break;
            }
        }
        if (i == 0)
        {
            buf = bwdz->buf[i];
        }
        else
        {
            if (i == DZ_CHUNK_BUF_CNT)
            {
                bwdz->cache[DZ_CHUNK_BUF_CNT - 1] = -1;
                ZSTRM_SET(&bwdz->zs, next_in, (Bytef *) bwdz->map +
                          bwdz->offsets[chunk]);
                ZSTRM_SET(&bwdz->zs, avail_in,
                          bwdz->offsets[chunk + 1] - bwdz->offsets[chunk]);
                ZSTRM_SET(&bwdz->zs, next_out, (Bytef *)
                          bwdz->buf[DZ_CHUNK_BUF_CNT - 1]);
                ZSTRM_SET(&bwdz->zs, avail_out, bwdz->chunk_len);
                if (inflate(&bwdz->zs, Z_PARTIAL_FLUSH) != Z_OK)
                {
                    fprintf(stderr, "inflate error %s\n", bwdz->zs.msg);
                }
                assert(ZSTRM_GET(&bwdz->zs, avail_in) == 0);
                --i;
            }
            buf = bwdz->buf[i];
            bw_dzip_shift_cache(bwdz, i);
            bwdz->cache[0] = chunk;
            bwdz->buf[0] = buf;
        }
        if (unlikely(chunk + 1 == bwdz->chunks))
        {
            bwdz->cur_chunk_tail = buf + (bwdz->len % bwdz->chunk_len);
        }
        else
        {
            bwdz->cur_chunk_tail = buf + bwdz->chunk_len;
        }
        bwdz->cur_chunk_head = buf;
        bwdz->cur_pos = buf + bwdz->pos_in_chunk;
    }
    return *bwdz->cur_pos++;
}

static saidx_t bw_file_read_from_dzip_fp(bw_file_t *bwfp,
                                         sauchar_t *buf, int len)
{
    int c;
    sauchar_t *p = buf;
    if (len > DZ_READ_SMALL_LEN)
    {
        bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
        while (len)
        {
            int step = bwdz->cur_chunk_tail - bwdz->cur_pos;
            if (step == 0)
            {
                if ((c = bw_file_get_char_from_dzip_fp(bwfp)) < 0)
                {
                    break;
                }
                *p++ = c;
                --len;
            }
            else
            {
                if (step > len)
                {
                    step = len;
                }
                memcpy(p, bwdz->cur_pos, step);
                p += step;
                bwdz->cur_pos += step;
                len -= step;
            }
        }
    }
    else
    {
        while (len)
        {
            if ((c = bw_file_get_char_from_dzip_fp(bwfp)) < 0)
            {
                break;
            }
            *p++ = c;
            --len;
        }
    }
    return p - buf;
}

static void bw_file_close_from_dzip_fp(bw_file_t *bwfp)
{
    int i;
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    if (!bwdz->duped)
    {
        if (munmap(bwdz->map, bwdz->rawbase + bwdz->rawlen))
        {
            perror("munmap failed");
        }
        if (bwdz->offsets)
        {
            free(bwdz->offsets);
        }
    }
    for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
    {
        if (bwdz->buf[i])
        {
            free(bwdz->buf[i]);
        }
    }
    inflateEnd(&bwdz->zs);
    free(bwdz);
    free(bwfp);
}

static bw_file_t *bw_file_dup_from_dzip_fp(bw_file_t *bwfp)
{
    int i;
    bw_file_t *bwgp;
    bw_dzip_fp_t *bzdw;
    bw_dzip_fp_t *bwdz = (bw_dzip_fp_t *) bwfp->tag;
    if (!bwdz)
    {
        return NULL;
    }
    bwgp = (bw_file_t *) malloc(sizeof(bw_file_t));
    if (!bwgp)
    {
        return NULL;
    }
    bzdw = (bw_dzip_fp_t *) malloc(sizeof(bw_dzip_fp_t));
    if (!bzdw)
    {
        free(bwgp);
        return NULL;
    }
    *bzdw = *bwdz;
    bzdw->duped = 1;
    for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
    {
        bzdw->buf[i] = NULL;
    }
    for (i = 0; i < DZ_CHUNK_BUF_CNT; i++)
    {
        bzdw->buf[i] = (sauchar_t *) malloc(bzdw->chunk_len);
        if (!bzdw->buf[i])
        {
            break;
        }
        if (bzdw->cache[i] >= 0)
        {
            memcpy(bzdw->buf[i], bwdz->buf[i], bzdw->chunk_len);
        }
    }
    *bwgp = *bwfp;
    bwgp->tag = bzdw;
    memset(&bzdw->zs, 0, sizeof(z_stream));
    if (i < DZ_CHUNK_BUF_CNT ||
        inflateInit2(&bzdw->zs, -15) != Z_OK)
    {
        bw_file_close_from_dzip_fp(bwgp);
        return NULL;
    }
    return bwgp;
}
bw_file_t *bw_file_new_from_dzip_fp(FILE *fp, int *pret, int *perr)
{
    bw_file_t *bwfp;
#undef CHECK_COND
#define CHECK_COND(_cond, _ret) \
    {\
        if (!(_cond))\
        {\
            if (pret) *pret = _ret;\
            if (perr) *perr = __LINE__;\
            return NULL;\
        }\
    }
    CHECK_COND(fp, DZ_RET_INV_ARG);
    bwfp = (bw_file_t *) malloc(sizeof(bw_file_t));
    CHECK_COND(bwfp, DZ_RET_MEM_ERR);
    bwfp->tag = bw_file_new_dzip_fp(fp, pret, perr);
    bwfp->seek = bw_file_seek_set_from_dzip_fp;
    bwfp->tell = bw_file_tell_from_dzip_fp;
    bwfp->size = bw_file_size_from_dzip_fp;
    bwfp->getc = bw_file_get_char_from_dzip_fp;
    bwfp->read = bw_file_read_from_dzip_fp;
    bwfp->close = bw_file_close_from_dzip_fp;
    bwfp->dup = bw_file_dup_from_dzip_fp;
    return bwfp;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
