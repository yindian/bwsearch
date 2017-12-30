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
#ifdef _WIN32
#include <sys/param.h>
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
#if BYTE_ORDER == LITTLE_ENDIAN
#define htole32(_x) (_x)
#define htole16(_x) (_x)
#else
#define htole32(_x) ((((_x) & 0xFF) << 24) | ((((_x) >> 8) & 0xFF) << 16) |\
                     ((((_x) >> 16) & 0xFF) << 8) | ((_x) >> 24))
#define htole16(_x) ((((_x) & 0xFF) << 8) | ((_x) >> 8))
#endif
#else
#include <stdint.h>
#include <endian.h>
#endif
#include <zlib.h>
#define ZSTRM_SET(_zs, _field, _val) (_zs)->_field = (_val)
#define ZSTRM_GET(_zs, _field) (_zs)->_field

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
#if 0
        off_t size;
#else
        struct stat st;
#endif
        int fnlen;
        char *base;
        unsigned chunks, hdrlen;
        unsigned char *p;
        uint32_t u32;
        uint16_t u16;
        CHECK_COND(fname, DZ_RET_INV_ARG);
        fp = fopen(fname, "rb");
        CHECK_COND(fp, DZ_RET_IO_ERR);
#if 0
        CHECK_COND(!fseeko(fp, 0, SEEK_END), DZ_RET_IO_ERR);
        size = ftello(fp);
        CHECK_COND(size / DZ_CHUNK_SIZE <= (65535 - 10) / 2, DZ_RET_TOO_LARGE);
        CHECK_COND(!fseeko(fp, 0, SEEK_SET), DZ_RET_IO_ERR);
#else
        CHECK_COND(!fstat(fileno(fp), &st), DZ_RET_IO_ERR);
        CHECK_COND(st.st_size <= (65535-10)/2*DZ_CHUNK_SIZE, DZ_RET_TOO_LARGE);
#endif
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
        CHECK_COND(fwrite(header, 1, hdrlen, gp) == hdrlen, DZ_RET_IO_ERR);
        CHECK_COND(fwrite(base, 1, fnlen + 1, gp) == fnlen + 1, DZ_RET_IO_ERR);
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
        fprintf(stderr, "Error in %s:%d\n", __FILE__, err);
    }
    return ret;
}

int dzip_decompress(const char *fname, int force)
{
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
