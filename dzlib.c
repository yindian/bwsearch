/*
 * See Copyright Notice in bwslib.h
 */
#include "dzlib.h"
#define _POSIX_SOURCE
#define _BSD_SOURCE
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
#else
#define htole32(_x) ((((_x) & 0xFF) << 24) | ((((_x) >> 8) & 0xFF) << 16) |\
                     ((((_x) >> 16) & 0xFF) << 8) | ((_x) >> 24))
#define htole16(_x) ((((_x) & 0xFF) << 8) | ((_x) >> 8))
#endif
#endif
#include <zlib.h>
#define ZSTRM_SET(_zs, _field, _val) (_zs)->_field = (_val)
#define ZSTRM_GET(_zs, _field) (_zs)->_field
#ifdef _OPENMP
#include <omp.h>
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
            assert(DZ_CHUNK_SIZE <= (1 << 16));
            assert(DZ_DEFLATE_BUF_SIZE <= (1 << 16));
            buf = (Bytef *) malloc((jobs * 2) << 16);
            CHECK_COND(buf, DZ_RET_MEM_ERR);
            memset(buf, 0, (jobs * 2) << 16);
#define IN_BUF(_i)  (buf + ((_i) * DZ_CHUNK_SIZE))
#define OUT_BUF(_i) (buf + ((jobs + (_i)) << 16))
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
                crc = crc32(crc, buf, size);
                cnt = (size + DZ_CHUNK_SIZE - 1) / DZ_CHUNK_SIZE;
#ifdef _OPENMP
#pragma omp parallel for default(shared) private(i, u16)
#endif
                for (i = 0; i < cnt; i++)
                {
                    ZSTRM_SET(&zs[i], next_in, IN_BUF(i));
                    if (i == cnt - 1)
                    {
                        ZSTRM_SET(&zs[i], avail_in, size - DZ_CHUNK_SIZE * i);
                    }
                    else
                    {
                        ZSTRM_SET(&zs[i], avail_in, DZ_CHUNK_SIZE);
                    }
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
                    CHECK_COND(fwrite(OUT_BUF(i), 1,
                                      DZ_DEFLATE_BUF_SIZE - zs[i].avail_out, gp)
                               == DZ_DEFLATE_BUF_SIZE - zs[i].avail_out,
                               DZ_RET_IO_ERR);
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
            for (i = 0; i < jobs - 1; i++)
            {
                CHECK_COND(zs[i].avail_out==zs[i+1].avail_out, DZ_RET_ZLIB_ERR);
                CHECK_COND(!memcmp(OUT_BUF(i), OUT_BUF(i+1),
                                   DZ_DEFLATE_BUF_SIZE - zs[i].avail_out),
                           DZ_RET_ZLIB_ERR);
            }
            do
            {
                CHECK_COND(fwrite(OUT_BUF(0), 1,
                                  DZ_DEFLATE_BUF_SIZE - zs[0].avail_out, gp)
                           == DZ_DEFLATE_BUF_SIZE - zs[0].avail_out,
                           DZ_RET_IO_ERR);
            } while (0);
            free(buf);
            u32 = htole32(crc);
            CHECK_COND(fwrite(&u32, 1, 4, gp) == 4, DZ_RET_IO_ERR);
            u32 = st.st_size;
            u32 = htole32(u32);
            CHECK_COND(fwrite(&u32, 1, 4, gp) == 4, DZ_RET_IO_ERR);
        } while (0);
        free(zs);
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
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
